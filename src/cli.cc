/**
 * \file src/cli.cc
 * \ingroup cli
 *
 * Provide all the features of the opustags executable from a C++ API. The main point of separating
 * this module from the main one is to allow easy testing.
 */

#include <opustags.h>

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
  --raw                         disable encoding conversion

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
	{"raw", no_argument, 0, 'r'},
	{NULL, 0, 0, 0}
};

ot::options ot::parse_options(int argc, char** argv, FILE* comments_input)
{
	options opt;
	static ot::encoding_converter to_utf8("", "UTF-8");
	const char* equal;
	ot::status rc;
	bool set_all = false;
	opt = {};
	if (argc == 1)
		throw status {st::bad_arguments, "No arguments specified. Use -h for help."};
	int c;
	optind = 0;
	while ((c = getopt_long(argc, argv, ":ho:iyd:a:s:DSe", getopt_options, NULL)) != -1) {
		switch (c) {
		case 'h':
			opt.print_help = true;
			break;
		case 'o':
			if (opt.path_out)
				throw status {st::bad_arguments, "Cannot specify --output more than once."};
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
			opt.to_delete.emplace_back(optarg);
			break;
		case 'a':
		case 's':
			equal = strchr(optarg, '=');
			if (equal == nullptr)
				throw status {st::bad_arguments, "Comment does not contain an equal sign: "s + optarg + "."};
			if (c == 's')
				opt.to_delete.emplace_back(optarg, equal - optarg);
			opt.to_add.emplace_back(optarg);
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
		case 'r':
			opt.raw = true;
			break;
		case ':':
			throw status {st::bad_arguments, "Missing value for option '"s + argv[optind - 1] + "'."};
		default:
			throw status {st::bad_arguments, "Unrecognized option '" +
			              (optopt ? "-"s + static_cast<char>(optopt) : argv[optind - 1]) + "'."};
		}
	}
	if (opt.print_help)
		return opt;

	// All non-option arguments are input files.
	bool stdin_as_input = false;
	for (int i = optind; i < argc; i++) {
		stdin_as_input = stdin_as_input || strcmp(argv[i], "-") == 0;
		opt.paths_in.emplace_back(argv[i]);
	}

	// Convert arguments to UTF-8.
	if (!opt.raw) {
		for (std::list<std::string>* args : { &opt.to_add, &opt.to_delete }) {
			try {
				for (std::string& arg : *args)
					arg = to_utf8(arg);
			} catch (const ot::status& rc) {
				throw status {st::bad_arguments, "Could not encode argument into UTF-8: " + rc.message};
			}
		}
	}

	if (opt.in_place && opt.path_out)
		throw status {st::bad_arguments, "Cannot combine --in-place and --output."};

	if (opt.in_place && stdin_as_input)
		throw status {st::bad_arguments, "Cannot modify standard input in place."};

	if ((!opt.in_place || opt.edit_interactively) && opt.paths_in.size() != 1)
		throw status {st::bad_arguments, "Exactly one input file must be specified."};

	if (set_all && stdin_as_input)
		throw status {st::bad_arguments, "Cannot use standard input as input file when --set-all is specified."};

	if (opt.edit_interactively && (stdin_as_input || opt.path_out == "-"))
		throw status {st::bad_arguments, "Cannot edit interactively when standard input or standard output are already used."};

	if (opt.edit_interactively && !opt.path_out.has_value() && !opt.in_place)
		throw status {st::bad_arguments, "Cannot edit interactively when no output is specified."};

	if (opt.edit_interactively && (opt.delete_all || !opt.to_add.empty() || !opt.to_delete.empty()))
		throw status {st::bad_arguments, "Cannot mix --edit with -adDsS."};

	if (set_all) {
		// Read comments from stdin and prepend them to opt.to_add.
		std::list<std::string> comments = read_comments(comments_input, opt.raw);
		opt.to_add.splice(opt.to_add.begin(), std::move(comments));
	}
	return opt;
}

/** Format a UTF-8 string by adding tabulations (\t) after line feeds (\n) to mark continuation for
 *  multiline values. */
static std::string format_value(const std::string& source)
{
	auto newline_count = std::count(source.begin(), source.end(), '\n');

	// General case: the value fits on a single line. Use std::string’s copy constructor for the
	// most efficient copy we could hope for.
	if (newline_count == 0)
		return source;

	std::string formatted;
	formatted.reserve(source.size() + newline_count);
	for (auto c : source) {
		formatted.push_back(c);
		if (c == '\n')
			formatted.push_back('\t');
	}
	return formatted;
}

/**
 * Print comments in a human readable format that can also be read back in by #read_comment.
 *
 * To disambiguate between a newline embedded in a comment and a newline representing the start of
 * the next tag, continuation lines always have a single TAB (^I) character added to the beginning.
 */
