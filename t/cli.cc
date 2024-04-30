#include <opustags.h>
#include "tap.h"

#include <string.h>

static ot::status read_comments(FILE* input, std::list<std::u8string>& comments, bool raw)
{
	ot::options opt;
	opt.raw = raw;
	try {
		comments = ot::read_comments(input, opt);
	} catch (const ot::status& rc) {
		return rc;
	}
	return ot::st::ok;
}

void check_read_comments()
{
	std::list<std::u8string> comments;
	ot::status rc;
	{
		std::string txt = "TITLE=a b c\n\nARTIST=X\nArtist=Y\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = read_comments(input.get(), comments, false);
		if (rc != ot::st::ok)
			throw failure("could not read comments");
		auto&& expected = {u8"TITLE=a b c", u8"ARTIST=X", u8"Artist=Y"};
		if (!std::equal(comments.begin(), comments.end(), expected.begin(), expected.end()))
			throw failure("parsed user comments did not match expectations");
	}
	{
		std::string txt = "CORRUPTED=\xFF\xFF\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = read_comments(input.get(), comments, false);
		if (rc != ot::st::badly_encoded)
			throw failure("did not get the expected error reading corrupted data");
	}
	{
		std::string txt = "RAW=\xFF\xFF\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = read_comments(input.get(), comments, true);
		if (rc != ot::st::ok)
			throw failure("could not read comments");
		if (comments.front() != (char8_t*) "RAW=\xFF\xFF")
			throw failure("parsed user comments did not match expectations");
	}
	{
		std::string txt = "MULTILINE=First\n\tSecond\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = read_comments(input.get(), comments, true);
		if (rc != ot::st::ok)
			throw failure("could not read comments");
		if (comments.front() != u8"MULTILINE=First\nSecond")
			throw failure("parsed user comments did not match expectations");
	}
	{
		std::string txt = "MALFORMED\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = read_comments(input.get(), comments, false);
		if (rc != ot::st::error)
			throw failure("did not get the expected error reading malformed comments");
	}
	{
		std::string txt = "\tBad"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		rc = read_comments(input.get(), comments, true);
		if (rc != ot::st::error)
			throw failure("did not get the expected error reading bad continuation line");
	}
}

/**
 * Wrap #ot::parse_options with a higher-level interface much more convenient for testing.
 * In practice, the argc/argv combo are enough though for the current state of opustags.
 */
static ot::status parse_options(const std::vector<const char*>& args, ot::options& opt, FILE *comments)
{
	int argc = args.size();
	char* argv[argc];
	for (int i = 0; i < argc; ++i)
		argv[i] = strdup(args[i]);
	ot::status rc = ot::st::ok;
	try {
		opt = ot::parse_options(argc, argv, comments);
	} catch (const ot::status& e) {
		rc = e;
	}
	for (int i = 0; i < argc; ++i)
		free(argv[i]);
	return rc;
}

void check_good_arguments()
{
	auto parse = [](std::vector<const char*> args) {
		ot::options opt;
		std::string txt = "N=1\n"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		ot::status rc = parse_options(args, opt, input.get());
		if (rc.code != ot::st::ok)
			throw failure("unexpected option parsing error");
		return opt;
	};

	ot::options opt;
	opt = parse({"opustags", "--help", "x", "-o", "y"});
	if (!opt.print_help)
		throw failure("did not catch --help");

	opt = parse({"opustags", "x", "--output", "y", "-D", "-s", "X=Y Z", "-d", "a=b"});
	if (opt.paths_in.size() != 1 || opt.paths_in.front() != "x" || !opt.path_out ||
	    opt.path_out != "y" || !opt.delete_all || opt.overwrite || opt.to_delete.size() != 2 ||
	    opt.to_delete.front() != u8"X" || *std::next(opt.to_delete.begin()) != u8"a=b" ||
	    opt.to_add != std::list<std::u8string>{ u8"X=Y Z" })
		throw failure("unexpected option parsing result for case #1");

	opt = parse({"opustags", "-S", "x", "-S", "-a", "x=y z", "-i"});
	if (opt.paths_in.size() != 1 || opt.paths_in.front() != "x" || opt.path_out ||
	    !opt.overwrite || opt.to_delete.size() != 0 ||
	    opt.to_add != std::list<std::u8string>{ u8"N=1", u8"x=y z" })
		throw failure("unexpected option parsing result for case #2");

	opt = parse({"opustags", "-i", "x", "y", "z"});
	if (opt.paths_in.size() != 3 || opt.paths_in[0] != "x" || opt.paths_in[1] != "y" ||
	    opt.paths_in[2] != "z" || !opt.overwrite || !opt.in_place)
		throw failure("unexpected option parsing result for case #3");

	opt = parse({"opustags", "-ie", "x"});
	if (opt.paths_in.size() != 1 || opt.paths_in[0] != "x" ||
	    !opt.edit_interactively || !opt.overwrite || !opt.in_place)
		throw failure("unexpected option parsing result for case #4");

	opt = parse({"opustags", "-a", "X=\xFF", "--raw", "x"});
	if (!opt.raw || opt.to_add.front() != u8"X=\xFF")
		throw failure("--raw did not disable transcoding");
}

