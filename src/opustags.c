#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "opustags.h"
#include "picture.h"

int parse_tags(char *data, long len, opus_tags *tags)
{
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
    if(tags->comment == NULL)
    {
        free(tags->lengths);
        return -1;
    }
    pos += 4;
    // Comment
    uint32_t i;
    for(i=0; i<tags->count; i++)
    {
        tags->lengths[i] = le32toh(*((uint32_t*) (data + pos)));
        tags->comment[i] = data + pos + 4;
        pos += 4 + tags->lengths[i];
        if(pos > len)
            return -1;
    }

    if(pos < len)
        fprintf(stderr, "warning: %ld unused bytes at the end of the OpusTags packet\n", len - pos);

    return 0;
}

int render_tags(opus_tags *tags, ogg_packet *op)
{
    // Note: op->packet must be manually freed.
    op->b_o_s = 0;
    op->e_o_s = 0;
    op->granulepos = 0;
    op->packetno = 1;
    long len = 8 + 4 + tags->vendor_length + 4;
    uint32_t i;
    for(i = 0; i < tags->count; i++)
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
    for(i = 0; i < tags->count; i++)
    {
        n = htole32(tags->lengths[i]);
        memcpy(data, &n, 4);
        memcpy(data+4, tags->comment[i], tags->lengths[i]);
        data += 4 + tags->lengths[i];
    }
    return 0;
}

