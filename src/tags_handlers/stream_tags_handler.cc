#include "tags_handlers/stream_tags_handler.h"

using namespace opustags;

StreamTagsHandler::StreamTagsHandler(const int streamno)
    : streamno(streamno), work_finished(false)
{
}

bool StreamTagsHandler::relevant(const int streamno)
{
    return streamno == this->streamno;
}

void StreamTagsHandler::list(const int streamno, const Tags &tags)
{
    if (streamno != this->streamno)
        return;
    list_impl(tags);
    work_finished = true;
}

bool StreamTagsHandler::edit(const int streamno, Tags &tags)
{
    if (streamno != this->streamno)
        return false;
    const auto ret = edit_impl(tags);
    work_finished = true;
    return ret;
}

bool StreamTagsHandler::done()
{
    return work_finished;
}

void StreamTagsHandler::list_impl(const Tags &)
{
}

bool StreamTagsHandler::edit_impl(Tags &)
{
    return false;
}
