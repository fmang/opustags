#include <opustags.h>
#include "tap.h"

static void check_encode_base64()
{
	is(ot::encode_base64(""), "", "empty");
	is(ot::encode_base64("a"), "YQ==", "1 character");
	is(ot::encode_base64("aa"), "YWE=", "2 characters");
	is(ot::encode_base64("aaa"), "YWFh", "3 characters");
	is(ot::encode_base64("aaaa"), "YWFhYQ==", "4 characters");
	is(ot::encode_base64("\xFF\xFF\xFE"), "///+", "RFC alphabet");
	is(ot::encode_base64("\0x"sv), "AHg=", "embedded null bytes");
}

static void check_decode_base64()
{
	is(ot::decode_base64(""), "", "empty");
	is(ot::decode_base64("YQ=="), "a", "1 character");
	is(ot::decode_base64("YWE="), "aa", "2 characters");
	is(ot::decode_base64("YQ"), "a", "padless 1 character");
	is(ot::decode_base64("YWE"), "aa", "padless 2 characters");
	is(ot::decode_base64("YWFh"), "aaa", "3 characters");
	is(ot::decode_base64("YWFhYQ=="), "aaaa", "4 characters");
	is(ot::decode_base64("///+"), "\xFF\xFF\xFE", "RFC alphabet");
	is(ot::decode_base64("AHg="), "\0x"sv, "embedded null bytes");

	try {
		ot::decode_base64("Y===");
		throw failure("accepted a bad block size");
	} catch (const ot::status& e) {
	}

	try {
		ot::decode_base64("\xFF bad message!");
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
