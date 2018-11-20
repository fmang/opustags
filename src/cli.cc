/**
 * \file src/cli.cc
 * \ingroup cli
 *
 * Provide all the features of the opustags executable from a C++ API. The main point of separating
 * this module from the main one is to allow easy testing.
 */

#include <config.h>
#include <opustags.h>

#include <getopt.h>
#include <limits.h>
#include <unistd.h>

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

ot::status ot::process_options(int argc, char** argv, ot::options& opt)
{
	if (argc == 1) {
		fputs(version, stdout);
		fputs(usage, stdout);
		return st::exit_now;
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
				return st::bad_arguments;
			}
			break;
		case 'i':
			opt.inplace = optarg == nullptr ? ".otmp" : optarg;
			if (strcmp(opt.inplace, "") == 0) {
				fputs("the in-place suffix cannot be empty\n", stderr);
				return st::bad_arguments;
			}
			break;
		case 'y':
			opt.overwrite = true;
			break;
		case 'd':
			if (strchr(optarg, '=') != nullptr) {
				fprintf(stderr, "invalid field name: '%s'\n", optarg);
				return st::bad_arguments;
			}
			opt.to_delete.emplace_back(optarg);
			break;
		case 'a':
		case 's':
			if (strchr(optarg, '=') == NULL) {
				fprintf(stderr, "invalid comment: '%s'\n", optarg);
				return st::bad_arguments;
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
			return st::bad_arguments;
		}
	}
	if (opt.print_help) {
		puts(version);
		puts(usage);
		puts(help);
		puts("See the man page for extensive documentation.");
		return st::exit_now;
	}
	if (optind != argc - 1) {
		fputs("exactly one input file must be specified\n", stderr);
		return st::bad_arguments;
	}
	opt.path_in = argv[optind];
	if (opt.path_in.empty()) {
		fputs("input's file path cannot be empty\n", stderr);
		return st::bad_arguments;
	}
	if (opt.inplace != nullptr) {
		if (!opt.path_out.empty()) {
			fputs("cannot combine --in-place and --output\n", stderr);
			return st::bad_arguments;
		}
		opt.path_out = opt.path_in + opt.inplace;
	}
	if (opt.path_in == "-" && opt.set_all) {
		fputs("can't open stdin for input when -S is specified\n", stderr);
		return st::bad_arguments;
	}
	if (opt.path_in == "-" && opt.inplace) {
		fputs("cannot modify stdin in-place\n", stderr);
		return st::bad_arguments;
	}
	return st::ok;
}

/**
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
 * \todo Use getline. Lift the 16 KiB limitation and whatever's hardcoded here.
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

/**
 * Parse the packet as an OpusTags comment header, apply the user's modifications, and write the new
 * packet to the writer.
 */
static ot::status process_tags(const ogg_packet& packet, const ot::options& opt, ot::ogg_writer* writer)
{
	ot::opus_tags tags;
	ot::status rc = ot::parse_tags(packet, tags);
	if (rc != ot::st::ok)
		return rc;

	if (opt.delete_all) {
		tags.comments.clear();
	} else {
		for (const std::string& name : opt.to_delete)
			ot::delete_comments(tags, name.c_str());
	}

	if (opt.set_all)
		tags.comments = ot::read_comments(stdin);
	for (const std::string& comment : opt.to_add)
		tags.comments.emplace_back(comment);

	if (writer) {
		auto packet = ot::render_tags(tags);
		return writer->write_packet(packet);
	} else {
		ot::print_comments(tags.comments, stdout);
		return ot::st::ok;
	}
}

