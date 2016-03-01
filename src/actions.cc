#include "actions.h"

using namespace opustags;

void opustags::list_tags(ogg::Decoder &dec, ITagsHandler &handler)
{
    std::shared_ptr<ogg::Stream> s;
    while (!handler.done()) {
        s = dec.read_page();
        if (s == nullptr)
            break; // end of stream
        switch (s->state) {
            case ogg::HEADER_READY:
                if (!handler.relevant(s->stream.serialno))
                    s->downgrade();
                break;
            case ogg::TAGS_READY:
                handler.list(s->stream.serialno, s->tags);
                s->downgrade(); // no more use for it
                break;
            default:
                ;
        }
    }
}

void opustags::edit_tags(
    ogg::Decoder &in, ogg::Encoder &out, ITagsHandler &handler)
{
    std::shared_ptr<ogg::Stream> s;
    while (true) {
        s = in.read_page();
        if (s == nullptr)
            break; // end of stream
        switch (s->state) {

            case ogg::HEADER_READY:
                if (!handler.relevant(s->stream.serialno)) {
                    s->downgrade();
                    out.write_raw_page(in.current_page);
                } else {
                    out.forward(*s);
                }
                break;

            case ogg::TAGS_READY:
                handler.edit(s->stream.serialno, s->tags);
                out.write_tags(s->stream.serialno, s->tags);
                break;

            case ogg::DATA_READY:
                out.forward(*s);
                break;

            case ogg::RAW_READY:
                out.write_raw_page(in.current_page);
                break;

            default:
                ;
        }
    }
}
