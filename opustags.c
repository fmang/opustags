#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    tags->comment = calloc(tags->count, sizeof(char*));
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

int render_tags(opus_tags *tags, ogg_packet *op){
    // Note: op->packet must be manually freed.
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;
    op->packetno = 1;
    long len = 8 + 4 + tags->vendor_length + 4;
    uint32_t i;
    for(i=0; i<tags->count; i++)
        len += 4 + tags->lengths[i];
    op->bytes = len;
    char *data = malloc(len);
    if(!data)
        return -1;
    op->packet = (unsigned char*) data;
    uint32_t n;
    memcpy(data, "OpusTags", 8);
    n = htole32(tags->vendor_length);
    memcpy(data+8, &n, 4);
    memcpy(data+12, tags->vendor_string, tags->vendor_length);
    data += 12 + tags->vendor_length;
    n = htole32(tags->count);
    memcpy(data, &n, 4);
    data += 4;
    for(i=0; i<tags->count; i++){
        n = htole32(tags->lengths[i]);
        memcpy(data, &n, 4);
        memcpy(data+4, tags->comment[i], tags->lengths[i]);
        data += 4 + tags->lengths[i];
    }
    return 0;
}

int match_field(const char *comment, uint32_t len, const char *field){
    size_t field_len;
    for(field_len = 0; field[field_len] != '\0' && field[field_len] != '='; field_len++);
    if(len <= field_len)
        return 0;
    if(comment[field_len] != '=')
        return 0;
    if(strncmp(comment, field, field_len) != 0)
        return 0;
    return 1;

}

void delete_tags(opus_tags *tags, const char *field){
    uint32_t i;
    for(i=0; i<tags->count; i++){
        if(match_field(tags->comment[i], tags->lengths[i], field)){
            tags->count--;
            tags->lengths[i] = tags->lengths[tags->count];
            tags->comment[i] = tags->comment[tags->count];
            // No need to resize the arrays.
        }
    }
}

int add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count){
    if(count == 0)
        return 0;
    uint32_t *lengths = realloc(tags->lengths, (tags->count + count) * sizeof(uint32_t));
    const char **comment = realloc(tags->comment, (tags->count + count) * sizeof(char*));
    if(lengths == NULL || comment == NULL)
        return -1;
    tags->lengths = lengths;
    tags->comment = comment;
    uint32_t i;
    for(i=0; i<count; i++){
        tags->lengths[tags->count + i] = strlen(tags_to_add[i]);
        tags->comment[tags->count + i] = tags_to_add[i];
    }
    tags->count += count;
    return 0;
}

void print_tags(opus_tags *tags){
    if(tags->count == 0)
        puts("no tags");
    int i;
    for(i=0; i<tags->count; i++){
        fwrite(tags->comment[i], 1, tags->lengths[i], stdout);
        puts("");
    }
}

void free_tags(opus_tags *tags){
    if(tags->count > 0){
        free(tags->lengths);
        free(tags->comment);
    }
}

int write_page(ogg_page *og, FILE *stream){
    if(fwrite(og->header, 1, og->header_len, stream) < og->header_len)
        return -1;
    if(fwrite(og->body, 1, og->body_len, stream) < og->body_len)
        return -1;
    return 0;
}

