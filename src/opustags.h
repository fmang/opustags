/**
 * \file src/opustags.h
 * \brief Interface of all the submodules of opustags.
 */

#include <cstdint>
#include <ogg/ogg.h>

namespace ot {

/**
 * \defgroup ogg Ogg
 * \brief Helpers to work with libogg.
 */

/**
 * \defgroup opus Opus
 * \brief Opus packet decoding and recoding.
 *
 * \{
 */

struct opus_tags {
	uint32_t vendor_length;
	const char *vendor_string;
	uint32_t count;
	uint32_t *lengths;
	const char **comment;
};

int parse_tags(char *data, long len, opus_tags *tags);
int render_tags(opus_tags *tags, ogg_packet *op);
void delete_tags(opus_tags *tags, const char *field);
int add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count);
void print_tags(opus_tags *tags);
void free_tags(opus_tags *tags);

/** \} */

}
