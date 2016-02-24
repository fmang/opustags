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
    if (!tags.contains(tag_key))
        throw TagDoesNotExistError(tag_key);

    tags.remove(tag_key);
    return true;
}
