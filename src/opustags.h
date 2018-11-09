/**
 * \file src/opustags.h
 * \brief Interface of all the submodules of opustags.
 */

#include <ogg/ogg.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <list>

namespace ot {

/**
 * Non-owning string.
 *
 * The interface is meant to be compatible with std::string_view from C++17, so
 * that we'll be able to delete it as soon as C++17 is widely supported.
 */
class string_view {
public:
	string_view() {}
	string_view(const char *data) : _data(data), _size(strlen(data)) {}
	string_view(const char *data, size_t size) : _data(data), _size(size) {}
	const char *data() const { return _data; }
	size_t size() const { return _size; }
	bool operator==(const string_view &other) const {
		return _size == other._size && memcmp(_data, other._data, _size) == 0;
	}
	bool operator!=(const string_view &other) const { return !(*this == other); }
private:
	const char *_data = nullptr;
	size_t _size = 0;
};

/**
 * \defgroup ogg Ogg
 * \brief Helpers to work with libogg.
 *
 * \{
 */

struct ogg_reader {
	/**
	 * The file is our source of binary data. It is not integrated to libogg, so we need to
	 * handle it ourselves.
	 *
	 * In the future, we should use an std::istream or something.
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
	 * Current page from the sync state.
	 *
	 * Its memory is managed by libogg, inside the sync state, and is valid until the next call
	 * to ogg_sync_pageout.
	 */
	ogg_page page;
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
	/**
	 * Current packet from the stream state.
	 *
	 * Its memory is managed by libogg, inside the stream state, and is valid until the next
	 * call to ogg_stream_packetout.
	 */
	ogg_packet packet;
};

struct ogg_writer {
	/**
	 * The stream state receives packets and generates pages.
	 *
	 * We only need it to put the OpusHead and OpusTags packets into their own pages. The other
	 * pages are naively written to the output stream.
	 */
	ogg_stream_state stream;
	/**
	 * Output file. It should be opened in binary mode. We use it to write whole pages,
	 * represented as a block of data and a length.
	 */
	FILE* file;
};

int write_page(ogg_page *og, FILE *stream);

/** \} */

/**
 * \defgroup opus Opus
 * \brief Opus packet decoding and recoding.
 *
 * \{
 */

/**
 * Represent all the data in an OpusTags packet.
 */
struct opus_tags {
	/**
	 * OpusTags packets begin with a vendor string, meant to identify the
	 * implementation of the encoder. It is an arbitrary UTF-8 string.
	 */
	string_view vendor;
	/**
	 * Comments. These are a list of string following the NAME=Value format.
	 * A comment may also be called a field, or a tag.
	 *
	 * The field name in vorbis comment is case-insensitive and ASCII,
	 * while the value can be any valid UTF-8 string.
	 */
	std::list<string_view> comments;
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
	string_view extra_data;
};

/**
 * Possible return status of #parse_tags.
 *
 * The overflowing error family means that the end of packet was reached when
 * attempting to read the overflowing value. For example,
 * overflowing_comment_count means that after reading the vendor string, less
 * than 4 bytes were left in the packet.
 */
enum class parse_result {
	ok = 0,
	bad_magic_number = -100,
	overflowing_magic_number,
	overflowing_vendor_length,
	overflowing_vendor_data,
	overflowing_comment_count,
	overflowing_comment_length,
	overflowing_comment_data,
};

parse_result parse_tags(const char *data, long len, opus_tags *tags);
int render_tags(opus_tags *tags, ogg_packet *op);
void delete_tags(opus_tags *tags, const char *field);

/** \} */

}
