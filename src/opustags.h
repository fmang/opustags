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
 * Non-owning string, similar to what std::string_view is in C++17.
 */
struct string_view {
	string_view() {};
	string_view(const char *data) : data(data), size(strlen(data)) {};
	string_view(const char *data, size_t size) : data(data), size(size) {}
	bool operator==(const string_view &other) const {
		return size == other.size && memcmp(data, other.data, size) == 0;
	}
	bool operator!=(const string_view &other) const { return !(*this == other); }
	const char *data = nullptr;
	size_t size = 0;
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
	uint32_t vendor_length;
	/** \todo Convert to a string view. */
	const char *vendor_string;
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

int parse_tags(const char *data, long len, opus_tags *tags);
int render_tags(opus_tags *tags, ogg_packet *op);
void delete_tags(opus_tags *tags, const char *field);
int add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count);
void print_tags(opus_tags *tags);

/** \} */

}
