#pragma once

#include <stdexcept>
#include <iostream>
#include <map>
#include <ogg/ogg.h>

namespace opustags {
namespace ogg
{
    typedef std::map<std::string, std::string> Tags;

    enum StreamState {
        BEGIN_OF_STREAM,
        HEADER_READY,
        TAGS_READY,
        DATA_READY,
        RAW_READY,
        END_OF_STREAM,
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
        Stream(int serialno);
        ~Stream();

        // Called by Decoder once a page was read.
        // Returns true if it's ready, false if it expects more data.
        // In the latter case, Decoder::read_page will keep reading.
        bool page_in(const ogg_page&);

        // Make the stream behave as if it were unknown.
        // As a consequence, no more effort would be made in extracting data.
        void downgrade();

        StreamState state;
        StreamType type;
        Tags tags;

        ogg_stream_state stream;
    };

    struct Decoder
    {
        Decoder(std::istream&&);
        ~Decoder();

        // Read a page, dispatch it, and return the stream it belongs to.
        // The read page is given to Stream::page_in before this function
        // returns.
        // After the end of the file is reached, it returns NULL.
        Stream* read_page();

        std::istream input;

        ogg_sync_state sync;
        ogg_page current_page;
        std::map<int, Stream> streams;
    };

    struct Encoder
    {
        Encoder(std::ostream&&);
        ~Encoder();

        void write_page(const ogg_page&);

        // Write the page without even ensuring its page number is correct.
        // It would be an efficient way to copy a stream identically, and also
        // needed for write_page.
        void write_raw_page(const ogg_page&);

        void write_tags(int serialno, const Tags&);

        std::ostream output;

        // We're gonna need some ogg_stream_state for adjusting the page
        // numbers and splitting large packets as it's gotta be done.
        std::map<int, ogg_stream_state> streams;
    };
}
}