int match_field(const char *comment, uint32_t len, const char *field)
{
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

void delete_tags(opus_tags *tags, const char *field)
{
    uint32_t i;
    for(i = 0; i < tags->count; i++)
    {
        if(match_field(tags->comment[i], tags->lengths[i], field))
        {
            tags->count--;
            tags->lengths[i] = tags->lengths[tags->count];
            tags->comment[i] = tags->comment[tags->count];
            // No need to resize the arrays.
        }
    }
}

int add_tags(opus_tags *tags, const char **tags_to_add, uint32_t count)
{
    if(count == 0)
        return 0;
    uint32_t *lengths = realloc(tags->lengths, (tags->count + count) * sizeof(uint32_t));
    const char **comment = realloc(tags->comment, (tags->count + count) * sizeof(char*));
    if(lengths == NULL || comment == NULL)
        return -1;
    tags->lengths = lengths;
    tags->comment = comment;
    uint32_t i;
    for(i = 0; i < count; i++)
    {
        tags->lengths[tags->count + i] = strlen(tags_to_add[i]);
        tags->comment[tags->count + i] = tags_to_add[i];
    }
    tags->count += count;
    return 0;
}

void print_tags(opus_tags *tags)
{
    if(tags->count == 0)
        puts("#no tags");
    int i;
    for(i = 0; i < tags->count; i++)
    {
        fwrite(tags->comment[i], 1, tags->lengths[i], stdout);
        puts("");
    }
}

void free_tags(opus_tags *tags)
{
    if(tags->count > 0)
    {
        free(tags->lengths);
        free(tags->comment);
    }
}

int write_page(ogg_page *og, FILE *stream)
{
    if(fwrite(og->header, 1, og->header_len, stream) < og->header_len)
        return -1;
    if(fwrite(og->body, 1, og->body_len, stream) < og->body_len)
        return -1;
    return 0;
}

const char *version = PNAME " version " PVERSION "\n";

const char *usage =
    "Usage: " PNAME " --help\n"
    "       " PNAME " [OPTIONS] FILE\n"
    "       " PNAME " OPTIONS FILE -o FILE\n";

const char *help =
    "Options:\n"
    "  -h, --help              print this help\n"
    "  -o, --output FILE       write the modified tags to a file\n"
    "  -i, --in-place [SUFFIX] use a temporary file then replace the original file\n"
    "  -y, --overwrite         overwrite the output file if it already exists\n"
    "  -d, --delete FIELD      delete all the fields of a specified type\n"
    "  -a, --add FIELD=VALUE   add a field\n"
    "  -s, --set FIELD=VALUE   delete then add a field\n"
    "  -p, --picture FILE      add cover (<64k in BASE64)\n"
    "  -D, --delete-all        delete all the fields!\n"
    "  -S, --set-all           read the fields from stdin\n";

struct option options[] = {
    {"help", no_argument, 0, 'h'},
    {"output", required_argument, 0, 'o'},
    {"in-place", optional_argument, 0, 'i'},
    {"overwrite", no_argument, 0, 'y'},
    {"delete", required_argument, 0, 'd'},
    {"add", required_argument, 0, 'a'},
    {"set", required_argument, 0, 's'},
    {"picture", required_argument, 0, 'p'},
    {"delete-all", no_argument, 0, 'D'},
    {"set-all", no_argument, 0, 'S'},
    {NULL, 0, 0, 0}
};

int main(int argc, char **argv)
{
    if(argc == 1)
    {
        fputs(version, stdout);
        fputs(usage, stdout);
        return EXIT_SUCCESS;
    }
    char *path_in, *path_out = NULL, *inplace = NULL, *path_picture = NULL, *picture_data = NULL;
    const char* to_add[argc];
    const char* to_delete[argc];
    const char* to_picture[1];
    const char *error_message;
    int count_add = 0, count_delete = 0;
    int delete_all = 0;
    int set_all = 0;
    int overwrite = 0;
    int print_help = 0;
    int c;
    while((c = getopt_long(argc, argv, "ho:i::yd:a:s:p:DS", options, NULL)) != -1)
    {
        switch(c){
            case 'h':
                print_help = 1;
                break;
            case 'o':
                path_out = optarg;
                break;
            case 'i':
                inplace = optarg == NULL ? ".otmp" : optarg;
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
            case 'p':
                path_picture = optarg;
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
    if(print_help)
    {
        puts(version);
        puts(usage);
        puts(help);
        puts("See the man page for extensive documentation.");
        return EXIT_SUCCESS;
    }
    if(optind != argc - 1)
    {
        fputs("invalid arguments\n", stderr);
        return EXIT_FAILURE;
    }
    if(inplace && path_out)
    {
        fputs("cannot combine --in-place and --output\n", stderr);
        return EXIT_FAILURE;
    }
    path_in = argv[optind];
    if(path_out != NULL && strcmp(path_in, "-") != 0)
    {
        char canon_in[PATH_MAX+1], canon_out[PATH_MAX+1];
        if(realpath(path_in, canon_in) && realpath(path_out, canon_out))
        {
            if(strcmp(canon_in, canon_out) == 0)
            {
                fputs("error: the input and output files are the same\n", stderr);
                return EXIT_FAILURE;
            }
        }
    }
    FILE *in;
    if(strcmp(path_in, "-") == 0)
    {
        if(set_all)
        {
            fputs("can't open stdin for input when -S is specified\n", stderr);
            return EXIT_FAILURE;
        }
        if(inplace)
        {
            fputs("cannot modify stdin 'in-place'\n", stderr);
            return EXIT_FAILURE;
        }
        in = stdin;
    }
    else
        in = fopen(path_in, "r");
    if(!in)
    {
        perror("fopen");
        return EXIT_FAILURE;
    }
    FILE *out = NULL;
    if(inplace != NULL)
    {
        path_out = malloc(strlen(path_in) + strlen(inplace) + 1);
        if(path_out == NULL)
        {
            fputs("failure to allocate memory\n", stderr);
            fclose(in);
            return EXIT_FAILURE;
        }
        strcpy(path_out, path_in);
        strcat(path_out, inplace);
    }
    if(path_out != NULL)
    {
        if(strcmp(path_out, "-") == 0)
            out = stdout;
        else{
            if(!overwrite && !inplace)
            {
                if(access(path_out, F_OK) == 0)
                {
                    fprintf(stderr, "'%s' already exists (use -y to overwrite)\n", path_out);
                    fclose(in);
                    return EXIT_FAILURE;
                }
            }
            out = fopen(path_out, "w");
            if(!out)
            {
                perror("fopen");
                fclose(in);
                if(inplace)
                    free(path_out);
                return EXIT_FAILURE;
            }
        }
    }
    if(path_picture != NULL)
    {
        int seen_file_icons=0;
        picture_data = parse_picture_specification(path_picture, &error_message, &seen_file_icons);
        if(picture_data == NULL)
        {
            fprintf(stderr,"Not read picture: %s\n", error_message);
        }
    }
    ogg_sync_state oy;
    ogg_stream_state os, enc;
    ogg_page og;
    ogg_packet op;
    opus_tags tags;
    ogg_sync_init(&oy);
    int ogg_buffer_size = 65536;
    char *buf;
    size_t len;
    char *error = NULL;
    int packet_count = -1;
    while(error == NULL)
    {
        // Read until we complete a page.
        if(ogg_sync_pageout(&oy, &og) != 1)
        {
            if(feof(in))
                break;
            buf = ogg_sync_buffer(&oy, ogg_buffer_size);
            if(buf == NULL)
            {
                error = "ogg_sync_buffer: out of memory";
                break;
            }
            len = fread(buf, 1, ogg_buffer_size, in);
            if(ferror(in))
                error = strerror(errno);
            ogg_sync_wrote(&oy, len);
            if(ogg_sync_check(&oy) != 0)
                error = "ogg_sync_check: internal error";
            continue;
        }
        // We got a page.
        // Short-circuit when the relevant packets have been read.
        if(packet_count >= 2 && out)
        {
            if(write_page(&og, out) == -1)
            {
                error = "write_page: fwrite error";
                break;
            }
            continue;
        }
        // Initialize the streams from the first page.
        if(packet_count == -1){
            if(ogg_stream_init(&os, ogg_page_serialno(&og)) == -1)
            {
                error = "ogg_stream_init: couldn't create a decoder";
                break;
            }
            if(out){
                if(ogg_stream_init(&enc, ogg_page_serialno(&og)) == -1)
                {
                    error = "ogg_stream_init: couldn't create an encoder";
                    break;
                }
            }
            packet_count = 0;
        }
        if(ogg_stream_pagein(&os, &og) == -1)
        {
            error = "ogg_stream_pagein: invalid page";
            break;
        }
        // Read all the packets.
        while(ogg_stream_packetout(&os, &op) == 1)
        {
            packet_count++;
            if(packet_count == 1)
            { // Identification header
                if(strncmp((char*) op.packet, "OpusHead", 8) != 0)
                {
                    error = "opustags: invalid identification header";
                    break;
                }
            }
            else if(packet_count == 2){ // Comment header
                if(parse_tags((char*) op.packet, op.bytes, &tags) == -1)
                {
                    error = "opustags: invalid comment header";
                    break;
                }
                if(delete_all)
                    tags.count = 0;
                else
                {
                    int i;
                    for(i=0; i<count_delete; i++)
                        delete_tags(&tags, to_delete[i]);
                }
                char *raw_tags = NULL;
                if(set_all){
                    int raw_comment_size = 16383;
                    raw_tags = malloc(raw_comment_size + 1);
                    if(raw_tags == NULL)
                    {
                        error = "malloc: not enough memory for buffering stdin";
                        free(raw_tags);
                        break;
                    }
                    else{
                        int raw_count_max = 256;
                        char *raw_comment[raw_count_max];
                        size_t raw_len = fread(raw_tags, 1, raw_comment_size, stdin);
                        if(raw_len == raw_comment_size)
                            fputs("warning: truncating comment to 16 KiB\n", stderr);
                        raw_tags[raw_len] = '\0';
                        uint32_t raw_count = 0;
                        size_t field_len = 0;
                        int caught_eq = 0;
                        int caught_cmnt = 0;
                        size_t i = 0;
                        char *cursor = raw_tags;
                        for(i=0; i <= raw_len && raw_count < raw_count_max; i++)
                        {
                            if(raw_tags[i] == '\n' || raw_tags[i] == '\0')
                            {
                                if(!caught_cmnt && field_len > 0)
                                {
                                    if(caught_eq)
                                        raw_comment[raw_count++] = cursor;
                                    else
                                        fputs("warning: skipping malformed tag\n", stderr);
                                }
                                cursor = raw_tags + i + 1;
                                field_len = 0;
                                caught_cmnt = 0;
                                caught_eq = 0;
                                raw_tags[i] = '\0';
                                continue;
                            }
                            if(raw_tags[i] == ' ' && field_len == 0)
                            {
                                cursor = raw_tags + i + 1;
                            } else {
                                if(raw_tags[i] == '#' && field_len == 0)
                                    caught_cmnt = 1;
                                if(raw_tags[i] == '=')
                                    caught_eq = 1;
                                field_len++;
                           }
                        }
                        add_tags(&tags, (const char**) raw_comment, raw_count);
                    }
                }
                add_tags(&tags, to_add, count_add);
                if(picture_data != NULL)
                {
                    char *picture_tag = "METADATA_BLOCK_PICTURE";
                    char *picture_meta = NULL;
                    int picture_meta_len = strlen(picture_tag) + strlen(picture_data) + 1;
                    picture_meta = malloc(picture_meta_len);
                    if(picture_meta == NULL || picture_meta_len > ogg_buffer_size)
                    {
                        fprintf(stderr,"Bad picture size: %d\n", picture_meta_len);
                        free(picture_meta);
                    } else {
                        delete_tags(&tags, picture_tag);
                        strcpy(picture_meta, picture_tag);
                        strcat(picture_meta, "=");
                        strcat(picture_meta, picture_data);
                        strcat(picture_meta, "\0");
                        to_picture[0] = picture_meta;
                        add_tags(&tags, to_picture, 1);
                    }
                }
                if(out)
                {
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
                if(error || !out)
                    break;
                else
                    continue;
            }
            if(out)
            {
                if(ogg_stream_packetin(&enc, &op) == -1)
                {
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
        else if(packet_count >= 2) // Read-only mode
            break;
    }
    if(packet_count >= 0)
    {
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
    if(error)
    {
        fprintf(stderr, "%s\n", error);
        if(path_out != NULL && out != stdout)
            remove(path_out);
        if(inplace)
            free(path_out);
        return EXIT_FAILURE;
    }
    else if(inplace)
    {
        if(rename(path_out, path_in) == -1)
        {
            perror("rename");
            free(path_out);
            return EXIT_FAILURE;
        }
        free(path_out);
    }
    return EXIT_SUCCESS;
}
