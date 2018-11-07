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

int ot::parse_tags(const char *data, long len, opus_tags *tags)
{
	long pos;
	if (len < 8+4+4)
		return -1;
	if (strncmp(data, "OpusTags", 8) != 0)
		return -1;
	// Vendor
	pos = 8;
	tags->vendor = ot::string_view(data + pos + 4, le32toh(*((uint32_t*) (data + pos))));
	pos += 4 + tags->vendor.size();
	if (pos + 4 > len)
		return -1;
	// Count
	uint32_t count = le32toh(*((uint32_t*) (data + pos)));
	pos += 4;
	// Comments
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t comment_length = le32toh(*((uint32_t*) (data + pos)));
		const char *comment_value = data + pos + 4;
		tags->comments.emplace_back(comment_value, comment_length);
		pos += 4 + comment_length;
		if (pos > len)
			return -1;
	}
	// Extra data
	tags->extra_data = ot::string_view(data + pos, static_cast<size_t>(len - pos));
	return 0;
}

int ot::render_tags(opus_tags *tags, ogg_packet *op)
{
	// Note: op->packet must be manually freed.
	op->b_o_s = 0;
	op->e_o_s = 0;
	op->granulepos = 0;
	op->packetno = 1;
	long len = 8 + 4 + tags->vendor.size() + 4;
	for (const string_view &comment : tags->comments)
		len += 4 + comment.size();
	len += tags->extra_data.size();
	op->bytes = len;
	char *data = static_cast<char*>(malloc(len));
	if (!data)
		return -1;
	op->packet = (unsigned char*) data;
	uint32_t n;
	memcpy(data, "OpusTags", 8);
	n = htole32(tags->vendor.size());
	memcpy(data+8, &n, 4);
	memcpy(data+12, tags->vendor.data(), tags->vendor.size());
	data += 12 + tags->vendor.size();
	n = htole32(tags->comments.size());
	memcpy(data, &n, 4);
	data += 4;
	for (const string_view &comment : tags->comments) {
		n = htole32(comment.size());
		memcpy(data, &n, 4);
		memcpy(data+4, comment.data(), comment.size());
		data += 4 + comment.size();
	}
	memcpy(data, tags->extra_data.data(), tags->extra_data.size());
	return 0;
}

/**
 * \todo Make the field name case-insensitive?
 */
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
	auto it = tags->comments.begin(), end = tags->comments.end();
	while (it != end) {
		auto current = it++;
		if (match_field(current->data(), current->size(), field))
			tags->comments.erase(current);
	}
}

/**
 * \todo Return void.
 */
int ot::add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i)
		tags->comments.emplace_back(tags_to_add[i]);
	return 0;
}

void ot::print_tags(opus_tags *tags)
{
	for (const string_view &comment : tags->comments) {
		fwrite(comment.data(), 1, comment.size(), stdout);
		puts("");
	}
}
