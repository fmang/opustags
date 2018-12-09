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
		if (bad_tmp.open("/dev/null") != ot::st::standard_error)
			throw failure("cannot open a device as a partial file");
		if (bad_tmp.open(result) != ot::st::ok)
			throw failure("could not open a simple result file");
		name = bad_tmp.name();
		if (name.size() != strlen(result) + 12 ||
		    name.compare(0, strlen(result), result) != 0)
			throw failure("the temporary name is surprising: " + name);
	}
	if (access(name.c_str(), F_OK) != -1)
		throw failure("the bad temporary file was not deleted");

	ot::partial_file good_tmp;
	if (good_tmp.open(result) != ot::st::ok)
		throw failure("could not open the result file");
	name = good_tmp.name();
	if (good_tmp.commit() != ot::st::ok)
		throw failure("could not commit the result file");
	if (access(name.c_str(), F_OK) != -1)
		throw failure("the good temporary file was not deleted");
	if (access(result, F_OK) != 0)
		throw failure("the final result file is not there");
	if (remove(result) != 0)
		throw failure("could not remove the result file");
}

void check_converter()
{
	const char* ephemere_iso = "\xc9\x70\x68\xe9\x6d\xe8\x72\x65";
	ot::encoding_converter to_utf8("ISO-8859-1", "UTF-8");
	std::string out;
	ot::status rc = to_utf8(ephemere_iso, out);
	if (rc != ot::st::ok || out != "Éphémère")
		throw failure("conversion to UTF-8 should have worked");

	ot::encoding_converter from_utf8("UTF-8", "ISO-8859-1//TRANSLIT");
	rc = from_utf8("Éphémère", out);
	if (rc != ot::st::ok || out != ephemere_iso)
		throw failure("conversion from UTF-8 should have worked");
	rc = from_utf8("\xFF\xFF", out);
	if (rc != ot::st::badly_encoded)
		throw failure("conversion from bad UTF-8 should have failed");
	rc = from_utf8("cat 猫 chat", out);
	if (rc != ot::st::information_lost || out != "cat ? chat")
		throw failure("lossy conversion from UTF-8 should have worked");
}

int main(int argc, char **argv)
{
	std::cout << "1..2\n";
	run(check_partial_files, "test partial files");
	run(check_converter, "test encoding converter");
	return 0;
}
