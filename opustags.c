#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main(int argc, char **argv){
    FILE *in = fopen(argv[1], "r");
    if(!in){
        perror("fopen");
        return EXIT_FAILURE;
    }
    ogg_sync_state oy;
    ogg_page og;
    ogg_sync_init(&oy);
    char *buf;
    size_t len;
    char *error = NULL;
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
        puts("got a page");
    }
    ogg_sync_clear(&oy);
    fclose(in);
    if(error){
        fprintf(stderr, "%s\n", error);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
