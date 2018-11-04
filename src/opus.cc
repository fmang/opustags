/**
 * \file src/opus.cc
 * \ingroup opus
 *
 * The way Opus is encapsulated into an Ogg stream, and the content of the packets we're dealing
 * with here is defined by [RFC 7584](https://tools.ietf.org/html/rfc7845.html).
 *
 * Section 3 "Packet Organization" is critical for us:
 *
 * - The first page contains exactly 1 packet, the OpusHead, and it contains it entirely.
 * - The second page begins the OpusTags packet, which may span several pages.
 * - The OpusTags packet must finish the page on which it completes.
 *
 * The structure of the OpusTags packet is defined in section 5.2 "Comment Header" of the RFC.
 *
 * OpusTags is similar to [Vorbis Comment](https://www.xiph.org/vorbis/doc/v-comment.html), which
 * gives us some context, but let's stick to the RFC for the technical details.
 *
 * \todo Validate that the vendor string and comments are valid UTF-8.
 * \todo Validate that field names are ASCII: 0x20 through 0x7D, 0x3D ('=') excluded.
 * \todo Field names are case insensitive, respect that.
 *
 */

#include "opustags.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#endif

int ot::parse_tags(char *data, long len, opus_tags *tags)
{
	long pos;
	if (len < 8+4+4)
		return -1;
	if (strncmp(data, "OpusTags", 8) != 0)
		return -1;
	// Vendor
	pos = 8;
	tags->vendor_length = le32toh(*((uint32_t*) (data + pos)));
	tags->vendor_string = data + pos + 4;
	pos += 4 + tags->vendor_length;
	if (pos + 4 > len)
		return -1;
	// Count
	tags->count = le32toh(*((uint32_t*) (data + pos)));
	if (tags->count == 0)
		return 0;
	tags->lengths = static_cast<uint32_t*>(calloc(tags->count, sizeof(uint32_t)));
	if (tags->lengths == NULL)
		return -1;
	tags->comment = static_cast<const char**>(calloc(tags->count, sizeof(char*)));
	if (tags->comment == NULL) {
		free(tags->lengths);
		return -1;
	}
	pos += 4;
	// Comment
	uint32_t i;
	for (i=0; i<tags->count; i++) {
		tags->lengths[i] = le32toh(*((uint32_t*) (data + pos)));
		tags->comment[i] = data + pos + 4;
		pos += 4 + tags->lengths[i];
		if (pos > len)
			return -1;
	}

	if (pos < len)
		fprintf(stderr, "warning: %ld unused bytes at the end of the OpusTags packet\n", len - pos);

	return 0;
}

int ot::render_tags(opus_tags *tags, ogg_packet *op)
{
	// Note: op->packet must be manually freed.
	op->b_o_s = 0;
	op->e_o_s = 0;
	op->granulepos = 0;
	op->packetno = 1;
	long len = 8 + 4 + tags->vendor_length + 4;
	uint32_t i;
	for (i=0; i<tags->count; i++)
		len += 4 + tags->lengths[i];
	op->bytes = len;
	char *data = static_cast<char*>(malloc(len));
	if (!data)
		return -1;
	op->packet = (unsigned char*) data;
	uint32_t n;
	memcpy(data, "OpusTags", 8);
	n = htole32(tags->vendor_length);
	memcpy(data+8, &n, 4);
	memcpy(data+12, tags->vendor_string, tags->vendor_length);
	data += 12 + tags->vendor_length;
	n = htole32(tags->count);
	memcpy(data, &n, 4);
	data += 4;
	for (i=0; i<tags->count; i++) {
		n = htole32(tags->lengths[i]);
		memcpy(data, &n, 4);
		memcpy(data+4, tags->comment[i], tags->lengths[i]);
		data += 4 + tags->lengths[i];
	}
	return 0;
}

static int match_field(const char *comment, uint32_t len, const char *field)
{
	size_t field_len;
	for (field_len = 0; field[field_len] != '\0' && field[field_len] != '='; field_len++);
	if (len <= field_len)
		return 0;
	if (comment[field_len] != '=')
		return 0;
	if (strncmp(comment, field, field_len) != 0)
		return 0;
	return 1;
}

void ot::delete_tags(opus_tags *tags, const char *field)
{
	uint32_t i;
	for (i=0; i<tags->count; i++) {
		if (match_field(tags->comment[i], tags->lengths[i], field)) {
			// We want to delete the current element, so we move the last tag at
			// position i, then decrease the array size. We need decrease i to inspect
			// at the next iteration the tag we just moved.
			tags->count--;
			tags->lengths[i] = tags->lengths[tags->count];
			tags->comment[i] = tags->comment[tags->count];
			--i;
			// No need to resize the arrays.
		}
	}
}

int ot::add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count)
{
	if (count == 0)
		return 0;
	uint32_t *lengths = static_cast<uint32_t*>(realloc(tags->lengths, (tags->count + count) * sizeof(uint32_t)));
	const char **comment = static_cast<const char**>(realloc(tags->comment, (tags->count + count) * sizeof(char*)));
	if (lengths == NULL || comment == NULL)
		return -1;
	tags->lengths = lengths;
	tags->comment = comment;
	uint32_t i;
	for (i=0; i<count; i++) {
		tags->lengths[tags->count + i] = strlen(tags_to_add[i]);
		tags->comment[tags->count + i] = tags_to_add[i];
	}
	tags->count += count;
	return 0;
}

void ot::print_tags(opus_tags *tags)
{
	for (uint32_t i=0; i<tags->count; i++) {
		fwrite(tags->comment[i], 1, tags->lengths[i], stdout);
		puts("");
	}
}

void ot::free_tags(opus_tags *tags)
{
	if (tags->count > 0) {
		free(tags->lengths);
		free(tags->comment);
	}
}
