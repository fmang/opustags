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

int main(int argc, char **argv)
{
	std::cout << "1..1\n";
	run(check_partial_files, "test partial files");
	return 0;
}