void ot::print_comments(const std::list<std::string>& comments, FILE* output, bool raw)
{
	static ot::encoding_converter from_utf8("UTF-8", "");
	std::string local;
	bool has_control = false;
	for (const std::string& source_comment : comments) {
		if (!has_control) { // Don’t bother analyzing comments if the flag is already up.
			for (unsigned char c : source_comment) {
				if (c < 0x20 && c != '\n') {
					has_control = true;
					break;
				}
			}
		}

		std::string utf8_comment = format_value(source_comment);
		const std::string* comment;
		// Convert the comment from UTF-8 to the system encoding if relevant.
		if (raw) {
			comment = &utf8_comment;
		} else {
			try {
				local = from_utf8(utf8_comment);
				comment = &local;
			} catch (ot::status& rc) {
				rc.message += " See --raw.";
				throw;
			}
		}

		fwrite(comment->data(), 1, comment->size(), output);
		putc('\n', output);
	}
	if (has_control)
		fputs("warning: Some tags contain control characters.\n", stderr);
}

std::list<std::string> ot::read_comments(FILE* input, bool raw)
{
	std::list<std::string> comments;
	static ot::encoding_converter to_utf8("", "UTF-8");
	comments.clear();
	char* source_line = nullptr;
	size_t buflen = 0;
	ssize_t nread;
	std::string* previous_comment = nullptr;
	while ((nread = getline(&source_line, &buflen, input)) != -1) {
		if (nread > 0 && source_line[nread - 1] == '\n')
			--nread; // Chomp.

		std::string line;
		if (raw) {
			line = std::string(source_line, nread);
		} else {
			try {
				line = to_utf8(std::string_view(source_line, nread));
			} catch (const ot::status& rc) {
				free(source_line);
				throw ot::status {ot::st::badly_encoded, "UTF-8 conversion error: " + rc.message};
			}
		}

		if (line.empty()) {
			// Ignore empty lines.
			previous_comment = nullptr;
		} else if (line[0] == '#') {
			// Ignore comments.
			previous_comment = nullptr;
		} else if (line[0] == '\t') {
			// Continuation line: append the current line to the previous tag.
			if (previous_comment == nullptr) {
				ot::status rc = {ot::st::error, "Unexpected continuation line: " + std::string(source_line, nread)};
				free(source_line);
				throw rc;
			} else {
				line[0] = '\n';
				previous_comment->append(line);
			}
		} else if (line.find('=') == std::string::npos) {
			ot::status rc = {ot::st::error, "Malformed tag: " + std::string(source_line, nread)};
			free(source_line);
			throw rc;
		} else {
			previous_comment = &comments.emplace_back(std::move(line));
		}
	}
	free(source_line);
	return comments;
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
static void edit_tags(ot::opus_tags& tags, const ot::options& opt)
{
	if (opt.delete_all) {
		tags.comments.clear();
	} else for (const std::string& name : opt.to_delete) {
		ot::delete_comments(tags.comments, name.c_str());
	}

	for (const std::string& comment : opt.to_add)
		tags.comments.emplace_back(comment);
}

/** Spawn VISUAL or EDITOR to edit the given tags. */
static void edit_tags_interactively(ot::opus_tags& tags, const std::optional<std::string>& base_path, bool raw)
{
	const char* editor = nullptr;
	if (getenv("TERM") != nullptr)
		editor = getenv("VISUAL");
	if (editor == nullptr) // without a terminal, or if VISUAL is unset
		editor = getenv("EDITOR");
	if (editor == nullptr)
		throw ot::status {ot::st::error,
		                 "No editor specified in environment variable VISUAL or EDITOR."};

	// Building the temporary tags file.
	ot::status rc;
	std::string tags_path = base_path.value_or("tags") + ".XXXXXX.opustags";
	int fd = mkstemps(const_cast<char*>(tags_path.data()), 9);
	ot::file tags_file;
	if (fd == -1 || (tags_file = fdopen(fd, "w")) == nullptr)
		throw ot::status {ot::st::standard_error,
		                  "Could not open '" + tags_path + "': " + strerror(errno)};
	ot::print_comments(tags.comments, tags_file.get(), raw);
	tags_file.reset();

	// Spawn the editor, and watch the modification timestamps.
	timespec before = ot::get_file_timestamp(tags_path.c_str());
	ot::status editor_rc;
	try {
		ot::run_editor(editor, tags_path);
		editor_rc = ot::st::ok;
	} catch (const ot::status& rc) {
		editor_rc = rc;
	}
	timespec after = ot::get_file_timestamp(tags_path.c_str());
	bool modified = (before.tv_sec != after.tv_sec || before.tv_nsec != after.tv_nsec);
	if (editor_rc != ot::st::ok) {
		if (modified)
			fprintf(stderr, "warning: Leaving %s on the disk.\n", tags_path.c_str());
		else
			remove(tags_path.c_str());
		throw editor_rc;
	} else if (!modified) {
		remove(tags_path.c_str());
		fputs("Cancelling edition because the tags file was not modified.\n", stderr);
		throw ot::status {ot::st::cancel};
	}

	// Applying the new tags.
	tags_file = fopen(tags_path.c_str(), "re");
	if (tags_file == nullptr)
		throw ot::status {ot::st::standard_error, "Error opening " + tags_path + ": " + strerror(errno)};
	try {
		tags.comments = ot::read_comments(tags_file.get(), raw);
	} catch (const ot::status& rc) {
		fprintf(stderr, "warning: Leaving %s on the disk.\n", tags_path.c_str());
		throw;
	}
	tags_file.reset();

	// Remove the temporary tags file only on success, because unlike the
	// partial Ogg file that is irrecoverable, the edited tags file
	// contains user data, so let’s leave users a chance to recover it.
	remove(tags_path.c_str());
}

/**
 * Main loop of opustags. Read the packets from the reader, and forwards them to the writer.
 * Transform the OpusTags packet on the fly.
 *
 * The writer is optional. When writer is nullptr, opustags runs in read-only mode.
 */
static void process(ot::ogg_reader& reader, ot::ogg_writer* writer, const ot::options &opt)
{
	bool focused = false; /*< the stream on which we operate is defined */
	int focused_serialno; /*< when focused, the serialno of the focused stream */

	/** When the number of pages the OpusTags packet takes differs from the input stream to the
	 *  output stream, we need to renumber all the succeeding pages. If the input stream
	 *  contains gaps, the offset will naively reproduce the gaps: page numbers 0 (1) 2 4 will
	 *  become 0 (1 2) 3 5, where (…) is the OpusTags packet, and not 0 (1 2) 3 4. */
	int pageno_offset = 0;

	while (reader.next_page()) {
		auto serialno = ogg_page_serialno(&reader.page);
		auto pageno = ogg_page_pageno(&reader.page);
		if (!focused) {
			focused = true;
			focused_serialno = serialno;
		} else if (serialno != focused_serialno) {
			/** \todo Support mixed streams. */
			throw ot::status {ot::st::error, "Muxed streams are not supported yet."};
		}
		if (reader.absolute_page_no == 0) { // Identification header
			if (!ot::is_opus_stream(reader.page))
				throw ot::status {ot::st::error, "Not an Opus stream."};
			if (writer)
				writer->write_page(reader.page);
		} else if (reader.absolute_page_no == 1) { // Comment header
			ot::opus_tags tags;
			reader.process_header_packet([&tags](ogg_packet& p) { tags = ot::parse_tags(p); });
			edit_tags(tags, opt);
			if (writer) {
				if (opt.edit_interactively) {
					fflush(writer->file); // flush before calling the subprocess
					edit_tags_interactively(tags, writer->path, opt.raw);
				}
				auto packet = ot::render_tags(tags);
				writer->write_header_packet(serialno, pageno, packet);
				pageno_offset = writer->next_page_no - 1 - reader.absolute_page_no;
			} else {
				ot::print_comments(tags.comments, stdout, opt.raw);
				break;
			}
		} else if (writer) {
			ot::renumber_page(reader.page, pageno + pageno_offset);
			writer->write_page(reader.page);
		}
	}
	if (reader.absolute_page_no < 1)
		throw ot::status {ot::st::error, "Expected at least 2 Ogg pages."};
}

static void run_single(const ot::options& opt, const std::string& path_in, const std::optional<std::string>& path_out)
{
	ot::file input;
	if (path_in == "-")
		input = stdin;
	else if ((input = fopen(path_in.c_str(), "re")) == nullptr)
		throw ot::status {ot::st::standard_error,
		                  "Could not open '" + path_in + "' for reading: " + strerror(errno)};
	ot::ogg_reader reader(input.get());

	/* Read-only mode. */
	if (!path_out) {
		process(reader, nullptr, opt);
		return;
	}

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

	struct stat output_info;
	if (path_out == "-") {
		output = stdout;
	} else if (stat(path_out->c_str(), &output_info) == 0) {
		/* The output file exists. */
		if (!S_ISREG(output_info.st_mode)) {
			/* Special files are opened for writing directly. */
			if ((final_output = fopen(path_out->c_str(), "we")) == nullptr)
				throw ot::status {ot::st::standard_error,
				                  "Could not open '" + path_out.value() + "' for writing: " + strerror(errno)};
			output = final_output.get();
		} else if (opt.overwrite) {
			temporary_output.open(path_out->c_str());
			output = temporary_output.get();
		} else {
			throw ot::status {ot::st::error, "'" + path_out.value() + "' already exists. Use -y to overwrite."};
		}
	} else if (errno == ENOENT) {
		temporary_output.open(path_out->c_str());
		output = temporary_output.get();
	} else {
		throw ot::status {ot::st::error, "Could not identify '" + path_in + "': " + strerror(errno)};
	}

	ot::ogg_writer writer(output);
	writer.path = path_out;
	process(reader, &writer, opt);
	temporary_output.commit();
}

void ot::run(const ot::options& opt)
{
	if (opt.print_help) {
		fputs(help_message, stdout);
		return;
	}

	ot::status global_rc = st::ok;
	for (const auto& path_in : opt.paths_in) {
		try {
			run_single(opt, path_in, opt.in_place ? path_in : opt.path_out);
		} catch (const ot::status& rc) {
			global_rc = st::error;
			if (!rc.message.empty())
				fprintf(stderr, "%s: error: %s\n", path_in.c_str(), rc.message.c_str());
		}
	}
	if (global_rc != st::ok)
		throw global_rc;
}
