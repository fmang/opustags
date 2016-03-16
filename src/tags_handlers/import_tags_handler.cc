#include "tags_handlers/import_tags_handler.h"

using namespace opustags;

ImportTagsHandler::ImportTagsHandler(std::istream &input_stream)
    : input_stream(input_stream)
{
}

bool ImportTagsHandler::relevant(const int streamno)
{
    return false;
}

void ImportTagsHandler::list(const int streamno, const Tags &)
{
}

bool ImportTagsHandler::edit(const int streamno, Tags &)
{
    return false;
}

bool ImportTagsHandler::done()
{
    return true;
}
