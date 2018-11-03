#include "opustags.h"

#include <cstdio>

int ot::write_page(ogg_page *og, FILE *stream)
{
	if((ssize_t) fwrite(og->header, 1, og->header_len, stream) < og->header_len)
		return -1;
	if((ssize_t) fwrite(og->body, 1, og->body_len, stream) < og->body_len)
		return -1;
	return 0;
}
