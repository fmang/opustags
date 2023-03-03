#include <opustags.h>
#include "tap.h"

static void check_encode_base64()
{
	opaque_is(ot::encode_base64(""_bsv), u8"", "empty");
	opaque_is(ot::encode_base64("a"_bsv), u8"YQ==", "1 character");
	opaque_is(ot::encode_base64("aa"_bsv), u8"YWE=", "2 characters");
	opaque_is(ot::encode_base64("aaa"_bsv), u8"YWFh", "3 characters");
	opaque_is(ot::encode_base64("aaaa"_bsv), u8"YWFhYQ==", "4 characters");
	opaque_is(ot::encode_base64("\xFF\xFF\xFE"_bsv), u8"///+", "RFC alphabet");
	opaque_is(ot::encode_base64("\0x"_bsv), u8"AHg=", "embedded null bytes");
}

static void check_decode_base64()
{
	opaque_is(ot::decode_base64(u8""), ""_bsv, "empty");
	opaque_is(ot::decode_base64(u8"YQ=="), "a"_bsv, "1 character");
	opaque_is(ot::decode_base64(u8"YWE="), "aa"_bsv, "2 characters");
	opaque_is(ot::decode_base64(u8"YQ"), "a"_bsv, "padless 1 character");
	opaque_is(ot::decode_base64(u8"YWE"), "aa"_bsv, "padless 2 characters");
	opaque_is(ot::decode_base64(u8"YWFh"), "aaa"_bsv, "3 characters");
	opaque_is(ot::decode_base64(u8"YWFhYQ=="), "aaaa"_bsv, "4 characters");
	opaque_is(ot::decode_base64(u8"///+"), "\xFF\xFF\xFE"_bsv, "RFC alphabet");
	opaque_is(ot::decode_base64(u8"AHg="), "\0x"_bsv, "embedded null bytes");

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
