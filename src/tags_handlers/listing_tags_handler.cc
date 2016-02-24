#include "tags_handlers/listing_tags_handler.h"

using namespace opustags;

ListingTagsHandler::ListingTagsHandler(
    const int streamno,
    std::ostream &output_stream)
    : StreamTagsHandler(streamno), output_stream(output_stream)
{
}

void ListingTagsHandler::list_impl(const Tags &tags)
{
    for (const auto &kv : tags.get_all())
        output_stream << std::get<0>(kv) << "=" << std::get<1>(kv) << "\n";
}
