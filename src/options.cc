#include <getopt.h>
#include <regex>
#include "tags_handlers/insertion_tags_handler.h"
#include "tags_handlers/modification_tags_handler.h"
#include "tags_handlers/removal_tags_handler.h"
#include "options.h"

using namespace opustags;

ArgumentError::ArgumentError(const std::string &message)
    : std::runtime_error(message.c_str())
{
}

Options::Options() :
    show_help(false),
    overwrite(false),
    set_all(false)
{
}

Options opustags::parse_args(const int argc, char **argv)
{
    static const auto short_def = "ho:i::yd:a:s:DS";

    static const option long_def[] = {
        {"help",        no_argument,        0, 'h'},
        {"output",      required_argument,  0, 'o'},
        {"in-place",    optional_argument,  0, 'i'},
        {"overwrite",   no_argument,        0, 'y'},
        {"delete",      required_argument,  0, 'd'},
        {"add",         required_argument,  0, 'a'},
        {"stream",      required_argument,  0, 0},
        {"set",         required_argument,  0, 's'},
        {"delete-all",  no_argument,        0, 'D'},
        {"set-all",     no_argument,        0, 'S'},

        {nullptr, 0, 0, 0}
    };

    Options options;

    int current_streamno = StreamTagsHandler::ALL_STREAMS;
    int option_index;
    char c;
    optind = 0;

    while ((c = getopt_long(
        argc, argv, short_def, long_def, &option_index)) != -1) {

        const std::string arg(optarg == nullptr ? "" : optarg);

        switch (c) {
            case 'h':
                options.show_help = true;
                break;

            case 'o':
                options.path_out = arg;
                break;

            case 'i':
                options.path_out = arg.empty() ? ".otmp" : arg;
                options.in_place = true;
                break;

            case 'y':
                options.overwrite = true;
                break;

            case 'd':
                if (arg.find('=') != std::string::npos)
                    throw ArgumentError("Invalid field: '" + arg + "'");
                options.tags_handler.add_handler(
                    std::make_shared<RemovalTagsHandler>(
                        current_streamno, arg));
                break;

            case 'a':
            case 's':
            {
                std::smatch match;
                std::regex regex("^(\\w+)=(.*)$");
                if (!std::regex_match(arg, match, regex))
                    throw ArgumentError("Invalid field: '" + arg + "'");
                if (c == 's')
                {
                    options.tags_handler.add_handler(
                        std::make_shared<ModificationTagsHandler>(
                            current_streamno, match[1], match[2]));
                }
                else
                {
                    options.tags_handler.add_handler(
                        std::make_shared<InsertionTagsHandler>(
                            current_streamno, match[1], match[2]));
                }
                break;
            }

            case 'S':
                options.set_all = true;
                break;

            case 'D':
                options.tags_handler.add_handler(
                    std::make_shared<RemovalTagsHandler>(current_streamno));
                break;

            case 0:
            {
                std::string long_arg = long_def[option_index].name;
                if (long_arg == "stream")
                    current_streamno = atoi(optarg);
                break;
            }

            default:
                throw ArgumentError("Invalid flag");
        }
    }

    return options;
}
