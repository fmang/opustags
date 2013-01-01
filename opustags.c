#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

typedef struct {
    uint32_t vendor_length;
    const char *vendor_string;
    uint32_t count;
    uint32_t *lengths;
    const char **comment;
} opus_tags;

int parse_tags(char *data, long len, opus_tags *tags){
    long pos;
    if(len < 8+4+4)
        return -1;
    if(strncmp(data, "OpusTags", 8) != 0)
        return -1;
    // Vendor
    pos = 8;
    tags->vendor_length = le32toh(*((uint32_t*) (data + pos)));
    tags->vendor_string = data + pos + 4;
    pos += 4 + tags->vendor_length;
    if(pos + 4 > len)
        return -1;
    // Count
    tags->count = le32toh(*((uint32_t*) (data + pos)));
    if(tags->count == 0)
        return 0;
    tags->lengths = calloc(tags->count, sizeof(uint32_t));
    if(tags->lengths == NULL)
        return -1;
    tags->comment = calloc(tags->count, sizeof(const char*));
    if(tags->comment == NULL){
        free(tags->lengths);
        return -1;
    }
    pos += 4;
    // Comment
    uint32_t i;
    for(i=0; i<tags->count; i++){
        tags->lengths[i] = le32toh(*((uint32_t*) (data + pos)));
        tags->comment[i] = data + pos + 4;
        pos += 4 + tags->lengths[i];
        if(pos > len)
            return -1;
    }
    if(pos != len)
        return -1;
    return 0;
}

void free_tags(opus_tags *tags){
    if(tags->count > 0){
        free(tags->lengths);
        free(tags->comment);
    }
}

int main(int argc, char **argv){
    FILE *in = fopen(argv[1], "r");
    if(!in){
        perror("fopen");
        return EXIT_FAILURE;
    }
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    opus_tags tags;
    ogg_sync_init(&oy);
    char *buf;
    size_t len;
    char *error = NULL;
    int packet_count = -1;
    while(error == NULL && !feof(in)){
        // Read until we complete a page.
        if(ogg_sync_pageout(&oy, &og) != 1){
            buf = ogg_sync_buffer(&oy, 65536);
            if(buf == NULL){
                error = "ogg_sync_buffer: out of memory";
                break;
            }
            len = fread(buf, 1, 65536, in);
            if(ferror(in))
                error = strerror(errno);
            ogg_sync_wrote(&oy, len);
            if(ogg_sync_check(&oy) != 0)
                error = "ogg_sync_check: internal error";
            continue;
        }
        // We got a page.
        if(packet_count == -1){ // First page
            if(ogg_stream_init(&os, ogg_page_serialno(&og)) == -1){
                error = "ogg_stream_init: unsuccessful";
                break;
            }
            packet_count = 0;
        }
        if(ogg_stream_pagein(&os, &og) == -1){
            error = "ogg_stream_pagein: invalid page";
            break;
        }
        if(ogg_stream_packetout(&os, &op) == 1){
            packet_count++;
            if(packet_count == 1){ // Identification header
                if(strncmp((char*) op.packet, "OpusHead", 8) != 0)
                    error = "opustags: invalid identification header";
            }
            if(packet_count == 2){ // Comment header
                if(parse_tags((char*) op.packet, op.bytes, &tags) == -1)
                    error = "opustags: invalid comment header";
                fwrite(tags.vendor_string, 1, tags.vendor_length, stdout);
                puts("");
                int i;
                for(i=0; i<tags.count; i++){
                    fwrite(tags.comment[i], 1, tags.lengths[i], stdout);
                    puts("");
                }
                free_tags(&tags);
                break;
            }
        }
        if(ogg_stream_check(&os) != 0)
            error = "ogg_stream_check: internal error";
    }
    if(packet_count >= 0)
        ogg_stream_clear(&os);
    ogg_sync_clear(&oy);
    fclose(in);
    if(!error && packet_count < 2)
        error = "opustags: invalid file";
    if(error){
        fprintf(stderr, "%s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
