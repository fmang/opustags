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

void check_bad_arguments()
{
	auto error_case = [](std::vector<const char*> args, const char* message, const std::string& name) {
		ot::options opt;
		ot::status rc = ot::parse_options(args.size(), const_cast<char**>(args.data()), opt);
		if (rc.code != ot::st::bad_arguments)
			throw failure("bad error code for case " + name);
		if (rc.message != message)
			throw failure("bad error message for case " + name + ", got: " + rc.message);
	};
	error_case({"opustags"}, "No arguments specified. Use -h for help.", "no arguments");
	error_case({"opustags", "--output", ""}, "Output file path cannot be empty.", "empty output path");
	error_case({"opustags", "--in-place="}, "In-place suffix cannot be empty.", "empty in-place suffix");
	error_case({"opustags", "--delete", "X="}, "Invalid field name 'X='.", "bad field name for -d");
	error_case({"opustags", "-a", "X"}, "Invalid comment 'X'.", "bad comment for -a");
	error_case({"opustags", "--set", "X"}, "Invalid comment 'X'.", "bad comment for --set");
	error_case({"opustags", "-a"}, "Missing value for option '-a'.", "short option with missing value");
	error_case({"opustags", "--add"}, "Missing value for option '--add'.", "long option with missing value");
	error_case({"opustags", "-x"}, "Unrecognized option '-x'.", "unrecognized short option");
	error_case({"opustags", "--derp"}, "Unrecognized option '--derp'.", "unrecognized long option");
	error_case({"opustags", "-x=y"}, "Unrecognized option '-x'.", "unrecognized short option with value");
	error_case({"opustags", "--derp=y"}, "Unrecognized option '--derp=y'.", "unrecognized long option with value");
	error_case({"opustags", "-aX=Y"}, "Exactly one input file must be specified.", "no input file");
	error_case({"opustags", ""}, "Input file path cannot be empty.", "empty input file path");
	error_case({"opustags", "-i", "-o", "/dev/null", "-"}, "Cannot combine --in-place and --output.", "in-place + output");
	error_case({"opustags", "-S", "-"}, "Cannot use standard input as input file when --set-all is specified.",
	                                    "set all and read opus from stdin");
	error_case({"opustags", "-i", "-"}, "Cannot modify standard input in-place.", "write stdin in-place");
	error_case({"opustags", "-o", "x", "--output", "y", "z"},
	           "Cannot specify --output more than once.", "double output");
	error_case({"opustags", "-i", "-i", "z"}, "Cannot specify --in-place more than once.", "double in-place");
}

int main(int argc, char **argv)
{
	std::cout << "1..2\n";
	run(check_read_comments, "check tags parsing");
	run(check_bad_arguments, "check options parsing errors");
	return 0;
}
