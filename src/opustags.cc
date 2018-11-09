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

/**
 * Display the tags on stdout.
 *
 * Print all the comments, separated by line breaks. Since a comment may
 * contain line breaks, this output is not completely reliable, but it fits
 * most cases.
 *
 * Only the comments are displayed.
 */
static void print_tags(ot::opus_tags &tags)
{
	for (const ot::string_view &comment : tags.comments) {
		fwrite(comment.data(), 1, comment.size(), stdout);
		puts("");
	}
}

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
    ot::ogg_reader reader;
    ot::ogg_writer writer;
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
    if(strcmp(path_in, "-") == 0){
        if(set_all){
            fputs("can't open stdin for input when -S is specified\n", stderr);
            return EXIT_FAILURE;
        }
        if(inplace){
            fputs("cannot modify stdin 'in-place'\n", stderr);
            return EXIT_FAILURE;
        }
        reader.file = stdin;
    }
    else
        reader.file = fopen(path_in, "r");
    if(!reader.file){
        perror("fopen");
        return EXIT_FAILURE;
    }
    writer.file = NULL;
    if(inplace != NULL){
        path_out = static_cast<char*>(malloc(strlen(path_in) + strlen(inplace) + 1));
        if(path_out == NULL){
            fputs("failure to allocate memory\n", stderr);
            fclose(reader.file);
            return EXIT_FAILURE;
        }
        strcpy(path_out, path_in);
        strcat(path_out, inplace);
    }
    if(path_out != NULL){
        if(strcmp(path_out, "-") == 0)
            writer.file = stdout;
        else{
            if(!overwrite && !inplace){
                if(access(path_out, F_OK) == 0){
                    fprintf(stderr, "'%s' already exists (use -y to overwrite)\n", path_out);
                    fclose(reader.file);
                    return EXIT_FAILURE;
                }
            }
            writer.file = fopen(path_out, "w");
            if(!writer.file){
                perror("fopen");
                fclose(reader.file);
                if(inplace)
                    free(path_out);
                return EXIT_FAILURE;
            }
        }
    }
    ot::opus_tags tags;
    const char *error = NULL;
    int packet_count = -1;
    while(error == NULL){
        // Read the next page.
        ot::status rc = reader.read_page();
        if (rc == ot::status::end_of_file) {
            break;
        } else if (rc != ot::status::ok) {
            if (rc == ot::status::standard_error)
                error = strerror(errno);
            else
                error = "error reading the next ogg page";
            break;
        }
        // Short-circuit when the relevant packets have been read.
        if(packet_count >= 2 && writer.file){
            if(ot::write_page(&reader.page, writer.file) == -1){
                error = "write_page: fwrite error";
                break;
            }
            continue;
        }
        // Initialize the streams from the first page.
        if(packet_count == -1){
            if(ogg_stream_init(&reader.stream, ogg_page_serialno(&reader.page)) == -1){
                error = "ogg_stream_init: couldn't create a decoder";
                break;
            }
            if(writer.file){
                if(ogg_stream_init(&writer.stream, ogg_page_serialno(&reader.page)) == -1){
                    error = "ogg_stream_init: couldn't create an encoder";
                    break;
                }
            }
            packet_count = 0;
        }
        if(ogg_stream_pagein(&reader.stream, &reader.page) == -1){
            error = "ogg_stream_pagein: invalid page";
            break;
        }
        // Read all the packets.
        while(ogg_stream_packetout(&reader.stream, &reader.packet) == 1){
            packet_count++;
            if(packet_count == 1){ // Identification header
                if(strncmp((char*) reader.packet.packet, "OpusHead", 8) != 0){
                    error = "opustags: invalid identification header";
                    break;
                }
            }
            else if(packet_count == 2){ // Comment header
                if(ot::parse_tags((char*) reader.packet.packet, reader.packet.bytes, &tags) != ot::status::ok){
                    error = "opustags: invalid comment header";
                    break;
                }
                if(delete_all)
                    tags.comments.clear();
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
                        for (size_t i = 0; i < raw_count; ++i)
                            tags.comments.emplace_back(raw_comment[i]);
                    }
                }
                for (size_t i = 0; i < count_add; ++i)
                    tags.comments.emplace_back(to_add[i]);
                if(writer.file){
                    ogg_packet packet;
                    ot::render_tags(&tags, &packet);
                    if(ogg_stream_packetin(&writer.stream, &packet) == -1)
                        error = "ogg_stream_packetin: internal error";
                    free(packet.packet);
                }
                else
                    print_tags(tags);
                if(raw_tags)
                    free(raw_tags);
                if(error || !writer.file)
                    break;
                else
                    continue;
            }
            if(writer.file){
                if(ogg_stream_packetin(&writer.stream, &reader.packet) == -1){
                    error = "ogg_stream_packetin: internal error";
                    break;
                }
            }
        }
        if(error != NULL)
            break;
        if(ogg_stream_check(&reader.stream) != 0)
            error = "ogg_stream_check: internal error (decoder)";
        // Write the page.
        if(writer.file){
            ogg_page page;
            ogg_stream_flush(&writer.stream, &page);
            if(ot::write_page(&page, writer.file) == -1)
                error = "write_page: fwrite error";
            else if(ogg_stream_check(&writer.stream) != 0)
                error = "ogg_stream_check: internal error (encoder)";
        }
        else if(packet_count >= 2) // Read-only mode
            break;
    }
    fclose(reader.file);
    if(writer.file)
        fclose(writer.file);
    if(!error && packet_count < 2)
        error = "opustags: invalid file";
    if(error){
        fprintf(stderr, "%s\n", error);
        if(path_out != NULL && writer.file != stdout)
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
