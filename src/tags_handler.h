#pragma once

#include "tags.h"

namespace opustags {

    // TagsHandler define various operations related to tags and stream in
    // order to control the main loop.
    // In its implementation, it is expected to receive an option structure.
    class ITagsHandler
    {
    public:
        // Irrelevant streams don't even need to be parsed, so we can save some
        // effort with this method.
        // Returns true if the stream should be parsed, false if it should be
        // ignored (list) or copied identically (edit).
        virtual bool relevant(const int streamno) = 0;

        // The list method is called by list_tags every time it has
        // successfully parsed an OpusTags header.
        virtual void list(const int streamno, const Tags &) = 0;

        // Transform the tags at will.
        // Returns true if the tags were indeed modified, false if they weren't.
        // The latter case may be used for optimization.
        virtual bool edit(const int streamno, Tags &) = 0;

        // The work is done.
        // When listing tags, once we've caught the streams we wanted, it's no
        // use keeping reading the file for new streams. In that case, a true
        // return value would abort any further processing.
        virtual bool done() = 0;

        // Signals the end of the stream.
        // If after this function is called, done() returns false, it's an
        // error. However, it would be better to raise the error inside
        // end_of_stream().
        // For example, if you expect to find the stream #1 and reach the
        // end-of-stream before finding it, better tell the user that you
        // didn't do what he expected.
        virtual void end_of_stream() {}
    };

}
