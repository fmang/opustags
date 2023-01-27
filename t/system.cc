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

void check_converter()
{
	const char* ephemere_iso = "\xc9\x70\x68\xe9\x6d\xe8\x72\x65";
	ot::encoding_converter to_utf8("ISO_8859-1", "UTF-8");
	ot::encoding_converter from_utf8("UTF-8", "ISO_8859-1");

	is(to_utf8(ephemere_iso), "Éphémère", "conversion to UTF-8 is correct");
	is(from_utf8("Éphémère"), ephemere_iso, "conversion from UTF-8 is correct");

	try {
		from_utf8("\xFF\xFF");
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
	plan(3);
	run(check_partial_files, "test partial files");
	run(check_converter, "test encoding converter");
	run(check_shell_esape, "test shell escaping");
	return 0;
}
