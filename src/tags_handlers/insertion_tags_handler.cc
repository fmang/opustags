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

bool InsertionTagsHandler::edit_impl(Tags &tags)
{
    if (tags.find(tag_key) != tags.end())
        throw TagAlreadyExistsError(tag_key);

    tags[tag_key] = tag_value;
    return true;
}
