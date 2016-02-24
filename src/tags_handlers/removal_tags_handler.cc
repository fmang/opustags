#include "tags_handlers/removal_tags_handler.h"
#include "tags_handlers_errors.h"

using namespace opustags;

RemovalTagsHandler::RemovalTagsHandler(
    const int streamno, const std::string &tag_key)
    : StreamTagsHandler(streamno), tag_key(tag_key)
{
}

bool RemovalTagsHandler::edit_impl(Tags &tags)
{
    if (tags.find(tag_key) == tags.end())
        throw TagDoesNotExistError(tag_key);

    tags.erase(tag_key);
    return true;
}
