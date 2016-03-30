#pragma once

#include "ogg.h"
#include "tags_handler.h"

namespace opustags {

    // Decode a file and call the handler's list method every time a tags
    // header is read. Set full to true if you want to make sure every single
    // page of the stream is read.
    //
    // Use:
    //   std::ifstream in("in.ogg");
    //   ogg::Decoder dec(in);
    //   TagsLister lister(options);
    //   list_tags(dec, lister);
    //
    void list_tags(ogg::Decoder&, ITagsHandler &, bool full = false);

    // Forward the input data to the output stream, transforming tags on-the-go
    // with the handler's edit method.
    //
    // Use:
    //   std::ifstream in("in.ogg");
    //   ogg::Decoder dec(in);
    //   std::ofstream out("out.ogg");
    //   std::Encoder enc(out);
    //   TagsEditor editor(options);
    //   edit_tags(dec, enc, editor);
    //
    void edit_tags(ogg::Decoder &in, ogg::Encoder &out, ITagsHandler &);

}
