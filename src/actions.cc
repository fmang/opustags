#include "actions.h"

using namespace opustags;

void opustags::list_tags(ogg::Decoder &dec, ITagsHandler &handler, bool full)
{
    std::map<long, int> sequence_numbers;
    int stream_count = 0;
    int remaining_streams = 0;
    std::shared_ptr<ogg::Stream> s;
    while (!handler.done()) {
        s = dec.read_page();
        if (s == nullptr)
            break; // end of stream
        switch (s->state) {
            case ogg::HEADER_READY:
                if (s->type != ogg::UNKNOWN_STREAM) {
                    stream_count++;
                    sequence_numbers[s->stream.serialno] = stream_count;
                    if (!handler.relevant(stream_count))
                        s->downgrade();
                }
                remaining_streams++;
                break;
            case ogg::TAGS_READY:
                handler.list(sequence_numbers[s->stream.serialno], s->tags);
                s->downgrade(); // no more use for it
            default:
                remaining_streams--;
        }
        if (!full && remaining_streams <= 0) {
            break;
            // premature exit, but calls end_of_stream anyway
            // we want our optimization to be transparent to the TagsHandler
        }
    }
    handler.end_of_stream();
}

void opustags::edit_tags(
    ogg::Decoder &in, ogg::Encoder &out, ITagsHandler &handler)
{
    std::map<long, int> sequence_numbers;
    int stream_count = 0;
    std::shared_ptr<ogg::Stream> s;
    for (;;) {
        s = in.read_page();
        if (s == nullptr)
            break; // end of stream
        switch (s->state) {

            case ogg::HEADER_READY:
                if (s->type != ogg::UNKNOWN_STREAM) {
                    stream_count++;
                    sequence_numbers[s->stream.serialno] = stream_count;
                    if (!handler.relevant(stream_count))
                        s->downgrade(); // makes it UNKNOWN
                }
                if (s->type == ogg::UNKNOWN_STREAM) {
                    out.write_raw_page(in.current_page);
                } else {
                    out.forward(*s);
                }
                break;

            case ogg::TAGS_READY:
                handler.edit(sequence_numbers[s->stream.serialno], s->tags);
                handler.list(sequence_numbers[s->stream.serialno], s->tags);
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
    handler.end_of_stream();
}
