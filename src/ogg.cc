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

#include <errno.h>
#include <string.h>

using namespace std::literals::string_literals;

bool ot::is_opus_stream(const ogg_page& identification_header)
{
	if (ogg_page_bos(&identification_header) == 0)
		return false;
	if (identification_header.body_len < 8)
		return false;
	return (memcmp(identification_header.body, "OpusHead", 8) == 0);
}

bool ot::ogg_reader::next_page()
{
	int rc;
	while ((rc = ogg_sync_pageout(&sync, &page)) != 1) {
		if (rc == -1) {
			throw status {st::bad_stream,
			              absolute_page_no == (size_t) -1 ? "Input is not a valid Ogg file."
			                                              : "Unsynced data in stream."};
		}
		if (ogg_sync_check(&sync) != 0)
			throw status {st::libogg_error, "ogg_sync_check signalled an error."};
		if (feof(file)) {
			if (sync.fill != sync.returned)
				throw status {st::bad_stream, "Unsynced data at end of stream."};
			return false; // end of sream
		}
		char* buf = ogg_sync_buffer(&sync, 65536);
		if (buf == nullptr)
			throw status {st::libogg_error, "ogg_sync_buffer failed."};
		size_t len = fread(buf, 1, 65536, file);
		if (ferror(file))
			throw status {st::standard_error, "fread error: "s + strerror(errno)};
		if (ogg_sync_wrote(&sync, len) != 0)
			throw status {st::libogg_error, "ogg_sync_wrote failed."};
	}
	++absolute_page_no;
	return true;
}

void ot::ogg_reader::process_header_packet(const std::function<void(ogg_packet&)>& f)
{
	if (ogg_page_continued(&page))
		throw status {ot::st::error, "Unexpected continued header page."};
	ogg_logical_stream stream(ogg_page_serialno(&page));
	stream.pageno = ogg_page_pageno(&page);
	if (ogg_stream_pagein(&stream, &page) != 0)
		throw status {st::libogg_error, "ogg_stream_pagein failed."};

	ogg_packet packet;
	int rc = ogg_stream_packetout(&stream, &packet);
	if (ogg_stream_check(&stream) != 0 || rc == -1)
		throw status {ot::st::libogg_error, "ogg_stream_packetout failed."};
	else if (rc == 0)
		throw status {ot::st::error,
		              "Reading header packets spanning multiple pages are not yet supported. "
		              "Please file an issue to make your wish known."};

	f(packet);

	/* Ensure that there are no other segments left in the packet using the lacing state of the
	 * stream. These are the relevant variables, as far as I understood them:
	 *  - lacing_vals: extensible array containing the lacing values of the segments,
	 *  - lacing_fill: number of elements in lacing_vals (not the capacity),
	 *  - lacing_returned: index of the next segment to be processed. */
	if (stream.lacing_returned != stream.lacing_fill)
		throw status {ot::st::error, "Header page contains more than a single packet."};
}

void ot::ogg_writer::write_page(const ogg_page& page)
{
	if (page.header_len < 0 || page.body_len < 0)
		throw status {st::int_overflow, "Overflowing page length"};
	auto header_len = static_cast<size_t>(page.header_len);
	auto body_len = static_cast<size_t>(page.body_len);
	if (fwrite(page.header, 1, header_len, file) < header_len)
		throw status {st::standard_error, "fwrite error: "s + strerror(errno)};
	if (fwrite(page.body, 1, body_len, file) < body_len)
		throw status {st::standard_error, "fwrite error: "s + strerror(errno)};
}

void ot::ogg_writer::write_header_packet(int serialno, int pageno, ogg_packet& packet)
{
	ogg_logical_stream stream(serialno);
	stream.b_o_s = (pageno != 0);
	stream.pageno = pageno;
	if (ogg_stream_packetin(&stream, &packet) != 0)
		throw status {ot::st::libogg_error, "ogg_stream_packetin failed"};

	ogg_page page;
	if (ogg_stream_flush(&stream, &page) != 0)
		write_page(page);
	else
		throw status {ot::st::libogg_error, "ogg_stream_flush failed"};

	if (ogg_stream_flush(&stream, &page) != 0)
		throw status {ot::st::error,
		              "Writing header packets spanning multiple pages are not yet supported. "
		              "Please file an issue to make your wish known."};

	if (ogg_stream_check(&stream) != 0)
		throw status {st::libogg_error, "ogg_stream_check failed"};
}
