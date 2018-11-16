#include "opustags.h"

#include <cstdio>

ot::ogg_reader::ogg_reader(FILE* input)
	: file(input)
{
	ogg_sync_init(&sync);
	memset(&stream, 0, sizeof(stream));
}

ot::ogg_reader::~ogg_reader()
{
	ogg_sync_clear(&sync);
	ogg_stream_clear(&stream);
}

ot::status ot::ogg_reader::read_page()
{
	while (ogg_sync_pageout(&sync, &page) != 1) {
		if (feof(file))
			return status::end_of_file;
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
	return status::ok;
}

ot::ogg_writer::ogg_writer(FILE* output)
	: file(output)
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
