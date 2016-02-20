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

        // Called by Reader once a page was read.
        // Return true if it's ready, false if it expects more data.
        // In the latter case, Reader::read_page will keep reading.
        bool page_in(ogg_page*);

        // The structure is stored into the Reader, and the memory inside is
        // managed by ogg_sync_state (maybe).
        // It's not a big deal since it should be used right after read_page()
        // was called, and is meant to be forwarded to the Writer.
        // TODO make sure it's allocated as expected, because the libogg
        //      specification isn't entirely clear.
        ogg_page *current_page;

        StreamState state;
        StreamType type;
        Tags tags;

        ogg_stream_state stream;
    };

    struct Reader
    {
        Reader(std::istream&&);
        ~Reader();

        // Read a page, dispatch it, and return the stream it belongs to.
        // The read page is given to Stream::page_in before this function
        // returns.
        // After the end of the file is reached, it returns NULL.
        Stream* read_page();

        std::istream input;

        ogg_sync_state sync;
        std::map<int, Stream> streams;
    };

    struct Writer
    {
        Writer(std::ostream&&);
        ~Writer();

        void write_page(ogg_page*);

        // Write the page without even ensuring its page number is correct.
        // It would be an efficient way to copy a stream identically, and also
        // needed for write_page.
        void write_raw_page(ogg_page*);

        void write_tags(int serialno, const Tags&);

        std::ostream output;

        // We're gonna need some ogg_stream_state for adjusting the page
        // numbers and splitting large packets as it's gotta be done.
        std::map<int, ogg_stream_state> streams;
    };
}
}
