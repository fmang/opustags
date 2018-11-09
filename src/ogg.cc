#include "opustags.h"

#include <cstdio>

ot::ogg_reader::ogg_reader()
{
	ogg_sync_init(&sync);
	memset(&stream, 0, sizeof(stream));
}

ot::ogg_reader::~ogg_reader()
{
	ogg_sync_clear(&sync);
	ogg_stream_clear(&stream);
}

ot::ogg_writer::ogg_writer()
{
	memset(&stream, 0, sizeof(stream));
}

ot::ogg_writer::~ogg_writer()
{
	ogg_stream_clear(&stream);
}

int ot::write_page(ogg_page *og, FILE *stream)
{
	if((ssize_t) fwrite(og->header, 1, og->header_len, stream) < og->header_len)
		return -1;
	if((ssize_t) fwrite(og->body, 1, og->body_len, stream) < og->body_len)
		return -1;
	return 0;
}
