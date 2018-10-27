#include <ogg/ogg.h>

#ifndef __OPUSTAGS_H
#define __OPUSTAGS_H

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#endif

typedef struct {
    uint32_t vendor_length;
    const char *vendor_string;
    uint32_t count;
    uint32_t *lengths;
    const char **comment;
} opus_tags;

int parse_tags(char *data, long len, opus_tags *tags);
int render_tags(opus_tags *tags, ogg_packet *op);
int match_field(const char *comment, uint32_t len, const char *field);
void delete_tags(opus_tags *tags, const char *field);
int add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count);
void print_tags(opus_tags *tags);
void free_tags(opus_tags *tags);
int write_page(ogg_page *og, FILE *stream);

#endif /* __OPUSTAGS_H */
