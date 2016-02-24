#include "tags_handlers/removal_tags_handler.h"
#include "tags_handlers_errors.h"

using namespace opustags;

RemovalTagsHandler::RemovalTagsHandler(const int streamno)
    : StreamTagsHandler(streamno)
{
}

RemovalTagsHandler::RemovalTagsHandler(
    const int streamno, const std::string &tag_key)
    : StreamTagsHandler(streamno), tag_key(tag_key)
{
}

bool RemovalTagsHandler::edit_impl(Tags &tags)
{
    if (tag_key.empty())
    {
        const auto all_tags = tags.get_all();
        for (const auto &kv : all_tags)
            tags.remove(std::get<0>(kv));
        return !all_tags.empty();
    }
    else
    {
        if (!tags.contains(tag_key))
            throw TagDoesNotExistError(tag_key);

        tags.remove(tag_key);
        return true;
    }
}