void check_bad_arguments()
{
	auto error_code_case = [](std::vector<const char*> args, const char* message, ot::st error_code, const std::string& name) {
		ot::options opt;
		std::string txt = "N=1\nINVALID"s;
		ot::file input = fmemopen((char*) txt.data(), txt.size(), "r");
		ot::status rc = parse_options(args, opt, input.get());
		if (rc.code != error_code)
			throw failure("bad error code for case " + name);
		if (!rc.message.starts_with(message))
			throw failure("bad error message for case " + name + ", got: " + rc.message);
	};
	auto error_case = [&error_code_case](std::vector<const char*> args, const char* message, const std::string& name) {
		 error_code_case(args, message, ot::st::bad_arguments, name);
	};
	error_case({"opustags"}, "No arguments specified. Use -h for help.", "no arguments");
	error_case({"opustags", "-a", "X"}, "Comment does not contain an equal sign: X.", "bad comment for -a");
	error_case({"opustags", "--set", "X"}, "Comment does not contain an equal sign: X.", "bad comment for --set");
	error_case({"opustags", "-a"}, "Missing value for option '-a'.", "short option with missing value");
	error_case({"opustags", "-x"}, "Unrecognized option '-x'.", "unrecognized short option");
	error_case({"opustags", "--derp"}, "Unrecognized option '--derp'.", "unrecognized long option");
	error_case({"opustags", "-x=y"}, "Unrecognized option '-x'.", "unrecognized short option with value");
	error_case({"opustags", "--derp=y"}, "Unrecognized option '--derp=y'.", "unrecognized long option with value");
	error_case({"opustags", "-aX=Y"}, "Exactly one input file must be specified.", "no input file");
	error_case({"opustags", "-i", "-o", "/dev/null", "-"}, "Cannot combine --in-place and --output.", "in-place + output");
	error_case({"opustags", "-S", "-"}, "Cannot use standard input more than once.", "set all and read opus from stdin");
	error_case({"opustags", "-i", "-"}, "Cannot modify standard input in place.", "write stdin in-place");
	error_case({"opustags", "-o", "x", "--output", "y", "z"},
	           "Cannot specify --output more than once.", "double output");
	error_code_case({"opustags", "-S", "x"}, "Malformed tag: INVALID", ot::st::error, "attempt to read invalid argument with -S");
	error_case({"opustags", "-o", "", "--output", "y", "z"},
	           "Cannot specify --output more than once.", "double output with first filename empty");
	error_case({"opustags", "-e", "-i", "x", "y"},
	           "Exactly one input file must be specified.", "editing interactively two files at once");
	error_case({"opustags", "--edit", "-", "-o", "x"},
	           "Cannot edit interactively when standard input or standard output are already used.",
	           "editing interactively from stdandard intput");
	error_case({"opustags", "--edit", "x", "-o", "-"},
	           "Cannot edit interactively when standard input or standard output are already used.",
	           "editing interactively to stdandard output");
	error_case({"opustags", "--edit", "x"}, "Cannot edit interactively when no output is specified.", "editing without output");
	error_case({"opustags", "--edit", "x", "-i", "-a", "X=Y"}, "Cannot mix --edit with -adDsS.", "mixing -e and -a");
	error_case({"opustags", "--edit", "x", "-i", "-d", "X"}, "Cannot mix --edit with -adDsS.", "mixing -e and -d");
	error_case({"opustags", "--edit", "x", "-i", "-D"}, "Cannot mix --edit with -adDsS.", "mixing -e and -D");
	error_case({"opustags", "--edit", "x", "-i", "-S"}, "Cannot mix --edit with -adDsS.", "mixing -e and -S");
	error_case({"opustags", "--output-cover", "x", "--output-cover", "y"},
	           "Cannot specify --output-cover more than once.", "multiple --output-cover");
	error_case({"opustags", "x", "-o", "-", "--output-cover", "-"},
	            "Cannot specify standard output for both --output and --output-cover.", "-o and --output-cover conflict");
	error_case({"opustags", "-i", "x", "y", "--output-cover", "z"},
	            "Cannot use --output-cover with multiple input files.", "--output-cover with multiple input");
	error_case({"opustags", "-i", "--vendor", "x"},
	            "--vendor is only supported in read-only mode.", "--vendor when editing");
	error_case({"opustags", "-d", "\xFF", "x"},
	           "Could not encode argument into UTF-8:",
	           "-d with binary data");
	error_case({"opustags", "-a", "X=\xFF", "x"},
	           "Could not encode argument into UTF-8:",
	           "-a with binary data");
	error_case({"opustags", "-s", "X=\xFF", "x"},
	           "Could not encode argument into UTF-8:",
	           "-s with binary data");
}

static void check_delete_comments()
{
	using C = std::list<std::u8string>;
	C original = {u8"TITLE=X", u8"Title=Y", u8"Title=Z", u8"ARTIST=A", u8"artIst=B"};

	C edited = original;
	ot::delete_comments(edited, u8"derp");
	if (!std::equal(edited.begin(), edited.end(), original.begin(), original.end()))
		throw failure("should not have deleted anything");

	ot::delete_comments(edited, u8"Title");
	C expected = {u8"ARTIST=A", u8"artIst=B"};
	if (!std::equal(edited.begin(), edited.end(), expected.begin(), expected.end()))
		throw failure("did not delete all titles correctly");

	edited = original;
	ot::delete_comments(edited, u8"titlE=Y");
	ot::delete_comments(edited, u8"Title=z");
	expected = {u8"TITLE=X", u8"Title=Z", u8"ARTIST=A", u8"artIst=B"};
	if (!std::equal(edited.begin(), edited.end(), expected.begin(), expected.end()))
		throw failure("did not delete a specific title correctly");
}

int main(int argc, char **argv)
{
	std::cout << "1..4\n";
	run(check_read_comments, "check tags parsing");
	run(check_good_arguments, "check options parsing");
	run(check_bad_arguments, "check options parsing errors");
	run(check_delete_comments, "delete comments");
	return 0;
}
