#include "tags_handlers/modification_tags_handler.h"

using namespace opustags;

ModificationTagsHandler::ModificationTagsHandler(
    const int streamno,
    const std::string &tag_key,
    const std::string &tag_value)
    : StreamTagsHandler(streamno), tag_key(tag_key), tag_value(tag_value)
{
}

bool ModificationTagsHandler::edit_impl(Tags &tags)
{
    tags.set(tag_key, tag_value);
    return true;
}
