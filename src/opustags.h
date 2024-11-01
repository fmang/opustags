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
#define htobe32(x) OSSwapHostToBigInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#endif

using namespace std::literals;

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
	invalid_size,
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

using byte_string = std::basic_string<uint8_t>;
using byte_string_view = std::basic_string_view<uint8_t>;

/***********************************************************************************************//**
 * \defgroup system System
 * \{
 */

/** fclose wrapper for std::unique_ptr’s deleter. */
void close_file(FILE*);

/**
 * Smart auto-closing FILE* handle.
 *
 * It implictly converts from an already opened FILE*.
 */
struct file : std::unique_ptr<FILE, decltype(&close_file)> {
	file(FILE* f = nullptr) : std::unique_ptr<FILE, decltype(&close_file)>(f, &close_file) {}
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

/** Read a whole file into memory and return the read content. */
byte_string slurp_binary_file(const char* filename);

/** Convert a string from the system locale’s encoding to UTF-8. */
std::u8string encode_utf8(std::string_view);

/** Convert a string from UTF-8 to the system locale’s encoding. */
std::string decode_utf8(std::u8string_view);

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

std::u8string encode_base64(byte_string_view src);
byte_string decode_base64(std::u8string_view src);

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
	std::u8string vendor;
	/**
	 * Comments are strings in the NAME=Value format. A comment may also be called a field, or a
	 * tag.
	 *
	 * The field name in vorbis comments is usually case-insensitive and ASCII, while the value
	 * can be any valid UTF-8 string. The specification is not too clear for Opus, but let's
	 * assume it's the same.
	 */
	std::list<std::u8string> comments;
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
	byte_string extra_data;
};

/**
 * Read the given OpusTags packet and extract its content into an opus_tags object.
 */
opus_tags parse_tags(const ogg_packet& packet);

/**
 * Serialize an #opus_tags object into an OpusTags Ogg packet.
 */
dynamic_ogg_packet render_tags(const opus_tags& tags);

/**
 * Extracted data from the METADATA_BLOCK_PICTURE tag. See
 * <https://xiph.org/flac/format.html#metadata_block_picture> for the full specifications.
 *
 * It may contain all kinds of metadata but most are not used at all. For now, let’s assume all
 * pictures have picture type 3 (front cover), and empty metadata.
 */
struct picture {
	picture() = default;

	/** Extract the picture information from serialized binary data.*/
	picture(byte_string block);
	byte_string_view mime_type;
	byte_string_view picture_data;

	/**
	 * Encode the picture attributes (mime_type, picture_data) into a binary block to be stored
	 * into METADATA_BLOCK_PICTURE.
	 */
	byte_string serialize() const;

	/** To avoid needless copies of the picture data, move the original data block there. The
	 *  string_view attributes will refer to it. */
	byte_string storage;
};

/** Extract the first picture embedded in the tags, regardless of its type. */
std::optional<picture> extract_cover(const opus_tags& tags);

/**
 * Return a METADATA_BLOCK_PICTURE tag defining the front cover art to the given picture data (JPEG,
 * PNG). The MIME type is deduced from the magic number.
 */
std::u8string make_cover(byte_string_view picture_data);

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
	 * Option: --delete, --set
	 */
	std::list<std::u8string> to_delete;
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
	 * Options: --add, --set, --set-all
	 */
	std::list<std::u8string> to_add;
	/**
	 * If set, the input file’s cover art is exported to the specified file. - for stdout. Does
	 * not overwrite the file if it already exists unless -y is specified. Does nothing if the
	 * input file does not contain a cover art.
	 *
	 * Option: --output-cover
	 */
	std::optional<std::string> cover_out;
	/**
	 * Print the vendor string at the beginning of the OpusTags packet instead of printing the
	 * tags. Only applicable in read-only mode.
	 *
	 * Option: --vendor
	 */
	bool print_vendor = false;
	/**
	 * Replace the vendor string by the one specified by the user.
	 *
	 * Option: --set-vendor
	 */
	std::optional<std::u8string> set_vendor;
	/**
	 * Disable encoding conversions. OpusTags are specified to always be encoded as UTF-8, but
	 * if for some reason a specific file contains binary tags that someone would like to
	 * extract and set as-is, encoding conversion would get in the way.
	 */
	bool raw = false;
	/**
	 * In text mode (default), tags are separated by a line feed. However, when combining
	 * opustags with grep or other line-based tools, this proves to be a bad separator because
	 * tag values may contain newlines. Changing the delimiter to '\0' with -z eases the
	 * processing of multi-line tags with other tools that support null-terminated lines.
	 */
	char tag_delimiter = '\n';
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
void print_comments(const std::list<std::u8string>& comments, FILE* output, const options& opt);

/**
 * Parse the comments outputted by #ot::print_comments. Unless raw is true, the comments are
 * converted from the system encoding to UTF-8, and returned as UTF-8.
 */
std::list<std::u8string> read_comments(FILE* input, const options& opt);

/**
 * Remove all comments matching the specified selector, which may either be a field name or a
 * NAME=VALUE pair. The field name is case-insensitive.
 */
void delete_comments(std::list<std::u8string>& comments, const std::u8string& selector);

/**
 * Main entry point to the opustags program, and pretty much the same as calling opustags from the
 * command-line.
 */
void run(const options& opt);

/** \} */

}

/** Handy literal suffix for building byte strings. */
ot::byte_string operator""_bs(const char* data, size_t size);
ot::byte_string_view operator""_bsv(const char* data, size_t size);
