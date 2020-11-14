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
 *
 */

#include <opustags.h>

#include <string.h>

#ifdef HAVE_ENDIAN_H
#  include <endian.h>
#endif

#ifdef HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
#endif

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#endif

ot::status ot::parse_tags(const ogg_packet& packet, opus_tags& tags)
{
	if (packet.bytes < 0)
		return {st::int_overflow, "Overflowing comment header length"};
	size_t size = static_cast<size_t>(packet.bytes);
	const char* data = reinterpret_cast<char*>(packet.packet);
	size_t pos = 0;
	opus_tags my_tags;

	// Magic number
	if (8 > size)
		return {st::cut_magic_number, "Comment header too short for the magic number"};
	if (memcmp(data, "OpusTags", 8) != 0)
		return {st::bad_magic_number, "Comment header did not start with OpusTags"};

	// Vendor
	pos = 8;
	if (pos + 4 > size)
		return {st::cut_vendor_length,
		        "Vendor string length did not fit the comment header"};
	size_t vendor_length = le32toh(*((uint32_t*) (data + pos)));
	if (pos + 4 + vendor_length > size)
		return {st::cut_vendor_data, "Vendor string did not fit the comment header"};
	my_tags.vendor = std::string(data + pos + 4, vendor_length);
	pos += 4 + my_tags.vendor.size();

	// Comment count
	if (pos + 4 > size)
		return {st::cut_comment_count, "Comment count did not fit the comment header"};
	uint32_t count = le32toh(*((uint32_t*) (data + pos)));
	pos += 4;

	// Comments' data
	for (uint32_t i = 0; i < count; ++i) {
		if (pos + 4 > size)
			return {st::cut_comment_length,
			        "Comment length did not fit the comment header"};
		uint32_t comment_length = le32toh(*((uint32_t*) (data + pos)));
		if (pos + 4 + comment_length > size)
			return {st::cut_comment_data,
			        "Comment string did not fit the comment header"};
		const char *comment_value = data + pos + 4;
		my_tags.comments.emplace_back(comment_value, comment_length);
		pos += 4 + comment_length;
	}

	// Extra data
	my_tags.extra_data = std::string(data + pos, size - pos);

	tags = std::move(my_tags);
	return st::ok;
}

ot::dynamic_ogg_packet ot::render_tags(const opus_tags& tags)
{
	size_t size = 8 + 4 + tags.vendor.size() + 4;
	for (const std::string& comment : tags.comments)
		size += 4 + comment.size();
	size += tags.extra_data.size();

	dynamic_ogg_packet op(size);
	op.b_o_s = 0;
	op.e_o_s = 0;
	op.granulepos = 0;
	op.packetno = 1;

	unsigned char* data = op.packet;
	uint32_t n;
	memcpy(data, "OpusTags", 8);
	n = htole32(tags.vendor.size());
	memcpy(data+8, &n, 4);
	memcpy(data+12, tags.vendor.data(), tags.vendor.size());
	data += 12 + tags.vendor.size();
	n = htole32(tags.comments.size());
	memcpy(data, &n, 4);
	data += 4;
	for (const std::string& comment : tags.comments) {
		n = htole32(comment.size());
		memcpy(data, &n, 4);
		memcpy(data+4, comment.data(), comment.size());
		data += 4 + comment.size();
	}
	memcpy(data, tags.extra_data.data(), tags.extra_data.size());

	return op;
}
