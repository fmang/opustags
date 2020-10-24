/**
 * \file src/cli.cc
 * \ingroup cli
 *
 * Provide all the features of the opustags executable from a C++ API. The main point of separating
 * this module from the main one is to allow easy testing.
 */

#include <config.h>
#include <opustags.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

using namespace std::literals::string_literals;

static const char help_message[] =
PROJECT_NAME " version " PROJECT_VERSION
R"raw(

Usage: opustags --help
       opustags [OPTIONS] FILE
       opustags OPTIONS -i FILE...
       opustags OPTIONS FILE -o FILE

Options:
  -h, --help                    print this help
  -o, --output FILE             specify the output file
  -i, --in-place                overwrite the input files
  -y, --overwrite               overwrite the output file if it already exists
  -a, --add FIELD=VALUE         add a comment
  -d, --delete FIELD[=VALUE]    delete previously existing comments
  -D, --delete-all              delete all the previously existing comments
  -s, --set FIELD=VALUE         replace a comment
  -S, --set-all                 import comments from standard input
  -e, --edit                    edit tags interactively in VISUAL/EDITOR

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
	{"edit", no_argument, 0, 'e'},
	{NULL, 0, 0, 0}
};

ot::status ot::parse_options(int argc, char** argv, ot::options& opt, FILE* comments_input)
{
	static ot::encoding_converter to_utf8("", "UTF-8");
	std::string utf8;
	std::string::size_type equal;
	ot::status rc;
	bool set_all = false;
	opt = {};
	if (argc == 1)
		return {st::bad_arguments, "No arguments specified. Use -h for help."};
	int c;
	optind = 0;
	while ((c = getopt_long(argc, argv, ":ho:iyd:a:s:DSe", getopt_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			opt.print_help = true;
			break;
		case 'o':
			if (opt.path_out)
				return {st::bad_arguments, "Cannot specify --output more than once."};
			opt.path_out = optarg;
			break;
		case 'i':
			opt.in_place = true;
			opt.overwrite = true;
			break;
		case 'y':
			opt.overwrite = true;
			break;
		case 'd':
			rc = to_utf8(optarg, strlen(optarg), utf8);
			if (rc != ot::st::ok)
				return {st::bad_arguments, "Could not encode argument into UTF-8: " + rc.message};
			opt.to_delete.emplace_back(std::move(utf8));
			break;
		case 'a':
		case 's':
			rc = to_utf8(optarg, strlen(optarg), utf8);
			if (rc != ot::st::ok)
				return {st::bad_arguments, "Could not encode argument into UTF-8: " + rc.message};
			if ((equal = utf8.find('=')) == std::string::npos)
				return {st::bad_arguments, "Comment does not contain an equal sign: "s + optarg + "."};
			if (c == 's')
				opt.to_delete.emplace_back(utf8.substr(0, equal));
			opt.to_add.emplace_back(std::move(utf8));
			break;
		case 'S':
			opt.delete_all = true;
			set_all = true;
			break;
		case 'D':
			opt.delete_all = true;
			break;
		case 'e':
			opt.edit_interactively = true;
			break;
		case ':':
			return {st::bad_arguments,
			        "Missing value for option '"s + argv[optind - 1] + "'."};
		default:
			return {st::bad_arguments, "Unrecognized option '" +
			        (optopt ? "-"s + static_cast<char>(optopt) : argv[optind - 1]) + "'."};
		}
	}
	if (opt.print_help)
		return st::ok;

	// All non-option arguments are input files.
	bool stdin_as_input = false;
	for (int i = optind; i < argc; i++) {
		stdin_as_input = stdin_as_input || strcmp(argv[i], "-") == 0;
		opt.paths_in.emplace_back(argv[i]);
	}

	if (opt.in_place && opt.path_out)
		return {st::bad_arguments, "Cannot combine --in-place and --output."};

	if (opt.in_place && stdin_as_input)
		return {st::bad_arguments, "Cannot modify standard input in place."};

	if ((!opt.in_place || opt.edit_interactively) && opt.paths_in.size() != 1)
		return {st::bad_arguments, "Exactly one input file must be specified."};

	if (set_all && stdin_as_input)
		return {st::bad_arguments, "Cannot use standard input as input file when --set-all is specified."};

	if (opt.edit_interactively && (set_all || stdin_as_input || opt.path_out == "-"))
		return {st::bad_arguments, "Cannot edit interactively when standard input or standard output are already used."};

	if (opt.edit_interactively && !opt.path_out.has_value() && !opt.in_place)
		return {st::bad_arguments, "Cannot edit interactively when no output is specified."};

	if (set_all) {
		// Read comments from stdin and prepend them to opt.to_add.
		std::list<std::string> comments;
		auto rc = read_comments(comments_input, comments);
		if (rc != st::ok)
			return rc;
		opt.to_add.splice(opt.to_add.begin(), std::move(comments));
	}
	return st::ok;
}

/**
 * \todo Find a way to support new lines such that they can be read back by #read_comment without
 *       ambiguity. We could add a raw mode and separate comments with a \0, or escape control
 *       characters with a backslash, but we should also preserve compatibiltity with potential
 *       callers that don’t escape backslashes. Maybe add options to select a mode between simple,
 *       raw, and escaped.
 */
void ot::print_comments(const std::list<std::string>& comments, FILE* output)
{
	static ot::encoding_converter from_utf8("UTF-8", "//TRANSLIT");
	std::string local;
	bool info_lost = false;
	bool bad_comments = false;
	bool has_newline = false;
	bool has_control = false;
	for (const std::string& comment : comments) {
		ot::status rc = from_utf8(comment, local);
		if (rc == ot::st::information_lost) {
			info_lost = true;
		} else if (rc != ot::st::ok) {
			bad_comments = true;
			continue;
		}
		for (unsigned char c : comment) {
			if (c == '\n')
				has_newline = true;
			else if (c < 0x20)
				has_control = true;
		}
		fwrite(local.data(), 1, local.size(), output);
		putc('\n', output);
	}
	if (info_lost)
		fputs("warning: Some tags have been transliterated to your system encoding.\n", stderr);
	if (bad_comments)
		fputs("warning: Some tags are not properly encoded and have not been displayed.\n", stderr);
	if (has_newline)
		fputs("warning: Some tags contain newline characters. "
		      "These are not supported by --set-all.\n", stderr);
	if (has_control)
		fputs("warning: Some tags contain control characters.\n", stderr);
}

ot::status ot::read_comments(FILE* input, std::list<std::string>& comments)
{
	static ot::encoding_converter to_utf8("", "UTF-8");
	comments.clear();
	char* line = nullptr;
	size_t buflen = 0;
	ssize_t nread;
	while ((nread = getline(&line, &buflen, input)) != -1) {
		if (nread > 0 && line[nread - 1] == '\n')
			--nread;
		if (nread == 0)
			continue;
		if (line[0] == '#') // comment
			continue;
		if (memchr(line, '=', nread) == nullptr) {
			ot::status rc = {ot::st::error, "Malformed tag: " + std::string(line, nread)};
			free(line);
			return rc;
		}
		std::string utf8;
		ot::status rc = to_utf8(line, nread, utf8);
		if (rc == ot::st::ok) {
			comments.emplace_back(std::move(utf8));
		} else {
			free(line);
			return {ot::st::badly_encoded, "UTF-8 conversion error: " + rc.message};
		}
	}
	free(line);
	return ot::st::ok;
}

void ot::delete_comments(std::list<std::string>& comments, const std::string& selector)
{
	auto name = selector.data();
	auto equal = selector.find('=');
	auto value = (equal == std::string::npos ? nullptr : name + equal + 1);
	auto name_len = value ? equal : selector.size();
	auto value_len = value ? selector.size() - equal - 1 : 0;
	auto it = comments.begin(), end = comments.end();
	while (it != end) {
		auto current = it++;
		bool name_match = current->size() > name_len + 1 &&
		                  (*current)[name_len] == '=' &&
		                  strncasecmp(current->data(), name, name_len) == 0;
		if (!name_match)
			continue;
		bool value_match = value == nullptr ||
		                   (current->size() == selector.size() &&
		                    memcmp(current->data() + equal + 1, value, value_len) == 0);
		if (value_match)
			comments.erase(current);
	}
}

/** Apply the modifications requested by the user to the opustags packet. */
static ot::status edit_tags(ot::opus_tags& tags, const ot::options& opt)
{
	if (opt.delete_all) {
		tags.comments.clear();
	} else for (const std::string& name : opt.to_delete) {
		ot::delete_comments(tags.comments, name.c_str());
	}

	for (const std::string& comment : opt.to_add)
		tags.comments.emplace_back(comment);

	return ot::st::ok;
}

/** Spawn VISUAL or EDITOR to edit the given tags. */
static ot::status edit_tags_interactively(ot::opus_tags& tags, const std::optional<std::string>& base_path)
{
	const char* editor = nullptr;
	if (getenv("TERM") != nullptr)
		editor = getenv("VISUAL");
	if (editor == nullptr) // without a terminal, or if VISUAL is unset
		editor = getenv("EDITOR");
	if (editor == nullptr)
		return {ot::st::error,
		        "No editor specified in environment variable VISUAL or EDITOR."};

	// Building the temporary tags file.
	std::string tags_path = base_path.value_or("tags") + ".XXXXXX.opustags";
	int fd = mkstemps(const_cast<char*>(tags_path.data()), 9);
	FILE* tags_file;
	if (fd == -1 || (tags_file = fdopen(fd, "w")) == nullptr)
		return {ot::st::standard_error,
		        "Could not open '" + tags_path + "': " + strerror(errno)};
	ot::print_comments(tags.comments, tags_file);
	if (fclose(tags_file) != 0)
		return {ot::st::standard_error, tags_path + ": fclose error: "s + strerror(errno)};

	// Spawn the editor, and watch the modification timestamps.
	ot::status rc;
	timespec before, after;
	if ((rc = ot::get_file_timestamp(tags_path.c_str(), before)) != ot::st::ok)
		return rc;
	ot::status editor_rc = ot::run_editor(editor, tags_path.c_str());
	if ((rc = ot::get_file_timestamp(tags_path.c_str(), after)) != ot::st::ok)
		return rc; // probably because the file was deleted
	bool modified = (before.tv_sec != after.tv_sec || before.tv_nsec != after.tv_nsec);
	if (editor_rc != ot::st::ok) {
		if (modified)
			fprintf(stderr, "warning: Leaving %s on the disk.\n", tags_path.c_str());
		else
			remove(tags_path.c_str());
		return rc;
	} else if (!modified) {
		remove(tags_path.c_str());
		fputs("Cancelling edition because the tags file was not modified.\n", stderr);
		return ot::st::cancel;
	}

	// Applying the new tags.
	tags_file = fopen(tags_path.c_str(), "r");
	if (tags_file == nullptr)
		return {ot::st::standard_error, "Error opening " + tags_path + ": " + strerror(errno)};
	if ((rc = ot::read_comments(tags_file, tags.comments)) != ot::st::ok) {
		fprintf(stderr, "warning: Leaving %s on the disk.\n", tags_path.c_str());
		return rc;
	}
	fclose(tags_file);

	// Remove the temporary tags file only on success, because unlike the
	// partial Ogg file that is irrecoverable, the edited tags file
	// contains user data, so let’s leave users a chance to recover it.
	remove(tags_path.c_str());

	return ot::st::ok;
}

/**
 * Main loop of opustags. Read the packets from the reader, and forwards them to the writer.
 * Transform the OpusTags packet on the fly.
 *
 * The writer is optional. When writer is nullptr, opustags runs in read-only mode.
 */
static ot::status process(ot::ogg_reader& reader, ot::ogg_writer* writer, const ot::options &opt)
{
	bool focused = false; /*< the stream on which we operate is defined */
	int focused_serialno; /*< when focused, the serialno of the focused stream */
	int absolute_page_no = -1; /*< page number in the physical stream, not logical */
	for (;;) {
		ot::status rc = reader.next_page();
		if (rc == ot::st::end_of_stream)
			break;
		else if (rc == ot::st::bad_stream && absolute_page_no == -1)
			return {ot::st::bad_stream, "Input is not a valid Ogg file."};
		else if (rc != ot::st::ok)
			return rc;
		++absolute_page_no;
		auto serialno = ogg_page_serialno(&reader.page);
		auto pageno = ogg_page_pageno(&reader.page);
		if (!focused) {
			focused = true;
			focused_serialno = serialno;
		} else if (serialno != focused_serialno) {
			/** \todo Support mixed streams. */
			return {ot::st::error, "Muxed streams are not supported yet."};
		}
		if (absolute_page_no == 0) { // Identification header
			if (!ot::is_opus_stream(reader.page))
				return {ot::st::error, "Not an Opus stream."};
			if (writer) {
				rc = writer->write_page(reader.page);
				if (rc != ot::st::ok)
					return rc;
			}
		} else if (absolute_page_no == 1) { // Comment header
			ot::opus_tags tags;
			rc = reader.process_header_packet(
				[&tags](ogg_packet& p) { return ot::parse_tags(p, tags); });
			if (rc != ot::st::ok)
				return rc;
			if ((rc = edit_tags(tags, opt)) != ot::st::ok)
				return rc;
			if (writer) {
				if (opt.edit_interactively &&
				    (rc = edit_tags_interactively(tags, writer->path)) != ot::st::ok)
					return rc;
				auto packet = ot::render_tags(tags);
				rc = writer->write_header_packet(serialno, pageno, packet);
				if (rc != ot::st::ok)
					return rc;
			} else {
				ot::print_comments(tags.comments, stdout);
				break;
			}
		} else {
			if (writer && (rc = writer->write_page(reader.page)) != ot::st::ok)
				return rc;
		}
	}
	if (absolute_page_no < 1)
		return {ot::st::error, "Expected at least 2 Ogg pages."};
	return ot::st::ok;
}

static ot::status run_single(const ot::options& opt, const std::string& path_in, const std::optional<std::string>& path_out)
{
	ot::file input;
	if (path_in == "-")
		input = stdin;
	else if ((input = fopen(path_in.c_str(), "r")) == nullptr)
		return {ot::st::standard_error,
		        "Could not open '" + path_in + "' for reading: " + strerror(errno)};
	ot::ogg_reader reader(input.get());

	/* Read-only mode. */
	if (!path_out)
		return process(reader, nullptr, opt);

	/* Read-write mode.
	 *
	 * The output pointer is set to one of:
	 *  - stdout for "-",
	 *  - final_output.get() for special files like /dev/null,
	 *  - temporary_output.get() for regular files.
	 *
	 * We use a temporary output file for the following reasons:
	 *  1. A partial .opus output would be seen by softwares like media players, but a .part
	 *     (for partial) won’t.
	 *  2. If the process crashes badly, or the power cuts off, we don't want to leave a partial
	 *     file at the final location. The temporary file is going to remain though.
	 *  3. If we're overwriting a regular file, we'd rather avoid wiping its content before we
	 *     even started reading the input file. That way, the original file is always preserved
	 *     on error or crash.
	 *  4. It is necessary for in-place editing. We can't reliably open the same file as both
	 *     input and output.
	 */

	FILE* output = nullptr;
	ot::partial_file temporary_output;
	ot::file final_output;

	ot::status rc = ot::st::ok;
	struct stat output_info;
	if (path_out == "-") {
		output = stdout;
	} else if (stat(path_out->c_str(), &output_info) == 0) {
		/* The output file exists. */
		if (!S_ISREG(output_info.st_mode)) {
			/* Special files are opened for writing directly. */
			if ((final_output = fopen(path_out->c_str(), "w")) == nullptr)
				rc = {ot::st::standard_error,
				      "Could not open '" + path_out.value() + "' for writing: " +
				      strerror(errno)};
			output = final_output.get();
		} else if (opt.overwrite) {
			rc = temporary_output.open(path_out->c_str());
			output = temporary_output.get();
		} else {
			rc = {ot::st::error,
			      "'" + path_out.value() + "' already exists. Use -y to overwrite."};
		}
	} else if (errno == ENOENT) {
		rc = temporary_output.open(path_out->c_str());
		output = temporary_output.get();
	} else {
		rc = {ot::st::error,
		      "Could not identify '" + path_in + "': " + strerror(errno)};
	}
	if (rc != ot::st::ok)
		return rc;

	ot::ogg_writer writer(output);
	writer.path = path_out;
	rc = process(reader, &writer, opt);
	if (rc == ot::st::ok)
		rc = temporary_output.commit();

	return rc;
}

ot::status ot::run(const ot::options& opt)
{
	if (opt.print_help) {
		fputs(help_message, stdout);
		return st::ok;
	}

	ot::status global_rc = st::ok;
	for (const auto& path_in : opt.paths_in) {
		ot::status rc = run_single(opt, path_in, opt.in_place ? path_in : opt.path_out);
		if (rc != st::ok) {
			global_rc = st::error;
			if (!rc.message.empty())
				fprintf(stderr, "%s: error: %s\n", path_in.c_str(), rc.message.c_str());
		}
	}
	return global_rc;
}
