#include "tags_handlers/export_tags_handler.h"

using namespace opustags;

ExportTagsHandler::ExportTagsHandler(std::ostream &output_stream)
    : output_stream(output_stream)
{
}

bool ExportTagsHandler::relevant(const int)
{
    return true;
}

void ExportTagsHandler::list(const int streamno, const Tags &tags)
{
    output_stream << "[Stream " << streamno << "]\n";
    for (const auto tag : tags.get_all())
        output_stream << tag.key << "=" << tag.value << "\n";
    output_stream << "\n";
}

bool ExportTagsHandler::edit(const int, Tags &)
{
    return false;
}

bool ExportTagsHandler::done()
{
    return false;
}
