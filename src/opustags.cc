#include <config.h>

#include "opustags.h"

#include <errno.h>
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
	for (const std::string& comment : tags.comments) {
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
    ot::options opt;
    int exit_code = parse_options(argc, argv, opt);
    if (exit_code != EXIT_SUCCESS)
        return exit_code;
    if (opt.print_help) {
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
    if (opt.inplace != nullptr && !opt.path_out.empty()) {
        fputs("cannot combine --in-place and --output\n", stderr);
        return EXIT_FAILURE;
    }
    opt.path_in = argv[optind];
    if (opt.path_in.empty()) {
        fputs("input's file path cannot be empty\n", stderr);
        return EXIT_FAILURE;
    }
    if (!opt.path_out.empty() && opt.path_in != "-" && opt.path_out != "-") {
        char canon_in[PATH_MAX+1], canon_out[PATH_MAX+1];
        if (realpath(opt.path_in.c_str(), canon_in) && realpath(opt.path_out.c_str(), canon_out)) {
            if (strcmp(canon_in, canon_out) == 0) {
                fputs("error: the input and output files are the same\n", stderr);
                return EXIT_FAILURE;
            }
        }
    }
    ot::ogg_reader reader;
    ot::ogg_writer writer;
    if (opt.path_in == "-") {
        if (opt.set_all) {
            fputs("can't open stdin for input when -S is specified\n", stderr);
            return EXIT_FAILURE;
        }
        if (opt.inplace) {
            fputs("cannot modify stdin in-place\n", stderr);
            return EXIT_FAILURE;
        }
        reader.file = stdin;
    }
    else
        reader.file = fopen(opt.path_in.c_str(), "r");
    if(!reader.file){
        perror("fopen");
        return EXIT_FAILURE;
    }
    writer.file = NULL;
    if (opt.inplace != nullptr)
        opt.path_out = opt.path_in + opt.inplace;
    if (!opt.path_out.empty()) {
        if (opt.path_out == "-") {
            writer.file = stdout;
        } else {
            if (!opt.overwrite && !opt.inplace){
                if (access(opt.path_out.c_str(), F_OK) == 0) {
                    fprintf(stderr, "'%s' already exists (use -y to overwrite)\n", opt.path_out.c_str());
                    fclose(reader.file);
                    return EXIT_FAILURE;
                }
            }
            writer.file = fopen(opt.path_out.c_str(), "w");
            if(!writer.file){
                perror("fopen");
                fclose(reader.file);
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
                if (opt.delete_all) {
                    tags.comments.clear();
                } else {
                    for (const std::string& name : opt.to_delete)
                        ot::delete_tags(&tags, name.c_str());
                }
                char *raw_tags = NULL;
                if (opt.set_all) {
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
                for (const std::string& comment : opt.to_add)
                    tags.comments.emplace_back(comment);
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
        if (!opt.path_out.empty() && writer.file != stdout)
            remove(opt.path_out.c_str());
        return EXIT_FAILURE;
    }
    else if (opt.inplace) {
        if (rename(opt.path_out.c_str(), opt.path_in.c_str()) == -1) {
            perror("rename");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
