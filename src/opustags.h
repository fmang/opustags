/**
 * \file src/opustags.h
 *
 * Welcome to opustags!
 *
 * Let's have a quick tour around. The project is split into the following modules:
 *
 * - The system module provides a few generic tools for interating with the system.
 * - The ogg module reads and writes Ogg files, letting you manipulate Ogg pages and packets.
 * - The opus module parses the contents of Ogg packets according to the Opus specifications.
 * - The cli module implements the main logic of the program.
 * - The opustags module contains the main function, which is a simple wrapper around cli.
 *
 * Each module is implemented in its eponymous .cc file. Their interfaces are all defined and
 * documented together in this header file. Look into the .cc files for implementation-specific
 * details.
 *
 * To understand how this program works, you need to know what an Ogg files is made of, in
 * particular the streams, pages, and packets. You hardly need any knowledge of the actual Opus
 * audio codec, but need the RFC 7845 "Ogg Encapsulation for the Opus Audio Codec" that defines the
 * format of the header packets that are essential to opustags.
 *
 */

#pragma once

#include <config.h>

#include <iconv.h>
#include <ogg/ogg.h>
#include <stdio.h>
#include <time.h>

#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef HAVE_ENDIAN_H
#  include <endian.h>
#endif

#ifdef HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
#endif

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#endif

