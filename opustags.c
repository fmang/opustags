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
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
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
                if(strncmp((char*) op.packet, "OpusTags", 8) != 0)
                    error = "opustags: invalid comment header";
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
