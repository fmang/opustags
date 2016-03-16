#include "tags_handlers/export_tags_handler.h"

using namespace opustags;

ExportTagsHandler::ExportTagsHandler(std::ostream &output_stream)
    : output_stream(output_stream)
{
}

bool ExportTagsHandler::relevant(const int streamno)
{
    return false;
}

void ExportTagsHandler::list(const int streamno, const Tags &)
{
}

bool ExportTagsHandler::edit(const int streamno, Tags &)
{
    return false;
}

bool ExportTagsHandler::done()
{
    return true;
}
