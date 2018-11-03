/**
 * \file src/opustags.h
 * \brief Interface of all the submodules of opustags.
 */

#include <cstdint>

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

/** \} */

}
