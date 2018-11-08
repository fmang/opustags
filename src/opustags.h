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
