/**
 * \file src/opustags.h
 * \brief Interface of all the submodules of opustags.
 */

#include <cstdint>
#include <cstdio>
#include <ogg/ogg.h>

namespace ot {

/**
 * Non-owning string, similar to what std::string_view is in C++17.
 */
struct string_view {
	const char *data;
	size_t size;
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
	const char *vendor_string;
	uint32_t count;
	uint32_t *lengths;
	const char **comment;
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
void free_tags(opus_tags *tags);

/** \} */

}
