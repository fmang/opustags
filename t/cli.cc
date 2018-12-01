#include <opustags.h>
#include "tap.h"

#include <string.h>

const char *user_comments = R"raw(
TITLE=a b c

ARTIST=X
Artist=Y)raw";

void check_read_comments()
{
	ot::file input = fmemopen(const_cast<char*>(user_comments), strlen(user_comments), "r");
	auto comments = ot::read_comments(input.get());
	auto&& expected = {"TITLE=a b c", "ARTIST=X", "Artist=Y"};
	if (!std::equal(comments.begin(), comments.end(), expected.begin(), expected.end()))
		throw failure("parsed user comments did not match expectations");
}

void check_parse_options()
{
	auto error_case = [](std::vector<const char*> args, const char* message, const std::string& name) {
		ot::options opt;
		ot::status rc = parse_options(args.size(), const_cast<char**>(args.data()), opt);
		if (rc != ot::st::bad_arguments)
			throw failure("bad error code when parsing " + name);
		if (rc.message != message)
			throw failure("bad error message when parsing " + name);
	};
	error_case({"opustags", "-a"}, "Missing value for option '-a'.", "short option with missing value");
	error_case({"opustags", "--add"}, "Missing value for option '--add'.", "long option with missing value");
	error_case({"opustags", "-x"}, "Unrecognized option '-x'.", "unrecognized short option");
	error_case({"opustags", "--derp"}, "Unrecognized option '--derp'.", "unrecognized long option");
	error_case({"opustags", "-x=y"}, "Unrecognized option '-x'.", "unrecognized short option with value");
	error_case({"opustags", "--derp=y"}, "Unrecognized option '--derp=y'.", "unrecognized long option with value");
}

int main(int argc, char **argv)
{
	std::cout << "1..2\n";
	run(check_read_comments, "check tags parsing");
	run(check_parse_options, "check options parsing");
	return 0;
}
