#pragma once

#include <stdexcept>
#include <map>
#include <vector>
#include "tags_handlers/composite_tags_handler.h"

namespace opustags
{
    struct Options final
    {
        Options();

        bool show_help;
        bool show_version;
        bool overwrite;
        bool full;

        bool in_place;
        std::string path_in;
        std::string path_out;

        CompositeTagsHandler tags_handler;
    };

    struct ArgumentError : std::runtime_error
    {
        ArgumentError(const std::string &message);
    };

    Options parse_args(const int argc, char **argv);
}
