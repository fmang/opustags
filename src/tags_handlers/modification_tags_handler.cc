#include "tags_handlers/modification_tags_handler.h"

using namespace opustags;

ModificationTagsHandler::ModificationTagsHandler(
    const int streamno,
    const std::string &tag_key,
    const std::string &tag_value)
    : StreamTagsHandler(streamno), tag_key(tag_key), tag_value(tag_value)
{
}

std::string ModificationTagsHandler::get_tag_key() const
{
    return tag_key;
}

std::string ModificationTagsHandler::get_tag_value() const
{
    return tag_value;
}

bool ModificationTagsHandler::edit_impl(Tags &tags)
{
    if (tags.contains(tag_key))
    {
        if (tags.get(tag_key) == tag_value)
            return false;
        tags.remove(tag_key);
    }

    tags.add(tag_key, tag_value);
    return true;
}
