#include "opustags.h"

#include <cstdio>

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
			return status::end_of_stream;
		char* buf = ogg_sync_buffer(&sync, 65536);
		if (buf == nullptr)
			return status::libogg_error;
		size_t len = fread(buf, 1, 65536, file);
		if (ferror(file))
			return status::standard_error;
		if (ogg_sync_wrote(&sync, len) != 0)
			return status::libogg_error;
		if (ogg_sync_check(&sync) != 0)
			return status::libogg_error;
	}
	/* at this point, we've got a good page */
	if (!stream_ready) {
		if (ogg_stream_init(&stream, ogg_page_serialno(&page)) != 0)
			return status::libogg_error;
		stream_ready = true;
	}
	stream_in_sync = false;
	return status::ok;
}

ot::status ot::ogg_reader::read_packet()
{
	if (!stream_ready)
		return status::stream_not_ready;
	if (!stream_in_sync) {
		if (ogg_stream_pagein(&stream, &page) != 0)
			return status::libogg_error;
		stream_in_sync = true;
	}
	int rc = ogg_stream_packetout(&stream, &packet);
	if (rc == 1)
		return status::ok;
	else if (rc == 0 && ogg_stream_check(&stream) == 0)
		return status::end_of_page;
	else
		return status::libogg_error;
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
		return status::int_overflow;
	auto header_len = static_cast<size_t>(page.header_len);
	auto body_len = static_cast<size_t>(page.body_len);
	if (fwrite(page.header, 1, header_len, file) < header_len)
		return status::standard_error;
	if (fwrite(page.body, 1, body_len, file) < body_len)
		return status::standard_error;
	return status::ok;
}

ot::status ot::ogg_writer::prepare_stream(long serialno)
{
	if (stream.serialno != serialno) {
		if (ogg_stream_reset_serialno(&stream, serialno) != 0)
			return status::libogg_error;
	}
	return status::ok;
}

ot::status ot::ogg_writer::write_packet(const ogg_packet& packet)
{
	if (ogg_stream_packetin(&stream, const_cast<ogg_packet*>(&packet)) != 0)
		return status::libogg_error;
	else
		return status::ok;
}

ot::status ot::ogg_writer::flush_page()
{
	ogg_page page;
	if (ogg_stream_flush(&stream, &page) != 0)
		return write_page(page);
	if (ogg_stream_check(&stream) != 0)
		return status::libogg_error;
	return status::ok; /* nothing was done */
}
