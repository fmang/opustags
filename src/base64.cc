/**
 * \file src/base64.cc
 * \brief Base64 encoding/decoding (RFC 4648).
 *
 * Inspired by Jouni Malinen’s BSD implementation at
 * <http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c>.
 *
 * This implementation is used to decode the cover arts embedded in the tags. According to
 * <https://wiki.xiph.org/VorbisComment>, line feeds are not allowed and padding is required.
 */

#include <opustags.h>

#include <cstring>

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string ot::encode_base64(std::string_view src)
{
	size_t len = src.size();
	size_t num_blocks = (len + 2) / 3; // Count of 3-byte blocks, rounded up.
	size_t olen = num_blocks * 4; // Each 3-byte block becomes 4 base64 bytes.
	if (olen < len)
		throw std::overflow_error("failed to encode excessively long base64 block");

	std::string out;
	out.resize(olen);

	const unsigned char* in = reinterpret_cast<const unsigned char*>(src.data());
	const unsigned char* end = in + len;
	unsigned char* pos = reinterpret_cast<unsigned char*>(out.data());
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else { // end - in == 2
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	return out;
}

std::string ot::decode_base64(std::string_view src)
{
	// Remove the padding and rely on the string length instead.
	while (src.back() == '=')
		src.remove_suffix(1);

	size_t olen = src.size() / 4 * 3; // Whole blocks;
	switch (src.size() % 4) {
		case 1: throw status {st::error, "invalid base64 block size"};
		case 2: olen += 1; break;
		case 3: olen += 2; break;
	}

	std::string out;
	out.resize(olen);
	unsigned char* pos = reinterpret_cast<unsigned char*>(out.data());

	unsigned char dtable[256];
	memset(dtable, 0x80, 256);
	for (size_t i = 0; i < sizeof(base64_table) - 1; ++i)
		dtable[base64_table[i]] = (unsigned char) i;

	unsigned char block[4];
	size_t count = 0;
	for (unsigned char c : src) {
		unsigned char tmp = dtable[c];
		if (tmp == 0x80)
			throw status {st::error, "invalid base64 character"};

		block[count++] = tmp;
		if (count == 2) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
		} else if (count == 3) {
			*pos++ = (block[1] << 4) | (block[2] >> 2);
		} else if (count == 4) {
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
		}
	}

	return out;
}
