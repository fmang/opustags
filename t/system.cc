#include <opustags.h>
#include "tap.h"

#include <string.h>
#include <unistd.h>

void check_partial_files()
{
	static const char* result = "partial_file.test";
	std::string name;
	{
		ot::partial_file bad_tmp;
		try {
			bad_tmp.open("/dev/null");
			throw failure("opening a device as a partial file should fail");
		} catch (const ot::status& rc) {
			is(rc, ot::st::standard_error, "opening a device as a partial file fails");
		}

		bad_tmp.open(result);
		name = bad_tmp.name();
		if (name.size() != strlen(result) + 12 ||
		    name.compare(0, strlen(result), result) != 0)
			throw failure("the temporary name is surprising: " + name);
	}
	is(access(name.c_str(), F_OK), -1, "expect the temporary file is deleted");

	ot::partial_file good_tmp;
	good_tmp.open(result);
	name = good_tmp.name();
	good_tmp.commit();
	is(access(name.c_str(), F_OK), -1, "expect the temporary file is deleted");
	is(access(result, F_OK), 0, "expect the final result file");
	is(remove(result), 0, "remove the result file");
}

void check_slurp()
{
	static const ot::byte_string_view pixel = ""_bsv
		"\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d"
		"\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01"
		"\x08\x02\x00\x00\x00\x90\x77\x53\xde\x00\x00\x00"
		"\x0c\x49\x44\x41\x54\x08\xd7\x63\xf8\xff\xff\x3f"
		"\x00\x05\xfe\x02\xfe\xdc\xcc\x59\xe7\x00\x00\x00"
		"\x00\x49\x45\x4e\x44\xae\x42\x60\x82";
	opaque_is(ot::slurp_binary_file("pixel.png"), pixel, "loads a whole file");
}

void check_converter()
{
	is(ot::decode_utf8(ot::encode_utf8("Éphémère")), "Éphémère", "decode_utf8 reverts encode_utf8");
	opaque_is(ot::encode_utf8(ot::decode_utf8(u8"Éphémère")), u8"Éphémère",
	          "encode_utf8 reverts decode_utf8");

	try {
		ot::decode_utf8((char8_t*) "\xFF\xFF");
		throw failure("conversion from bad UTF-8 did not fail");
	} catch (const ot::status&) {}
}

void check_shell_esape()
{
	is(ot::shell_escape("foo"), "'foo'", "simple string");
	is(ot::shell_escape("a'b"), "'a'\\''b'", "string with a simple quote");
	is(ot::shell_escape("a!b"), "'a'\\!'b'", "string with a bang");
	is(ot::shell_escape("a!b'c!d'e"), "'a'\\!'b'\\''c'\\!'d'\\''e'", "string with a bang");
}

int main(int argc, char **argv)
{
	plan(4);
	run(check_partial_files, "test partial files");
	run(check_slurp, "file slurping");
	run(check_converter, "test encoding converter");
	run(check_shell_esape, "test shell escaping");
	return 0;
}
