#pragma once

#include "tags.h"
#include "ogg.h"

namespace opustags {

    // TagsHandler define various operations related to tags and stream in
    // order to control the main loop.
    // In its implementation, it is expected to receive an option structure.
    struct TagsHandler
    {

        // Irrelevant streams don't even need to be parsed, so we can save some
        // effort with this method.
        // Returns true if the stream should be parsed, false if it should be
        // ignored (list) or copied identically (edit).
        bool relevant(int streamno) { return false; }

        // The list method is called by list_tags every time it has
        // successfully parsed an OpusTags header.
        void list(int streamno, const Tags&) {}

        // Transform the tags at will.
        // Returns true if the tags were indeed modified, false if they weren't.
        // The latter case may be used for optimization.
        bool edit(int streamno, Tags&) { return false; }

        // The work is done.
        // When listing tags, once we've caught the streams we wanted, it's no
        // use keeping reading the file for new streams. In that case, a true
        // return value would abort any further processing.
        bool done() { return false; }

    };

    // Decode a file and call the handler's list method every time a tags
    // header is read.
    //
    // Use:
    //   std::ifstream in("in.ogg");
    //   ogg::Decoder dec(&in);
    //   TagsLister lister(options);
    //   list_tags(dec, lister);
    //
    void list_tags(ogg::Decoder&, TagsHandler&);

    // Forward the input data to the output stream, transforming tags on-the-go
    // with the handler's edit method.
    //
    // Use:
    //   std::ifstream in("in.ogg");
    //   ogg::Decoder dec(&in);
    //   std::ofstream out("out.ogg");
    //   std::Encoder enc(&out);
    //   TagsEditor editor(options);
    //   edit_tags(dec, enc, editor);
    //
    void edit_tags(ogg::Decoder &in, ogg::Encoder &out, TagsHandler&);

}
