#include "tags_handlers/stream_tags_handler.h"

using namespace opustags;

const int StreamTagsHandler::ALL_STREAMS = -1;

StreamTagsHandler::StreamTagsHandler(const int streamno)
    : streamno(streamno), work_finished(false)
{
}

int StreamTagsHandler::get_streamno() const
{
    return streamno;
}

bool StreamTagsHandler::relevant(const int streamno)
{
    return streamno == this->streamno || this->streamno == ALL_STREAMS;
}

void StreamTagsHandler::list(const int streamno, const Tags &tags)
{
    if (!relevant(streamno))
        return;
    list_impl(tags);
    work_finished = this->streamno != ALL_STREAMS;
}

bool StreamTagsHandler::edit(const int streamno, Tags &tags)
{
    if (!relevant(streamno))
        return false;
    const auto ret = edit_impl(tags);
    work_finished = this->streamno != ALL_STREAMS;
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