int main(int argc, char **argv){
    const char *path_in, *path_out = NULL;
    const char* to_add[argc];
    const char* to_delete[argc];
    int count_add = 0, count_delete = 0;
    int delete_all = 0;
    int set_all = 0;
    int overwrite = 0;
    int c;
    while((c = getopt(argc, argv, "o:yd:a:s:DS")) != -1){
        switch(c){
            case 'o':
                path_out = optarg;
                break;
            case 'y':
                overwrite = 1;
                break;
            case 'd':
                if(strchr(optarg, '=') != NULL){
                    fprintf(stderr, "invalid field: '%s'\n", optarg);
                    return EXIT_FAILURE;
                }
                to_delete[count_delete++] = optarg;
                break;
            case 'a':
            case 's':
                if(strchr(optarg, '=') == NULL){
                    fprintf(stderr, "invalid comment: '%s'\n", optarg);
                    return EXIT_FAILURE;
                }
                to_add[count_add++] = optarg;
                if(c == 's')
                    to_delete[count_delete++] = optarg;
                break;
            case 'S':
                set_all = 1;
            case 'D':
                delete_all = 1;
                break;
            default:
                return EXIT_FAILURE;
        }
    }
    if(optind != argc - 1){
        fputs("invalid arguments\n", stderr);
        return EXIT_FAILURE;
    }
    path_in = argv[optind];
    FILE *out = NULL;
    if(path_out != NULL){
        if(strcmp(path_out, "-") == 0)
            out = stdout;
        else{
            if(!overwrite){
                if(access(path_out, F_OK) == 0){
                    fprintf(stderr, "'%s' already exists (use -y to overwrite)\n", path_out);
                    return EXIT_FAILURE;
                }
            }
            out = fopen(path_out, "w");
            if(!out){
                perror("fopen");
                return EXIT_FAILURE;
            }
        }
    }
    FILE *in;
    if(strcmp(path_in, "-") == 0){
        if(set_all){
            fputs("can't open stdin for input when -S is specified\n", stderr);
            if(out)
                fclose(out);
            return EXIT_FAILURE;
        }
        in = stdin;
    }
    else
        in = fopen(path_in, "r");
    if(!in){
        perror("fopen");
        if(out)
            fclose(out);
        return EXIT_FAILURE;
    }
    ogg_sync_state oy;
    ogg_stream_state os, enc;
    ogg_page og;
    ogg_packet op;
    opus_tags tags;
    ogg_sync_init(&oy);
    char *buf;
    size_t len;
    char *error = NULL;
    int packet_count = -1;
    while(error == NULL){
        // Read until we complete a page.
        if(ogg_sync_pageout(&oy, &og) != 1){
            if(feof(in))
                break;
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
        // Short-circuit when the relevant packets have been read.
        if(packet_count >= 2 && out){
            if(write_page(&og, out) == -1){
                error = "write_page: fwrite error";
                break;
            }
            continue;
        }
        // Initialize the streams from the first page.
        if(packet_count == -1){
            if(ogg_stream_init(&os, ogg_page_serialno(&og)) == -1){
                error = "ogg_stream_init: couldn't create a decoder";
                break;
            }
            if(out){
                if(ogg_stream_init(&enc, ogg_page_serialno(&og)) == -1){
                    error = "ogg_stream_init: couldn't create an encoder";
                    break;
                }
            }
            packet_count = 0;
        }
        if(ogg_stream_pagein(&os, &og) == -1){
            error = "ogg_stream_pagein: invalid page";
            break;
        }
        // Read all the packets.
        while(ogg_stream_packetout(&os, &op) == 1){
            packet_count++;
            if(packet_count == 1){ // Identification header
                if(strncmp((char*) op.packet, "OpusHead", 8) != 0){
                    error = "opustags: invalid identification header";
                    break;
                }
            }
            else if(packet_count == 2){ // Comment header
                if(parse_tags((char*) op.packet, op.bytes, &tags) == -1){
                    error = "opustags: invalid comment header";
                    break;
                }
                if(delete_all)
                    tags.count = 0;
                else{
                    int i;
                    for(i=0; i<count_delete; i++)
                        delete_tags(&tags, to_delete[i]);
                }
                char *raw_tags = NULL;
                if(set_all){
                    raw_tags = malloc(16384);
                    if(raw_tags == NULL){
                        error = "malloc: not enough memory for buffering stdin";
                        free(raw_tags);
                        break;
                    }
                    else{
                        char *raw_comment[256];
                        size_t raw_len = fread(raw_tags, 1, 16384, stdin);
                        if(raw_len == 16384)
                            fputs("warning: truncating comment to 16 KiB\n", stderr);
                        uint32_t raw_count = 0;
                        size_t field_len = 0;
                        int caught_eq = 0;
                        size_t i = 0;
                        char *cursor = raw_tags;
                        for(i=0; i<raw_len && raw_count < 256; i++){
                            if(raw_tags[i] == '\n'){
                                if(caught_eq)
                                    raw_comment[raw_count++] = cursor;
                                else
                                    fputs("warning: skipping malformed tag\n", stderr);
                                cursor = raw_tags + i + 1;
                                field_len = 0;
                                caught_eq = 0;
                                raw_tags[i] = '\0';
                                continue;
                            }
                            if(raw_tags[i] == '=')
                                caught_eq = 1;
                            field_len++;
                        }
                        add_tags(&tags, (const char**) raw_comment, raw_count);
                    }
                }
                add_tags(&tags, to_add, count_add);
                if(out){
                    ogg_packet packet;
                    render_tags(&tags, &packet);
                    if(ogg_stream_packetin(&enc, &packet) == -1)
                        error = "ogg_stream_packetin: internal error";
                    free(packet.packet);
                }
                else
                    print_tags(&tags);
                free_tags(&tags);
                if(raw_tags)
                    free(raw_tags);
                if(error)
                    break;
                else
                    continue;
            }
            if(out){
                if(ogg_stream_packetin(&enc, &op) == -1){
                    error = "ogg_stream_packetin: internal error";
                    break;
                }
            }
        }
        if(error != NULL)
            break;
        if(ogg_stream_check(&os) != 0)
            error = "ogg_stream_check: internal error (decoder)";
        // Write the page.
        if(out){
            ogg_stream_flush(&enc, &og);
            if(write_page(&og, out) == -1)
                error = "write_page: fwrite error";
            else if(ogg_stream_check(&enc) != 0)
                error = "ogg_stream_check: internal error (encoder)";
        }
    }
    if(packet_count >= 0){
        ogg_stream_clear(&os);
        if(out)
            ogg_stream_clear(&enc);
    }
    ogg_sync_clear(&oy);
    fclose(in);
    if(out)
        fclose(out);
    if(!error && packet_count < 2)
        error = "opustags: invalid file";
    if(error){
        fprintf(stderr, "%s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
