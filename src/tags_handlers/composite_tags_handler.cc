#include "tags_handlers/composite_tags_handler.h"

using namespace opustags;

void CompositeTagsHandler::add_handler(std::shared_ptr<ITagsHandler> handler)
{
    handlers.push_back(std::move(handler));
}

bool CompositeTagsHandler::relevant(const int streamno)
{
    for (const auto &handler : handlers)
        if (handler->relevant(streamno))
            return true;
    return false;
}

void CompositeTagsHandler::list(const int streamno, const Tags &tags)
{
    for (const auto &handler : handlers)
        handler->list(streamno, tags);
}

bool CompositeTagsHandler::edit(const int streamno, Tags &tags)
{
    bool modified = false;
    for (const auto &handler : handlers)
        modified |= handler->edit(streamno, tags);
    return modified;
}

bool CompositeTagsHandler::done()
{
    bool done = true;
    for (const auto &handler : handlers)
        done &= handler->done();
    return done;
}