namespace ot {

/**
 * Possible return status code, ranging from errors to special statuses. They are usually
 * accompanied with a message with the #status structure.
 *
 * Error codes do not need to be ultra specific, and are mainly used to report special conditions to
 * the caller function. Ultimately, only the error message in the #status is shown to the user.
 *
 * The cut error family means that the end of packet was reached when attempting to read the
 * overflowing value. For example, cut_comment_count means that after reading the vendor string,
 * less than 4 bytes were left in the packet.
 */
enum class st {
	/* Generic */
	ok,
	error,
	standard_error, /**< Error raised by the C standard library. */
	int_overflow,
	cancel,
	/* System */
	badly_encoded,
	child_process_failed,
	/* Ogg */
	bad_stream,
	libogg_error,
	/* Opus */
	bad_magic_number,
	cut_magic_number,
	cut_vendor_length,
	cut_vendor_data,
	cut_comment_count,
	cut_comment_length,
	cut_comment_data,
	/* CLI */
	bad_arguments,
};

/**
 * Wraps a status code with an optional message. It is implictly converted to and from a
 * #status_code. It may be thrown on error by any of the ot:: functions.
 *
 * All the statuses except #st::ok should be accompanied with a relevant error message, in case it
 * propagates back to the main function and is shown to the user.
 */
struct status {
	status(st code = st::ok) : code(code) {}
	template<class T> status(st code, T&& message) : code(code), message(message) {}
	operator st() const { return code; }
	st code;
	std::string message;
};

/***********************************************************************************************//**
 * \defgroup system System
 * \{
 */

/**
 * Smart auto-closing FILE* handle.
 *
 * It implictly converts from an already opened FILE*.
 */
struct file : std::unique_ptr<FILE, decltype(&fclose)> {
	file(FILE* f = nullptr) : std::unique_ptr<FILE, decltype(&fclose)>(f, &fclose) {}
};

/**
 * A partial file is a temporary file created to store the result of something. When it is complete,
 * it is moved to a final destination. Open it with #open and then you can either #commit it to save
 * it to its destination, or you can #abort to delete the temporary file. When the #partial_file
 * object is destroyed, it deletes the currently opened temporary file, if any.
 */
class partial_file {
public:
	~partial_file() { abort(); }
	/**
	 * Open a temporary file meant to be moved to the specified destination file path. The
	 * temporary file is created in the same directory as its destination in order to make the
	 * final move operation instant.
	 */
	void open(const char* destination);
	/** Close then move the partial file to its final location. */
	void commit();
	/** Delete the temporary file. */
	void abort();
	/** Get the underlying FILE* handle. */
	FILE* get() { return file.get(); }
	/** Get the name of the temporary file. */
	const char* name() const { return file == nullptr ? nullptr : temporary_name.c_str(); }
private:
	std::string temporary_name;
	std::string final_name;
	ot::file file;
};

/** C++ wrapper for iconv. */
class encoding_converter {
public:
	/**
	 * Allocate the iconv conversion state, initializing the given source and destination
	 * character encodings. If it's okay to have some information lost, make sure `to` ends with
	 * "//TRANSLIT", otherwise the conversion will fail when a character cannot be represented
	 * in the target encoding. See the documentation of iconv_open for details.
	 */
	encoding_converter(const char* from, const char* to);
	~encoding_converter();
	/**
	 * Convert text using iconv. If the input sequence is invalid, return #st::badly_encoded and
	 * abort the processing, leaving out in an undefined state.
	 */
	std::string operator()(std::string_view in);
private:
	iconv_t cd; /**< conversion descriptor */
};

/** Escape a string so that a POSIX shell interprets it as a single argument. */
std::string shell_escape(std::string_view word);

/**
 * Execute the editor process specified in editor. Wait for the process to exit and
 * return st::ok on success, or st::child_process_failed if it did not exit with 0.
 *
 * editor is passed unescaped to the shell, and may contain CLI options.
 * path is the name of the file to edit, which will be passed as the last argument to editor.
 */
void run_editor(std::string_view editor, std::string_view path);

/**
 * Return the specified path’s mtime, i.e. the last data modification
 * timestamp.
 */
timespec get_file_timestamp(const char* path);

/** \} */

/***********************************************************************************************//**
 * \defgroup ogg Ogg
 * \{
 */

/**
 * RAII-aware wrapper around libogg's ogg_stream_state. Though it handles automatic destruction, it
 * does not prevent copying or implement move semantics correctly, so it's your responsibility to
 * ensure these operations don't happen.
 */
struct ogg_logical_stream : ogg_stream_state {
	ogg_logical_stream(int serialno) {
		if (ogg_stream_init(this, serialno) != 0)
			throw std::bad_alloc();
	}
	~ogg_logical_stream() {
		ogg_stream_clear(this);
	}
};

/**
 * Identify the codec of a logical stream based on the first bytes of the first packet of the first
 * page. For Opus, the first 8 bytes must be OpusHead. Any other signature is assumed to be another
 * codec.
 */
bool is_opus_stream(const ogg_page& identification_header);

/**
 * Ogg reader, combining a FILE input, an ogg_sync_state reading the pages.
 *
 * Call #read_page repeatedly until it returns false to consume the stream, and use #page to check
 * its content.
 */
struct ogg_reader {
	/**
	 * Initialize the reader with the given input file handle. The caller is responsible for
	 * keeping the file handle alive, and to close it.
	 */
	ogg_reader(FILE* input) : file(input) { ogg_sync_init(&sync); }
	/**
	 * Clear all the internal memory allocated by libogg for the sync and stream state. The
	 * page and the packet are owned by these states, so nothing to do with them.
	 *
	 * The input file is not closed.
	 */
	~ogg_reader() { ogg_sync_clear(&sync); }
	/**
	 * Read the next page from the input file. The result is made available in the #page field,
	 * is owned by the Ogg reader, and is valid until the next call to #read_page.
	 *
	 * Return true if a page was read, false on end of stream.
	 */
	bool next_page();
	/**
	 * Read the single packet contained in the last page read, assuming it's a header page, and
	 * call the function f on it. This function has no side effect, and calling it twice on the
	 * same page will read the same packet again.
	 *
	 * It is currently limited to packets that fit on a single page, and should be later
	 * extended to support packets spanning multiple pages.
	 */
	void process_header_packet(const std::function<void(ogg_packet&)>& f);
	/**
	 * Current page from the sync state.
	 *
	 * Its memory is managed by libogg, inside the sync state, and is valid until the next call
	 * to ogg_sync_pageout, wrapped by #read_page.
	 */
	ogg_page page;
	/**
	 * Page number in the physical stream of the last read page, disregarding multiplexed
	 * streams. The first page number is 0. When no page has been read, its value is
	 * (size_t) -1.
	 */
	size_t absolute_page_no = -1;
	/**
	 * The file is our source of binary data. It is not integrated to libogg, so we need to
	 * handle it ourselves.
	 *
	 * The file is not owned by the ogg_reader instance.
	 */
	FILE* file;
	/**
	 * The sync layer gets binary data and yields a sequence of pages.
	 *
	 * A page contains packets that we can extract using the #stream state, but we only do that
	 * for the headers. Once we got the OpusHead and OpusTags packets, all the following pages
	 * are simply forwarded to the Ogg writer.
	 */
	ogg_sync_state sync;
};

/**
 * An Ogg writer lets you write ogg_page objets to an output file, and assemble packets into pages.
 *
 * Its packet writing facility is limited to writing single-page header packets, because that's all
 * we need for opustags.
 */
struct ogg_writer {
	/**
	 * Initialize the writer with the given output file handle. The caller is responsible for
	 * keeping the file handle alive, and to close it.
	 */
	explicit ogg_writer(FILE* output) : file(output) {}
	/**
	 * Write a whole Ogg page into the output stream.
	 *
	 * This is a basic I/O operation and does not even require libogg, or the stream.
	 */
	void write_page(const ogg_page& page);
	/**
	 * Write a header packet and flush the page. Header packets are always placed alone on their
	 * pages.
	 */
	void write_header_packet(int serialno, int pageno, ogg_packet& packet);
	/**
	 * Output file. It should be opened in binary mode. We use it to write whole pages,
	 * represented as a block of data and a length.
	 */
	FILE* file;
	/**
	 * Path to the output file.
	 */
	std::optional<std::string> path;
	/**
	 * Custom counter for the sequential page number to be written. It allows us to detect
	 * ogg_page_pageno mismatches and renumber the pages if needed.
	 */
	long next_page_no = 0;
};

/**
 * Ogg packet with dynamically allocated data.
 *
 * Provides a wrapper around libogg's ogg_packet with RAII.
 */
struct dynamic_ogg_packet : ogg_packet {
	/** Construct an ogg_packet of the given size. */
	explicit dynamic_ogg_packet(size_t size) {
		bytes = size;
		data = std::make_unique<unsigned char[]>(size);
		packet = data.get();
	}
private:
	/** Owning reference to the data. Use the packet field from ogg_packet instead. */
	std::unique_ptr<unsigned char[]> data;
};

/** Update the Ogg pageno field in the given page. The CRC is recomputed if needed. */
void renumber_page(ogg_page& page, long new_pageno);

/** \} */

/***********************************************************************************************//**
 * \defgroup opus Opus
 * \{
 */

/**
 * Faithfully represent *all* the data in an OpusTags packet, exactly as they will be written in the
 * final stream, disregarding the current system locale or anything else.
 *
 * The vendor and comment strings are expected to contain valid UTF-8, but we should keep their
 * values intact even if the string is not UTF-8 clean, or encoded in any other way.
 */
struct opus_tags {
	/**
	 * OpusTags packets begin with a vendor string, meant to identify the implementation of the
	 * encoder. It is expected to be an arbitrary UTF-8 string.
	 */
	std::string vendor;
	/**
	 * Comments are strings in the NAME=Value format. A comment may also be called a field, or a
	 * tag.
	 *
	 * The field name in vorbis comments is usually case-insensitive and ASCII, while the value
	 * can be any valid UTF-8 string. The specification is not too clear for Opus, but let's
	 * assume it's the same.
	 */
	std::list<std::string> comments;
	/**
	 * According to RFC 7845:
	 * > Immediately following the user comment list, the comment header MAY contain
	 * > zero-padding or other binary data that is not specified here.
	 *
	 * The first byte is supposed to indicate whether this data should be kept or not, but let's
	 * assume it's here for a reason and always keep it. Better safe than sorry.
	 *
	 * In the future, we could add options to manipulate this data: view it, edit it, truncate
	 * it if it's marked as padding, truncate it unconditionally.
	 */
	std::string extra_data;
};

/**
 * Read the given OpusTags packet and extract its content into an opus_tags object.
 */
opus_tags parse_tags(const ogg_packet& packet);

/**
 * Serialize an #opus_tags object into an OpusTags Ogg packet.
 */
dynamic_ogg_packet render_tags(const opus_tags& tags);

/** \} */

/***********************************************************************************************//**
 * \defgroup cli Command-Line Interface
 * \{
 */

/**
 * Structured representation of the command-line arguments to opustags.
 */
struct options {
	/**
	 * When true, opustags prints a detailed help and exits. All the other options are ignored.
	 *
	 * Option: --help
	 */
	bool print_help = false;
	/**
	 * Paths to the input files. The special string "-" means stdin.
	 *
	 * At least one input file must be given. If `--in-place` is used,
	 * more than one may be given.
	 */
	std::vector<std::string> paths_in;
	/**
	 * Optional path to output file. The special string "-" means stdout. For in-place
	 * editing, the input file name is used. If no output file name is supplied, and
	 * --in-place is not used, opustags runs in read-only mode.
	 *
	 * Options: --output, --in-place
	 */
	std::optional<std::string> path_out;
	/**
	 * By default, opustags won't overwrite the output file if it already exists. This can be
	 * forced with --overwrite. It is also enabled by --in-place.
	 *
	 * Options: --overwrite, --in-place
	 */
	bool overwrite = false;
	/**
	 * Process files in-place.
	 *
	 * Options: --in-place
	 */
	bool in_place = false;
	/**
	 * Spawn EDITOR to edit tags interactively.
	 *
	 * stdin and stdout must be left free for the editor, so paths_in and
	 * path_out can’t take `-`, and --set-all is not supported.
	 *
	 * Option: --edit
	 */
	bool edit_interactively = false;
	/**
	 * List of comments to delete. Each string is a selector according to the definition of
	 * #delete_comments.
	 *
	 * When #delete_all is true, this option is meaningless.
	 *
	 * #to_add takes precedence over #to_delete, so if the same comment appears in both lists,
	 * the one in #to_delete applies only to the previously existing tags.
	 *
	 * The strings are stored in UTF-8.
	 *
	 * Option: --delete, --set
	 */
	std::list<std::string> to_delete;
	/**
	 * Delete all the existing comments.
	 *
	 * Option: --delete-all, --set-all
	 */
	bool delete_all = false;
	/**
	 * List of comments to add, in the current system encoding. For exemple `TITLE=a b c`. They
	 * must be valid.
	 *
	 * The strings are stored in UTF-8.
	 *
	 * Options: --add, --set, --set-all
	 */
	std::list<std::string> to_add;
	/**
	 * Disable encoding conversions. OpusTags are specified to always be encoded as UTF-8, but
	 * if for some reason a specific file contains binary tags that someone would like to
	 * extract and set as-is, encoding conversion would get in the way.
	 */
	bool raw = false;
};

/**
 * Parse the command-line arguments. Does not perform I/O related validations, but checks the
 * consistency of its arguments. Comments are read if necessary from the given stream.
 */
options parse_options(int argc, char** argv, FILE* comments);

/**
 * Print all the comments, separated by line breaks. Since a comment may contain line breaks, this
 * output is not completely reliable, but it fits most cases.
 *
 * The comments must be encoded in UTF-8, and are converted to the system locale when printed,
 * unless raw is true.
 *
 * The output generated is meant to be parseable by #ot::read_comments.
 */
void print_comments(const std::list<std::string>& comments, FILE* output, bool raw);

/**
 * Parse the comments outputted by #ot::print_comments. Unless raw is true, the comments are
 * converted from the system encoding to UTF-8, and returned as UTF-8.
 */
std::list<std::string> read_comments(FILE* input, bool raw);

/**
 * Remove all comments matching the specified selector, which may either be a field name or a
 * NAME=VALUE pair. The field name is case-insensitive.
 *
 * The strings are all UTF-8.
 */
void delete_comments(std::list<std::string>& comments, const std::string& selector);

/**
 * Main entry point to the opustags program, and pretty much the same as calling opustags from the
 * command-line.
 */
void run(const options& opt);

/** \} */

}
