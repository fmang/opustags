#include "tags_handlers/external_edit_tags_handler.h"

using namespace opustags;

bool ExternalEditTagsHandler::relevant(const int streamno)
{
    return false;
}

void ExternalEditTagsHandler::list(const int streamno, const Tags &)
{
}

bool ExternalEditTagsHandler::edit(const int streamno, Tags &)
{
    return false;
}

bool ExternalEditTagsHandler::done()
{
    return true;
}
