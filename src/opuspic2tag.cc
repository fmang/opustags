#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "picture.h"
#include "config.h"

struct option options[] = {
    {"help", no_argument, 0, 'h'},
    {"desc", required_argument, 0, 'd'},
    {"type", required_argument, 0, 't'},
    {"output", required_argument, 0, 'o'},
    {NULL, 0, 0, 0}
};

int main(int argc, char **argv)
{
    if(argc == 1)
    {
        fprintf(stdout, "opuspic2tag version %s\n", PROJECT_VERSION);
        fprintf(stdout, "Usage: %s [OPTIONS] FILE{.jpg,.png,.gif}\n", argv[0]);
        return EXIT_SUCCESS;
    }
    char *path_picture = NULL, *picture_data = NULL, *picture_desc = NULL, *path_out = NULL;
    const char *error_message;
    int print_help = 0;
    unsigned long picture_type = 3;
    int c;
    while((c = getopt_long(argc, argv, "hd:t:o:", options, NULL)) != -1)
    {
        switch(c){
            case 'h':
                print_help = 1;
                break;
            case 'd':
                picture_desc = optarg;
                break;
            case 't':
                picture_type = atoi(optarg);
                picture_type = (picture_type > 20) ? 3 : picture_type;
                break;
            case 'o':
                path_out = optarg;
                break;
            default:
                return EXIT_FAILURE;
        }
    }
    if(print_help)
    {
        fprintf(stdout, "opuspic2tag version %s\n", PROJECT_VERSION);
        fprintf(stdout, "Usage: %s [OPTIONS] FILE{.jpg,.png,.gif}\n", argv[0]);
        fprintf(stdout, "Options:\n");
        fprintf(stdout, "  -h, --help              print this help\n");
        fprintf(stdout, "  -d, --desc STRING       description\n");
        fprintf(stdout, "  -t, --type NUM          set image type by 0-20 or keywords. Default: 3\n");
        fprintf(stdout, "  -o, --output FILE       write tag to a file (default=stdout)\n");
        fprintf(stdout, "See the man page for extensive documentation.");
        return EXIT_SUCCESS;
    }
    if(optind != argc - 1)
    {
        fputs("invalid arguments\n", stderr);
        return EXIT_FAILURE;
    }
    path_picture = argv[optind];
    if(path_picture != NULL)
    {
        int seen_file_icons=0;
        picture_data = opustags_picture_specification_parse(path_picture, &error_message, picture_desc, picture_type, &seen_file_icons);
        if(picture_data == NULL)
        {
            fprintf(stderr,"Not read picture: %s\n", error_message);
        }
    }
    FILE *out = NULL;
    if(path_out != NULL)
    {
        if(strcmp(path_picture, path_out) == 0)
        {
            fputs("error: the input and output files are the same\n", stderr);
            return EXIT_FAILURE;
        }
        out = fopen(path_out, "w");
        if(!out)
        {
            perror("fopen");
            return EXIT_FAILURE;
        }
    } else {
        out = stdout;
    }
    if(picture_data != NULL)
    {
        char *picture_tag = (char*)"METADATA_BLOCK_PICTURE";
        char *picture_meta = NULL;
        int picture_meta_len = strlen(picture_tag) + strlen(picture_data) + 1;
        picture_meta = (char*)malloc(picture_meta_len);
        if(picture_meta == NULL)
        {
            fprintf(stderr,"Bad picture size: %d\n", picture_meta_len);
            free(picture_meta);
        } else {
            strcpy(picture_meta, picture_tag);
            strcat(picture_meta, "=");
            strcat(picture_meta, picture_data);
            strcat(picture_meta, "\0");
        }
        fwrite(picture_meta, 1, picture_meta_len, out);
        free(picture_meta);
        fwrite("\n", 1, 1, out);
    }
    if(out)
        fclose(out);
    return EXIT_SUCCESS;
}
