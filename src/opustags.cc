#include <config.h>

#include "opustags.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ogg/ogg.h>

const char *version = PROJECT_NAME " version " PROJECT_VERSION "\n";

const char *usage =
    "Usage: opustags --help\n"
    "       opustags [OPTIONS] FILE\n"
    "       opustags OPTIONS FILE -o FILE\n";

const char *help =
    "Options:\n"
    "  -h, --help              print this help\n"
    "  -o, --output            write the modified tags to a file\n"
    "  -i, --in-place [SUFFIX] use a temporary file then replace the original file\n"
    "  -y, --overwrite         overwrite the output file if it already exists\n"
    "  -d, --delete FIELD      delete all the fields of a specified type\n"
    "  -a, --add FIELD=VALUE   add a field\n"
    "  -s, --set FIELD=VALUE   delete then add a field\n"
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
    {"delete-all", no_argument, 0, 'D'},
    {"set-all", no_argument, 0, 'S'},
    {NULL, 0, 0, 0}
};

int main(int argc, char **argv){
    if(argc == 1){
        fputs(version, stdout);
        fputs(usage, stdout);
        return EXIT_SUCCESS;
    }
    char *path_in, *path_out = NULL;
    const char *inplace = NULL;
    const char* to_add[argc];
    const char* to_delete[argc];
    int count_add = 0, count_delete = 0;
    int delete_all = 0;
    int set_all = 0;
    int overwrite = 0;
    int print_help = 0;
    int c;
    while((c = getopt_long(argc, argv, "ho:i::yd:a:s:DS", options, NULL)) != -1){
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
            case 'S':
                set_all = 1;
            case 'D':
                delete_all = 1;
                break;
            default:
                return EXIT_FAILURE;
        }
    }
    if(print_help){
        puts(version);
        puts(usage);
        puts(help);
        puts("See the man page for extensive documentation.");
        return EXIT_SUCCESS;
    }
    if(optind != argc - 1){
        fputs("invalid arguments\n", stderr);
        return EXIT_FAILURE;
    }
    if(inplace && path_out){
        fputs("cannot combine --in-place and --output\n", stderr);
        return EXIT_FAILURE;
    }
    path_in = argv[optind];
    if(path_out != NULL && strcmp(path_in, "-") != 0){
        char canon_in[PATH_MAX+1], canon_out[PATH_MAX+1];
        if(realpath(path_in, canon_in) && realpath(path_out, canon_out)){
            if(strcmp(canon_in, canon_out) == 0){
                fputs("error: the input and output files are the same\n", stderr);
                return EXIT_FAILURE;
            }
        }
    }
    FILE *in;
    if(strcmp(path_in, "-") == 0){
        if(set_all){
            fputs("can't open stdin for input when -S is specified\n", stderr);
            return EXIT_FAILURE;
        }
        if(inplace){
            fputs("cannot modify stdin 'in-place'\n", stderr);
            return EXIT_FAILURE;
        }
        in = stdin;
    }
    else
        in = fopen(path_in, "r");
    if(!in){
        perror("fopen");
        return EXIT_FAILURE;
    }
    FILE *out = NULL;
    if(inplace != NULL){
        path_out = static_cast<char*>(malloc(strlen(path_in) + strlen(inplace) + 1));
        if(path_out == NULL){
            fputs("failure to allocate memory\n", stderr);
            fclose(in);
            return EXIT_FAILURE;
        }
        strcpy(path_out, path_in);
        strcat(path_out, inplace);
    }
    if(path_out != NULL){
        if(strcmp(path_out, "-") == 0)
            out = stdout;
        else{
            if(!overwrite && !inplace){
                if(access(path_out, F_OK) == 0){
                    fprintf(stderr, "'%s' already exists (use -y to overwrite)\n", path_out);
                    fclose(in);
                    return EXIT_FAILURE;
                }
            }
            out = fopen(path_out, "w");
            if(!out){
                perror("fopen");
                fclose(in);
                if(inplace)
                    free(path_out);
                return EXIT_FAILURE;
            }
        }
    }
    ogg_sync_state oy;
    ogg_stream_state os, enc;
    ogg_page og;
    ogg_packet op;
    ot::opus_tags tags;
    ogg_sync_init(&oy);
    char *buf;
    size_t len;
    const char *error = NULL;
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
            if(ot::write_page(&og, out) == -1){
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
                if(ot::parse_tags((char*) op.packet, op.bytes, &tags) == -1){
                    error = "opustags: invalid comment header";
                    break;
                }
                if(delete_all)
                    tags.count = 0;
                else{
                    int i;
                    for(i=0; i<count_delete; i++)
                        ot::delete_tags(&tags, to_delete[i]);
                }
                char *raw_tags = NULL;
                if(set_all){
                    raw_tags = static_cast<char*>(malloc(16384));
                    if(raw_tags == NULL){
                        error = "malloc: not enough memory for buffering stdin";
                        free(raw_tags);
                        break;
                    }
                    else{
                        char *raw_comment[256];
                        size_t raw_len = fread(raw_tags, 1, 16383, stdin);
                        if(raw_len == 16383)
                            fputs("warning: truncating comment to 16 KiB\n", stderr);
                        raw_tags[raw_len] = '\0';
                        uint32_t raw_count = 0;
                        size_t field_len = 0;
                        int caught_eq = 0;
                        size_t i = 0;
                        char *cursor = raw_tags;
                        for(i=0; i <= raw_len && raw_count < 256; i++){
                            if(raw_tags[i] == '\n' || raw_tags[i] == '\0'){
                                if(field_len == 0)
                                    continue;
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
                        ot::add_tags(&tags, (const char**) raw_comment, raw_count);
                    }
                }
                ot::add_tags(&tags, to_add, count_add);
                if(out){
                    ogg_packet packet;
                    ot::render_tags(&tags, &packet);
                    if(ogg_stream_packetin(&enc, &packet) == -1)
                        error = "ogg_stream_packetin: internal error";
                    free(packet.packet);
                }
                else
                    ot::print_tags(&tags);
                ot::free_tags(&tags);
                if(raw_tags)
                    free(raw_tags);
                if(error || !out)
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
            if(ot::write_page(&og, out) == -1)
                error = "write_page: fwrite error";
            else if(ogg_stream_check(&enc) != 0)
                error = "ogg_stream_check: internal error (encoder)";
        }
        else if(packet_count >= 2) // Read-only mode
            break;
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
        if(path_out != NULL && out != stdout)
            remove(path_out);
        if(inplace)
            free(path_out);
        return EXIT_FAILURE;
    }
    else if(inplace){
        if(rename(path_out, path_in) == -1){
            perror("rename");
            free(path_out);
            return EXIT_FAILURE;
        }
        free(path_out);
    }
    return EXIT_SUCCESS;
}
