#include "tags_handlers/insertion_tags_handler.h"
#include "tags_handlers_errors.h"

using namespace opustags;

InsertionTagsHandler::InsertionTagsHandler(
    const int streamno,
    const std::string &tag_key,
    const std::string &tag_value)
    : StreamTagsHandler(streamno), tag_key(tag_key), tag_value(tag_value)
{
}

std::string InsertionTagsHandler::get_tag_key() const
{
    return tag_key;
}

std::string InsertionTagsHandler::get_tag_value() const
{
    return tag_value;
}

bool InsertionTagsHandler::edit_impl(Tags &tags)
{
    tags.add(tag_key, tag_value);
    return true;
}
