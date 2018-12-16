#include <opustags.h>
#include "tap.h"

#include <string.h>

using namespace std::literals::string_literals;

void check_read_comments()
{
	std::list<std::string> comments;
	ot::status rc;
	{
		std::string txt = "TITLE=a b c\n\nARTIST=X\nArtist=Y\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = ot::read_comments(input.get(), comments);
		if (rc != ot::st::ok)
			throw failure("could not read comments");
		auto&& expected = {"TITLE=a b c", "ARTIST=X", "Artist=Y"};
		if (!std::equal(comments.begin(), comments.end(), expected.begin(), expected.end()))
			throw failure("parsed user comments did not match expectations");
	}
	{
		std::string txt = "CORRUPTED=\xFF\xFF\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = ot::read_comments(input.get(), comments);
		if (rc != ot::st::badly_encoded)
			throw failure("did not get the expected error reading corrupted data");
	}
	{
		std::string txt = "MALFORMED\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = ot::read_comments(input.get(), comments);
		if (rc != ot::st::error)
			throw failure("did not get the expected error reading malformed comments");
	}
}

void check_good_arguments()
{
	auto parse = [](std::vector<const char*> args) {
		ot::options opt;
		ot::status rc = ot::parse_options(args.size(), const_cast<char**>(args.data()), opt);
		if (rc.code != ot::st::ok)
			throw failure("unexpected option parsing error");
		return opt;
	};

	ot::options opt;
	opt = parse({"opustags", "--help", "x", "-o", "y"});
	if (!opt.print_help)
		throw failure("did not catch --help");

	opt = parse({"opustags", "x", "--output", "y", "-D", "-s", "X=Y Z"});
	if (opt.path_in != "x" || opt.path_out != "y" || !opt.delete_all || opt.overwrite ||
	    opt.to_delete.size() != 1 || opt.to_delete[0] != "X" ||
	    opt.to_add.size() != 1 || opt.to_add[0] != "X=Y Z")
		throw failure("unexpected option parsing result for case #1");

	opt = parse({"opustags", "-S", "x", "-S", "-a", "x=y z", "-i"});
	if (opt.path_in != "x" || opt.path_out != "x" || !opt.set_all || !opt.overwrite ||
	    opt.to_delete.size() != 0 || opt.to_add.size() != 1 || opt.to_add[0] != "x=y z")
		throw failure("unexpected option parsing result for case #2");
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
	error_case({"opustags", "--delete", "X="}, "Invalid field name 'X='.", "bad field name for -d");
	error_case({"opustags", "-a", "X"}, "Comment does not contain an equal sign: X.", "bad comment for -a");
	error_case({"opustags", "--set", "X"}, "Comment does not contain an equal sign: X.", "bad comment for --set");
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
	error_case({"opustags", "-i", "-"}, "Cannot modify standard input in place.", "write stdin in-place");
	error_case({"opustags", "-o", "x", "--output", "y", "z"},
	           "Cannot specify --output more than once.", "double output");
}

int main(int argc, char **argv)
{
	std::cout << "1..3\n";
	run(check_read_comments, "check tags parsing");
	run(check_good_arguments, "check options parsing");
	run(check_bad_arguments, "check options parsing errors");
	return 0;
}
