/**
 * \file src/ogg.c
 * \ingroup ogg
 *
 * High-level interface for libogg.
 *
 * This module is not meant to be a complete libogg wrapper, but rather a convenient and highly
 * specialized layer above libogg and stdio.
 */

#include <opustags.h>

#include <string.h>

using namespace std::literals::string_literals;

ot::status ot::ogg_reader::read_page()
{
	while (ogg_sync_pageout(&sync, &page) != 1) {
		if (feof(file))
			return {st::end_of_stream, "End of stream was reached"};
		char* buf = ogg_sync_buffer(&sync, 65536);
		if (buf == nullptr)
			return {st::libogg_error, "ogg_sync_buffer failed"};
		size_t len = fread(buf, 1, 65536, file);
		if (ferror(file))
			return {st::standard_error, "fread error: "s + strerror(errno)};
		if (ogg_sync_wrote(&sync, len) != 0)
			return {st::libogg_error, "ogg_sync_wrote failed"};
		if (ogg_sync_check(&sync) != 0)
			return {st::libogg_error, "ogg_sync_check failed"};
	}
	return st::ok;
}

ot::status ot::ogg_reader::read_header_packet(const std::function<status(ogg_packet&)>& f)
{
	ogg_stream stream(ogg_page_serialno(&page));
	stream.pageno = ogg_page_pageno(&page);
	if (ogg_stream_pagein(&stream, &page) != 0)
		return {st::libogg_error, "ogg_stream_pagein failed."};
	ogg_packet packet;
	int rc = ogg_stream_packetout(&stream, &packet);
	if (ogg_stream_check(&stream) != 0 || rc == -1)
		return {ot::st::libogg_error, "ogg_stream_packetout failed."};
	else if (rc == 0)
		return {ot::st::error, "Header pages must not be empty."};
	ot::status f_rc = f(packet);
	if (f_rc != ot::st::ok)
		return f_rc;
	/* Ensure that there are no other segments left in the packet using the lacing state of the
	 * stream. These are the relevant variables, as far as I understood them:
	 *  - lacing_vals: extensible array containing the lacing values of the segments,
	 *  - lacing_fill: number of elements in lacing_vals (not the capacity),
	 *  - lacing_returned: index of the next segment to be processed. */
	if (stream.lacing_returned != stream.lacing_fill)
		return {ot::st::error, "Header page contains more than a single packet."};
	return ot::st::ok;
}

ot::status ot::ogg_writer::write_page(const ogg_page& page)
{
	if (page.header_len < 0 || page.body_len < 0)
		return {st::int_overflow, "Overflowing page length"};
	auto header_len = static_cast<size_t>(page.header_len);
	auto body_len = static_cast<size_t>(page.body_len);
	if (fwrite(page.header, 1, header_len, file) < header_len)
		return {st::standard_error, "fwrite error: "s + strerror(errno)};
	if (fwrite(page.body, 1, body_len, file) < body_len)
		return {st::standard_error, "fwrite error: "s + strerror(errno)};
	return st::ok;
}

ot::status ot::ogg_writer::write_header_packet(int serialno, int pageno, ogg_packet& packet)
{
	ogg_stream stream(serialno);
	stream.b_o_s = (pageno != 0);
	stream.pageno = pageno;
	if (ogg_stream_packetin(&stream, &packet) != 0)
		return {ot::st::libogg_error, "ogg_stream_packetin failed"};
	ogg_page page;
	if (ogg_stream_flush(&stream, &page) != 0) {
		ot::status rc = write_page(page);
		if (rc != ot::st::ok)
			return rc;
	} else {
		return {ot::st::libogg_error, "ogg_stream_flush failed"};
	}
	if (ogg_stream_flush(&stream, &page) != 0)
		return {ot::st::error,
		        "Header packets spanning multiple pages are not yet supported. "
		        "Please file an issue to make your wish known."};
	if (ogg_stream_check(&stream) != 0)
		return {st::libogg_error, "ogg_stream_check failed"};
	return ot::st::ok;
}
