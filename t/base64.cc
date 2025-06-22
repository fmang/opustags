#include <opustags.h>
#include "tap.h"

static void check_encode_base64()
{
	opaque_is(ot::encode_base64(""sv), u8"", "empty");
	opaque_is(ot::encode_base64("a"sv), u8"YQ==", "1 character");
	opaque_is(ot::encode_base64("aa"sv), u8"YWE=", "2 characters");
	opaque_is(ot::encode_base64("aaa"sv), u8"YWFh", "3 characters");
	opaque_is(ot::encode_base64("aaaa"sv), u8"YWFhYQ==", "4 characters");
	opaque_is(ot::encode_base64("\xFF\xFF\xFE"sv), u8"///+", "RFC alphabet");
	opaque_is(ot::encode_base64("\0x"sv), u8"AHg=", "embedded null bytes");
}

static void check_decode_base64()
{
	opaque_is(ot::decode_base64(u8""), ""sv, "empty");
	opaque_is(ot::decode_base64(u8"YQ=="), "a"sv, "1 character");
	opaque_is(ot::decode_base64(u8"YWE="), "aa"sv, "2 characters");
	opaque_is(ot::decode_base64(u8"YQ"), "a"sv, "padless 1 character");
	opaque_is(ot::decode_base64(u8"YWE"), "aa"sv, "padless 2 characters");
	opaque_is(ot::decode_base64(u8"YWFh"), "aaa"sv, "3 characters");
	opaque_is(ot::decode_base64(u8"YWFhYQ=="), "aaaa"sv, "4 characters");
	opaque_is(ot::decode_base64(u8"///+"), "\xFF\xFF\xFE"sv, "RFC alphabet");
	opaque_is(ot::decode_base64(u8"AHg="), "\0x"sv, "embedded null bytes");

	try {
		ot::decode_base64(u8"Y===");
		throw failure("accepted a bad block size");
	} catch (const ot::status& e) {
	}

	try {
		ot::decode_base64(u8"\xFF bad message!");
		throw failure("accepted an invalid character");
	} catch (const ot::status& e) {
	}
}

int main(int argc, char **argv)
{
	std::cout << "1..2\n";
	run(check_encode_base64, "base64 encoding");
	run(check_decode_base64, "base64 decoding");
	return 0;
}
