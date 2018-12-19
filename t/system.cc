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
		is(bad_tmp.open("/dev/null"), ot::st::standard_error,
		   "opening a device as a partial file fails");
		is(bad_tmp.open(result), ot::st::ok,
		   "opening a regular partial file works");
		name = bad_tmp.name();
		if (name.size() != strlen(result) + 12 ||
		    name.compare(0, strlen(result), result) != 0)
			throw failure("the temporary name is surprising: " + name);
	}
	is(access(name.c_str(), F_OK), -1, "expect the temporary file is deleted");

	ot::partial_file good_tmp;
	is(good_tmp.open(result), ot::st::ok, "open the partial file");
	name = good_tmp.name();
	is(good_tmp.commit(), ot::st::ok, "commit the result file");
	is(access(name.c_str(), F_OK), -1, "expect the temporary file is deleted");
	is(access(result, F_OK), 0, "expect the final result file");
	is(remove(result), 0, "remove the result file");
}

void check_converter()
{
	const char* ephemere_iso = "\xc9\x70\x68\xe9\x6d\xe8\x72\x65";
	ot::encoding_converter to_utf8("ISO-8859-1", "UTF-8");
	ot::encoding_converter from_utf8("UTF-8", "ISO-8859-1//TRANSLIT");
	std::string out;

	ot::status rc = to_utf8(ephemere_iso, out);
	is(rc, ot::st::ok, "conversion to UTF-8 is successful");
	is(out, "Éphémère", "conversion to UTF-8 is correct");

	rc = from_utf8("Éphémère", out);
	is(rc, ot::st::ok, "conversion from UTF-8 is successful");
	is(out, ephemere_iso, "conversion from UTF-8 is correct");

	rc = from_utf8("\xFF\xFF", out);
	is(rc, ot::st::badly_encoded, "conversion from bad UTF-8 fails");

	rc = from_utf8("cat 猫 chat", out);
	is(rc, ot::st::information_lost, "lossy conversion from UTF-8 is detected");
	is(out, "cat ? chat", "check result of lossy conversion");
}

int main(int argc, char **argv)
{
	plan(2);
	run(check_partial_files, "test partial files");
	run(check_converter, "test encoding converter");
	return 0;
}
