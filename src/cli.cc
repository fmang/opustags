/**
 * \file src/cli.cc
 * \ingroup cli
 *
 * Provide all the features of the opustags executable from a C++ API. The main point of separating
 * this module from the main one is to allow easy testing.
 *
 * \todo Use a safer temporary file name for in-place editing, like tmpnam.
 * \todo Abort editing with --set-all if one comment is invalid?
 */

#include <config.h>
#include <opustags.h>

#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

static const char help_message[] =
PROJECT_NAME " version " PROJECT_VERSION
R"raw(

Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS FILE -o FILE

Options:
  -h, --help              print this help
  -o, --output FILE       set the output file
  -i, --in-place          overwrite the input file instead of writing a different output file
  -y, --overwrite         overwrite the output file if it already exists
  -a, --add FIELD=VALUE   add a comment
  -d, --delete FIELD      delete all previously existing comments of a specific type
  -D, --delete-all        delete all the previously existing comments
  -s, --set FIELD=VALUE   replace a comment (shorthand for --delete FIELD --add FIELD=VALUE)
  -S, --set-all           replace all the comments with the ones read from standard input

See the man page for extensive documentation.
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
	if (argc == 1)
		return {st::bad_arguments, "No arguments specified. Use -h for help."};

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

	if (opt.print_help)
		return st::ok;
	if (optind != argc - 1) {
		fputs("exactly one input file must be specified\n", stderr);
		return st::bad_arguments;
	}
	opt.path_in = argv[optind];
	if (opt.path_in.empty()) {
		fputs("input's file path cannot be empty\n", stderr);
		return st::bad_arguments;
	}
	if (opt.inplace != nullptr && !opt.path_out.empty()) {
		fputs("cannot combine --in-place and --output\n", stderr);
		return st::bad_arguments;
	}
	if (opt.path_in == "-" && opt.set_all) {
		fputs("can't open standard input for input when --set-all is specified\n", stderr);
		return st::bad_arguments;
	}
	if (opt.path_in == "-" && opt.inplace) {
		fputs("cannot modify standard input in-place\n", stderr);
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

std::list<std::string> ot::read_comments(FILE* input)
{
	std::list<std::string> comments;
	char* line = nullptr;
	size_t buflen = 0;
	ssize_t nread;
	while ((nread = getline(&line, &buflen, input)) != -1) {
		if (nread > 0 && line[nread - 1] == '\n')
			--nread;
		if (nread == 0)
			continue;
		if (memchr(line, '=', nread) == nullptr) {
			fputs("warning: skipping malformed tag\n", stderr);
			continue;
		}
		comments.emplace_back(line, nread);
	}
	free(line);
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
	if (opt.print_help) {
		fputs(help_message, stdout);
		return st::ok;
	}

	std::string path_out = opt.inplace ? opt.path_in + opt.inplace : opt.path_out;

	if (!path_out.empty() && same_file(opt.path_in, path_out))
		return {ot::st::fatal_error, "Input and output files are the same"};

	ot::file input;
	if (opt.path_in == "-") {
		input = stdin;
	} else {
		input = fopen(opt.path_in.c_str(), "r");
		if (input == nullptr)
			return {ot::st::standard_error,
			        "Could not open '" + opt.path_in + "' for reading: " + strerror(errno)};
	}

	ot::file output;
	if (path_out == "-") {
		output.reset(stdout);
	} else if (!path_out.empty()) {
		if (!opt.overwrite && access(path_out.c_str(), F_OK) == 0)
			return {ot::st::fatal_error,
			        "'" + path_out + "' already exists (use -y to overwrite)"};
		output = fopen(path_out.c_str(), "w");
		if (output == nullptr)
			return {ot::st::standard_error,
				"Could not open '" + path_out + "' for writing: " + strerror(errno)};
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
		if (!path_out.empty() && path_out != "-")
			remove(path_out.c_str());
		return rc;
	}

	if (opt.inplace) {
		if (rename(path_out.c_str(), opt.path_in.c_str()) == -1)
			return {ot::st::fatal_error,
			        "Could not move the result to '" + opt.path_in + "': " + strerror(errno)};
	}

	return ot::st::ok;
}
