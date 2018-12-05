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

ot::ogg_reader::ogg_reader(FILE* input)
	: file(input)
{
	ogg_sync_init(&sync);
}

ot::ogg_reader::~ogg_reader()
{
	if (stream_ready)
		ogg_stream_clear(&stream);
	ogg_sync_clear(&sync);
}

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
	/* at this point, we've got a good page */
	if (!stream_ready) {
		if (ogg_stream_init(&stream, ogg_page_serialno(&page)) != 0)
			return {st::libogg_error, "ogg_stream_init failed"};
		stream_ready = true;
	}
	stream_in_sync = false;
	return st::ok;
}

ot::status ot::ogg_reader::read_packet()
{
	if (!stream_ready)
		return {st::stream_not_ready, "Stream was not initialized"};
	if (!stream_in_sync) {
		if (ogg_stream_pagein(&stream, &page) != 0)
			return {st::libogg_error, "ogg_stream_pagein failed"};
		stream_in_sync = true;
	}
	int rc = ogg_stream_packetout(&stream, &packet);
	if (rc == 1)
		return st::ok;
	else if (rc == 0 && ogg_stream_check(&stream) == 0)
		return {st::end_of_page, "End of page was reached"};
	else
		return {st::libogg_error, "ogg_stream_packetout failed"};
}

/**
 * \todo Make sure the page doesn't begin new packets that are continued on the following page.
 */
ot::status ot::ogg_reader::read_header_packet(const std::function<status(ogg_packet&)>& f)
{
	ot::status rc = read_packet();
	if (rc == ot::st::end_of_page)
		return {ot::st::error, "Header pages must not be empty."};
	else if (rc != ot::st::ok)
		return rc;
	if ((rc = f(packet)) != ot::st::ok)
		return rc;
	rc = read_packet();
	if (rc == ot::st::ok)
		return {ot::st::error, "Unexpected second packet in header page."};
	else if (rc != ot::st::end_of_page)
		return rc;
	return ot::st::ok;
}

ot::ogg_writer::ogg_writer(FILE* output)
	: file(output)
{
	if (ogg_stream_init(&stream, 0) != 0)
		throw std::bad_alloc();
}

ot::ogg_writer::~ogg_writer()
{
	ogg_stream_clear(&stream);
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

ot::status ot::ogg_writer::prepare_stream(long serialno)
{
	if (stream.serialno != serialno) {
		if (ogg_stream_reset_serialno(&stream, serialno) != 0)
			return {st::libogg_error, "ogg_stream_reset_serialno failed"};
	}
	return st::ok;
}

ot::status ot::ogg_writer::write_header_packet(ogg_packet& packet)
{
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
