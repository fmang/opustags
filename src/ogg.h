#pragma once

#include "tags.h"

#include <iostream>
#include <map>
#include <ogg/ogg.h>

namespace opustags {
namespace ogg
{
    enum StreamState {
        BEGIN_OF_STREAM,
        HEADER_READY,
        TAGS_READY,
        DATA_READY,
        RAW_READY,
        END_OF_STREAM,
        // Meaning of these states below, in Stream.
    };

    enum StreamType {
        UNKNOWN_STREAM,
        OPUS_STREAM,
    };

    // An Ogg file may contain many logical bitstreams, and among them, many
    // Opus streams. This class represents one stream, whether it is Opus or
    // not.
    struct Stream
    {
        Stream(int streamno);
        ~Stream();

        // Called by Decoder once a page was read.
        // Returns true if it's ready, false if it expects more data.
        // In the latter case, Decoder::read_page will keep reading.
        bool page_in(ogg_page&);

        // Make the stream behave as if it were unknown.
        // As a consequence, no more effort would be made in extracting data.
        void downgrade();

        StreamState state;
        StreamType type;
        Tags tags;

        // Here are the meanings of the state variable:
        //   BEGIN_OF_STREAM: the stream was just initialized,
        //   HEADER_READY: the header is parsed and we know the type,
        //   TAGS_READY: the tags are parsed and complete,
        //   DATA_READY: we've read a data page whose meaning is no concern to us,
        //   RAW_READY: we don't even know what kind of stream that is, so don't alter it,
        //   END_OF_STREAM: no more thing to read, not even the current page.

        // From the state, we decide what to do with the Decoder's current_page.
        // The difference between DATA_READY and RAW_DATA is that the former
        // might require a reencoding of the current page. For example, if the
        // tags grow and span over two pages, all the following pages are gonna
        // need a new sequence number.

        ogg_stream_state stream;

    private:
        bool handle_page();
        void handle_packet(const ogg_packet&);
        void parse_header(const ogg_packet&);
        void parse_tags(const ogg_packet&);
    };

    struct Decoder
    {
        Decoder(std::istream*);
        ~Decoder();

        // Read a page, dispatch it, and return the stream it belongs to.
        // The read page is given to Stream::page_in before this function
        // returns.
        // After the end of the file is reached, it returns NULL.
        Stream* read_page();

        std::istream *input;

        ogg_sync_state sync;
        ogg_page current_page;
        std::map<int, Stream> streams;

    private:
        bool page_out();
        bool buff();
    };

    struct Encoder
    {
        Encoder(std::ostream*);

        // Copy the input stream's current page.
        void forward(Stream &in);

        // Write the page without even ensuring its page number is correct.
        // It would be an efficient way to copy a stream identically, and also
        // needed for write_page.
        void write_raw_page(const ogg_page&);

        void write_tags(int streamno, const Tags&);

        std::ostream *output;

        // We're gonna need some ogg_stream_state for adjusting the page
        // numbers and splitting large packets as it's gotta be done.
        std::map<int, Stream> streams;

    private:
        Stream& get_stream(int streamno);
        void forward_stream(Stream &in, Stream &out);
        void flush_stream(Stream &out);
    };
}
}
