#pragma once

#include <stdexcept>
#include <map>
#include <vector>

namespace opustags
{
    struct Options final
    {
        Options();

        bool show_help;
        bool overwrite;
        bool delete_all;
        bool set_all;

        std::string in_place; // string?
        std::string path_out;
        std::map<std::string, std::string> to_add;
        std::vector<std::string> to_delete;
    };

    struct ArgumentError : std::runtime_error
    {
        ArgumentError(const std::string &message);
    };

    Options parse_args(const int argc, char **argv);
}
