#include <config.h>
#include <opustags.h>

#include <getopt.h>

#include <memory>

static const char* version = PROJECT_NAME " version " PROJECT_VERSION "\n";

static const char* usage = 1 + R"raw(
Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS FILE -o FILE
)raw";

static const char* help = 1 + R"raw(
Options:
  -h, --help              print this help
  -o, --output            write the modified tags to a file
  -i, --in-place [SUFFIX] use a temporary file then replace the original file
  -y, --overwrite         overwrite the output file if it already exists
  -d, --delete FIELD      delete all the fields of a specified type
  -a, --add FIELD=VALUE   add a field
  -s, --set FIELD=VALUE   delete then add a field
  -D, --delete-all        delete all the fields!
  -S, --set-all           read the fields from stdin
)raw";

static struct option getopt_options[] = {
	{"help", no_argument, 0, 'h'},
	{"output", required_argument, 0, 'o'},
	{"in-place", optional_argument, 0, 'i'},
	{"overwrite", no_argument, 0, 'y'},
	{"delete", required_argument, 0, 'd'},
	{"add", required_argument, 0, 'a'},
	{"set", required_argument, 0, 's'},
	{"delete-all", no_argument, 0, 'D'},
	{"set-all", no_argument, 0, 'S'},
	{NULL, 0, 0, 0}
};

/**
 * Process the command-line arguments.
 *
 * This function does not perform I/O related validations, but checks the consistency of its
 * arguments.
 *
 * It returns one of :
 * - #ot::status::ok, meaning the process may continue normally.
 * - #ot::status::exit_now, meaning there is nothing to do and process should exit successfully.
 *   This happens when all the user wants is see the help or usage.
 * - #ot::status::bad_arguments, meaning the arguments were invalid and the process should exit with
 *   an error.
 *
 * Help messages are written on standard output, and error messages on standard error.
 */
ot::status ot::process_options(int argc, char** argv, ot::options& opt)
{
	if (argc == 1) {
		fputs(version, stdout);
		fputs(usage, stdout);
		return status::exit_now;
	}
	int c;
	while ((c = getopt_long(argc, argv, "ho:i::yd:a:s:DS", getopt_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			opt.print_help = true;
			break;
		case 'o':
			opt.path_out = optarg;
			if (opt.path_out.empty()) {
				fputs("output's file path cannot be empty\n", stderr);
				return status::bad_arguments;
			}
			break;
		case 'i':
			opt.inplace = optarg == nullptr ? ".otmp" : optarg;
			if (strcmp(opt.inplace, "") == 0) {
				fputs("the in-place suffix cannot be empty\n", stderr);
				return status::bad_arguments;
			}
			break;
		case 'y':
			opt.overwrite = true;
			break;
		case 'd':
			if (strchr(optarg, '=') != nullptr) {
				fprintf(stderr, "invalid field name: '%s'\n", optarg);
				return status::bad_arguments;
			}
			opt.to_delete.emplace_back(optarg);
			break;
		case 'a':
		case 's':
			if (strchr(optarg, '=') == NULL) {
				fprintf(stderr, "invalid comment: '%s'\n", optarg);
				return status::bad_arguments;
			}
			opt.to_add.emplace_back(optarg);
			if (c == 's')
				opt.to_delete.emplace_back(optarg);
			break;
		case 'S':
			opt.set_all = true;
			/* fall through */
		case 'D':
			opt.delete_all = true;
			break;
		default:
			/* getopt printed a message */
			return status::bad_arguments;
		}
	}
	if (opt.print_help) {
		puts(version);
		puts(usage);
		puts(help);
		puts("See the man page for extensive documentation.");
		return status::exit_now;
	}
	if (optind != argc - 1) {
		fputs("exactly one input file must be specified\n", stderr);
		return status::bad_arguments;
	}
	opt.path_in = argv[optind];
	if (opt.path_in.empty()) {
		fputs("input's file path cannot be empty\n", stderr);
		return status::bad_arguments;
	}
	if (opt.inplace != nullptr && !opt.path_out.empty()) {
		fputs("cannot combine --in-place and --output\n", stderr);
		return status::bad_arguments;
	}
	if (opt.path_in == "-" && opt.set_all) {
		fputs("can't open stdin for input when -S is specified\n", stderr);
		return status::bad_arguments;
	}
	if (opt.path_in == "-" && opt.inplace) {
		fputs("cannot modify stdin in-place\n", stderr);
		return status::bad_arguments;
	}
	return status::ok;
}

/**
 * Display the tags on stdout.
 *
 * Print all the comments, separated by line breaks. Since a comment may
 * contain line breaks, this output is not completely reliable, but it fits
 * most cases.
 *
 * The output generated is meant to be parseable by #ot::read_tags.
 *
 * \todo Escape new lines.
 */
void ot::print_comments(const std::list<std::string>& comments, FILE* output)
{
	for (const std::string& comment : comments) {
		fwrite(comment.data(), 1, comment.size(), output);
		puts("");
	}
}

/**
 * Parse the comments outputted by #ot::print_comments.
 *
 * \todo Use an std::istream or getline. Lift the 16 KiB limitation and whatever's hardcoded here.
 */
std::list<std::string> ot::read_comments(FILE* input)
{
	std::list<std::string> comments;
	auto raw_tags = std::make_unique<char[]>(16383);
	size_t raw_len = fread(raw_tags.get(), 1, 16382, stdin);
	if (raw_len == 16382)
		fputs("warning: truncating comment to 16 KiB\n", stderr);
	raw_tags[raw_len] = '\0';
	size_t field_len = 0;
	bool caught_eq = false;
	char* cursor = raw_tags.get();
	for (size_t i = 0; i <= raw_len; ++i) {
		if (raw_tags[i] == '\n' || raw_tags[i] == '\0') {
			raw_tags[i] = '\0';
			if (field_len == 0)
				continue;
			if (caught_eq)
				comments.emplace_back(cursor);
			else
				fputs("warning: skipping malformed tag\n", stderr);
			cursor = raw_tags.get() + i + 1;
			field_len = 0;
			caught_eq = false;
			continue;
		}
		if (raw_tags[i] == '=')
			caught_eq = true;
		++field_len;
	}
	return comments;
}
