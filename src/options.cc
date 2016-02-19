#include <getopt.h>
#include <regex>
#include "options.h"

using namespace opustags;

ArgumentError::ArgumentError(const std::string &message)
    : std::runtime_error(message.c_str())
{
}

Options::Options() :
    show_help(false),
    overwrite(false),
    delete_all(false),
    set_all(false)
{
}

Options opustags::parse_args(const int argc, char **argv)
{
    static const auto short_def = "ho:i::yd:a:s:DS";
    static const option long_def[] = {
        {"help", no_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {"in-place", optional_argument, 0, 'i'},
        {"overwrite", no_argument, 0, 'y'},
        {"delete", required_argument, 0, 'd'},
        {"add", required_argument, 0, 'a'},
        {"set", required_argument, 0, 's'},
        {"delete-all", no_argument, 0, 'D'},
        {"set-all", no_argument, 0, 'S'},
        {NULL, 0, 0, 0}
    };

    Options options;
    char c;
    optind = 0;
    while ((c = getopt_long(argc, argv, short_def, long_def, nullptr)) != -1)
    {
        const std::string arg(optarg == nullptr ? "" : optarg);

        switch (c)
        {
            case 'h':
                options.show_help = true;
                break;

            case 'o':
                options.path_out = arg;
                break;

            case 'i':
                options.in_place = arg.empty() ? ".otmp" : arg;
                break;

            case 'y':
                options.overwrite = true;
                break;

            case 'd':
                if (arg.find('=') != std::string::npos)
                    throw ArgumentError("Invalid field: '" + arg + "'");
                options.to_delete.push_back(arg);
                break;

            case 'a':
            case 's':
            {
                std::smatch match;
                std::regex regex("^(\\w+)=(.*)$");
                if (!std::regex_match(arg, match, regex))
                    throw ArgumentError("Invalid field: '" + arg + "'");
                options.to_add[match[1]] = match[2];
                if (c == 's')
                    options.to_delete.push_back(match[1]);
                break;
            }

            case 'S':
                options.set_all = true;
                break;

            case 'D':
                options.delete_all = true;
                break;

            default:
                throw ArgumentError("Invalid flag");
        }
    }

    return options;
}
