/**
 * \file src/opustags.h
 *
 * Welcome to opustags!
 *
 * Let's have a quick tour around. The project is split into the following modules:
 *
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

#include <ogg/ogg.h>
#include <stdio.h>

#include <list>
#include <memory>
#include <string>
#include <vector>

namespace ot {

/**
 * Possible return status code, ranging from errors to special statuses. They are usually
 * accompanied with a message with the #status structure.
 *
 * Functions that return non-ok status codes to signal special conditions like #end_of_stream should
 * have it explictly mentionned in their documentation. By default, a non-ok status should be
 * handled like an error.
 *
 * The cut error family means that the end of packet was reached when attempting to read the
 * overflowing value. For example, cut_comment_count means that after reading the vendor string,
 * less than 4 bytes were left in the packet.
 */
enum class st {
	/* Generic */
	ok,
	int_overflow,
	standard_error,
	/* Ogg */
	end_of_stream,
	end_of_page,
	stream_not_ready,
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
	fatal_error,
};

/**
 * Wraps a status code with an optional message. It is implictly converted to and from a
 * #status_code.
 *
 * All the statuses except #st::ok should be accompanied with a relevant error message, in case it
 * propagates back to the main function and is shown to the user.
 */
struct status {
	status(st code = st::ok) : code(code) {}
	template<class T> status(st code, T&& message) : code(code), message(message) {}
	operator st() { return code; }
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
	ot::status open(const char* destination);
	/** Close then move the partial file to its final location. */
	ot::status commit();
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

/** \} */

/***********************************************************************************************//**
 * \defgroup ogg Ogg
 * \{
 */

/**
 * Ogg reader, combining a FILE input, an ogg_sync_state reading the pages, and an ogg_stream_state
 * extracting the packets from the page.
 *
 * Call #read_page repeatedly until #status::end_of_stream to consume the stream, and use #page to
 * check its content. To extract its packets, call #read_packet until #status::end_of_packet.
 */
class ogg_reader {
public:
	/**
	 * Initialize the reader with the given input file handle. The caller is responsible for
	 * keeping the file handle alive, and to close it.
	 */
	ogg_reader(FILE* input);
	/**
	 * Clear all the internal memory allocated by libogg for the sync and stream state. The
	 * page and the packet are owned by these states, so nothing to do with them.
	 *
	 * The input file is not closed.
	 */
	~ogg_reader();
	/**
	 * Read the next page from the input file. The result, provided the status is #status::ok,
	 * is made available in the #page field, is owned by the Ogg reader, and is valid until the
	 * next call to #read_page.
	 *
	 * After the last page was read, return #status::end_of_stream.
	 */
	status read_page();
	/**
	 * Read the next available packet from the current #page. The packet is made available in
	 * the #packet field.
	 *
	 * No packet can be read until a page has been loaded with #read_page. If that happens,
	 * return #status::stream_not_ready.
	 *
	 * After the last packet was read, return #status::end_of_page.
	 */
	status read_packet();
	/**
	 * Current page from the sync state.
	 *
	 * Its memory is managed by libogg, inside the sync state, and is valid until the next call
	 * to ogg_sync_pageout, wrapped by #read_page.
	 */
	ogg_page page;
	/**
	 * Current packet from the stream state.
	 *
	 * Its memory is managed by libogg, inside the stream state, and is valid until the next
	 * call to ogg_stream_packetout, wrapped by #read_packet.
	 */
	ogg_packet packet;
private:
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
	/**
	 * Indicates whether the stream has been initialized or not.
	 *
	 * To initialize it properly, we need the serialno of the stream, which is available only
	 * after the first page was read.
	 */
	bool stream_ready = false;
	/**
	 * Indicates if the stream's last fed page is the current one.
	 *
	 * Its state is irrelevant if the stream is not ready.
	 */
	bool stream_in_sync;
	/**
	 * The stream layer receives pages and yields a sequence of packets.
	 *
	 * A single page may contain several packets, and a single packet may span on multiple
	 * pages. The 2 packets we're interested in occupy whole pages though, in theory, but we'd
	 * better ensure there are no extra packets anyway.
	 *
	 * After we've read OpusHead and OpusTags, we don't need the stream layer anymore.
	 */
	ogg_stream_state stream;
};

/**
 * An Ogg writer lets you write ogg_page objets to an output file, and assemble packets into pages.
 *
 * It has two modes of operations :
 *   1. call #write_page, or
 *   2. call #prepare_stream, then #write_packet one or more times, followed by #flush_page.
 *
 * You can switch between the two modes, but must not start writing packets and then pages without
 * flushing.
 */
class ogg_writer {
public:
	/**
	 * Initialize the writer with the given output file handle. The caller is responsible for
	 * keeping the file handle alive, and to close it.
	 */
	ogg_writer(FILE* output);
	/**
	 * Clears the stream state and any internal memory. Does not close the output file.
	 */
	~ogg_writer();
	/**
	 * Write a whole Ogg page into the output stream.
	 *
	 * This is a basic I/O operation and does not even require libogg, or the stream.
	 */
	status write_page(const ogg_page& page);
	/**
	 * Prepare the stream with the given Ogg serial number.
	 *
	 * If the stream is already configured with the right serial number, it doesn't do anything
	 * and is cheap to call.
	 *
	 * If the stream contains unflushed packets, they will be lost.
	 */
	status prepare_stream(long serialno);
	/**
	 * Add a packet to the current page under assembly.
	 *
	 * If the packet is coming from a different page, make sure the serial number fits by
	 * calling #prepare_stream.
	 *
	 * When the page is complete, you should call #flush_page to finalize the page.
	 *
	 * You must not call #write_page after it, until you call #flush_page.
	 */
	status write_packet(const ogg_packet& packet);
	/**
	 * Write the page under assembly. Future calls to #write_packet will be written in a new
	 * page.
	 */
	status flush_page();
private:
	/**
	 * The stream state receives packets and generates pages.
	 *
	 * In our specific use case, we only need it to put the OpusHead and OpusTags packets into
	 * their own pages. The other pages are naively written to the output stream.
	 */
	ogg_stream_state stream;
	/**
	 * Output file. It should be opened in binary mode. We use it to write whole pages,
	 * represented as a block of data and a length.
	 */
	FILE* file;
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
	 * encoder. It should be an arbitrary UTF-8 string.
	 */
	std::string vendor;
	/**
	 * Comments. These are a list of string following the NAME=Value format.  A comment may also
	 * be called a field, or a tag.
	 *
	 * The field name in vorbis comment is case-insensitive and ASCII, while the value can be
	 * any valid UTF-8 string. The specification is not too clear for Opus, but let's assume
	 * it's the same.
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
 * Validate the content of the first packet of an Ogg stream to ensure it's a valid OpusHead.
 *
 * Returns #ot::status::ok on success, #ot::status::bad_identification_header on error.
 */
status validate_identification_header(const ogg_packet& packet);

/**
 * Read the given OpusTags packet and extract its content into an opus_tags object.
 *
 * On error, the tags object is left unchanged.
 */
status parse_tags(const ogg_packet& packet, opus_tags& tags);

/**
 * Serialize an #opus_tags object into an OpusTags Ogg packet.
 */
dynamic_ogg_packet render_tags(const opus_tags& tags);

/**
 * Remove all the comments whose field name is equal to the special one, case-sensitive.
 */
void delete_comments(opus_tags& tags, const char* field_name);

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
	 * Path to the input file. It cannot be empty. The special "-" string means stdin.
	 *
	 * This is the mandatory non-flagged parameter.
	 */
	std::string path_in;
	/**
	 * Path to the optional file. The special "-" string means stdout. When empty, opustags runs
	 * in read-only mode. For in-place editing, path_out is defined equal to path_in.
	 *
	 * Options: --output, --in-place
	 */
	std::string path_out;
	/**
	 * By default, opustags won't overwrite the output file if it already exists. This can be
	 * forced with --overwrite. It is also enabled by --in-place.
	 *
	 * Options: --overwrite, --in-place
	 */
	bool overwrite = false;
	/**
	 * List of field names to delete. `{"ARTIST"}` will delete *all* the comments `ARTIST=*`. It
	 * is currently case-sensitive. When #delete_all is true, it becomes meaningless.
	 *
	 * #to_add takes precedence over #to_delete, so if the same comment appears in both lists,
	 * the one in #to_delete applies only to the previously existing tags.
	 *
	 * \todo Consider making it case-insensitive.
	 * \todo Allow values like `ARTIST=x` to delete only the ARTIST comment whose value is x.
	 *
	 * Option: --delete, --set
	 */
	std::vector<std::string> to_delete;
	/**
	 * Delete all the existing comments.
	 *
	 * Option: --delete-all
	 */
	bool delete_all = false;
	/**
	 * List of comments to add, in the current system encoding. For exemple `TITLE=a b c`. They
	 * must be valid.
	 *
	 * Options: --add, --set, --set-all
	 */
	std::vector<std::string> to_add;
	/**
	 * Replace the previous comments by the ones supplied by the user.
	 *
	 * Read a list of comments from stdin and populate #to_add. Further comments may be added
	 * with the --add option.
	 *
	 * Option: --set-all
	 */
	bool set_all = false;
};

/**
 * Parse the command-line arguments. Does not perform I/O related validations, but checks the
 * consistency of its arguments.
 *
 * On error, the state of the options structure is unspecified.
 */
status parse_options(int argc, char** argv, options& opt);

/**
 * Print all the comments, separated by line breaks. Since a comment may
 * contain line breaks, this output is not completely reliable, but it fits
 * most cases.
 *
 * The output generated is meant to be parseable by #ot::read_tags.
 */
void print_comments(const std::list<std::string>& comments, FILE* output);

/**
 * Parse the comments outputted by #ot::print_comments.
 */
std::list<std::string> read_comments(FILE* input);

/**
 * Main entry point to the opustags program, and pretty much the same as calling opustags from the
 * command-line.
 */
status run(const options& opt);

/** \} */

}