ot::status ot::process(ogg_reader& reader, ogg_writer* writer, const ot::options &opt)
{
	int packet_count = 0;
	for (;;) {
		// Read the next page.
		ot::status rc = reader.read_page();
		if (rc == ot::st::end_of_stream)
			break;
		else if (rc != ot::st::ok)
			return rc;
		// Short-circuit when the relevant packets have been read.
		if (packet_count >= 2 && writer) {
			if ((rc = writer->write_page(reader.page)) != ot::st::ok)
				return rc;
			continue;
		}
		auto serialno = ogg_page_serialno(&reader.page);
		if (writer && (rc = writer->prepare_stream(serialno)) != ot::st::ok)
			return rc;
		// Read all the packets.
		for (;;) {
			rc = reader.read_packet();
			if (rc == ot::st::end_of_page)
				break;
			else if (rc != ot::st::ok)
				return rc;
			packet_count++;
			if (packet_count == 1) { // Identification header
				rc = ot::validate_identification_header(reader.packet);
				if (rc != ot::st::ok)
					return rc;
			} else if (packet_count == 2) { // Comment header
				rc = process_tags(reader.packet, opt, writer);
				if (rc != ot::st::ok)
					return rc;
				if (!writer)
					return ot::st::ok; /* nothing else to do */
				else
					continue; /* process_tags wrote the new packet */
			}
			if (writer && (rc = writer->write_packet(reader.packet)) != ot::st::ok)
				return rc;
		}
		// Write the assembled page.
		if (writer && (rc = writer->flush_page()) != ot::st::ok)
			return rc;
	}
	if (packet_count < 2)
		return {ot::st::fatal_error, "Expected at least 2 Ogg packets"};
	return ot::st::ok;
}

/**
 * Check if two filepaths point to the same file, after path canonicalization.
 * The path "-" is treated specially, meaning stdin for path_in and stdout for path_out.
 */
static bool same_file(const std::string& path_in, const std::string& path_out)
{
	if (path_in == "-" || path_out == "-")
		return false;
	char canon_in[PATH_MAX+1], canon_out[PATH_MAX+1];
	if (realpath(path_in.c_str(), canon_in) && realpath(path_out.c_str(), canon_out)) {
		return (strcmp(canon_in, canon_out) == 0);
	}
	return false;
}

ot::status ot::run(ot::options& opt)
{
	if (!opt.path_out.empty() && same_file(opt.path_in, opt.path_out))
		return {ot::st::fatal_error, "Input and output files are the same"};

	std::unique_ptr<FILE, decltype(&fclose)> input(nullptr, &fclose);
	if (opt.path_in == "-") {
		input.reset(stdin);
	} else {
		FILE* input_file = fopen(opt.path_in.c_str(), "r");
		if (input_file == nullptr)
			return {ot::st::standard_error,
			        "could not open '" + opt.path_in + "' for reading: " + strerror(errno)};
		input.reset(input_file);
	}

	std::unique_ptr<FILE, decltype(&fclose)> output(nullptr, &fclose);
	if (opt.path_out == "-") {
		output.reset(stdout);
	} else if (!opt.path_out.empty()) {
		if (!opt.overwrite && access(opt.path_out.c_str(), F_OK) == 0)
			return {ot::st::fatal_error,
			        "'" + opt.path_out + "' already exists (use -y to overwrite)"};
		FILE* output_file = fopen(opt.path_out.c_str(), "w");
		if (output_file == nullptr)
			return {ot::st::standard_error,
				"could not open '" + opt.path_out + "' for writing: " + strerror(errno)};
		output.reset(output_file);
	}

	ot::status rc;
	{
		ot::ogg_reader reader(input.get());
		std::unique_ptr<ot::ogg_writer> writer;
		if (output != nullptr)
			writer = std::make_unique<ot::ogg_writer>(output.get());
		rc = process(reader, writer.get(), opt);
		/* delete reader and writer before closing the files */
	}

	input.reset();
	output.reset();

	if (rc != ot::st::ok) {
		if (!opt.path_out.empty() && opt.path_out != "-")
			remove(opt.path_out.c_str());
		return rc;
	}

	if (opt.inplace) {
		if (rename(opt.path_out.c_str(), opt.path_in.c_str()) == -1)
			return {ot::st::fatal_error,
			        "could not move the result to '" + opt.path_in + "': " + strerror(errno)};
	}

	return ot::st::ok;
}
