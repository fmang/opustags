#include <getopt.h>
#include <regex>
#include "tags_handlers/insertion_tags_handler.h"
#include "tags_handlers/modification_tags_handler.h"
#include "tags_handlers/external_edit_tags_handler.h"
#include "tags_handlers/export_tags_handler.h"
#include "tags_handlers/import_tags_handler.h"
#include "tags_handlers/listing_tags_handler.h"
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
    full(false)
{
}

Options opustags::parse_args(const int argc, char **argv)
{
    static const auto short_def = "hVeo:i::yd:a:s:Dl";

    static const option long_def[] = {
        {"help",        no_argument,        0, 'h'},
        {"version",     no_argument,        0, 'V'},
        {"full",        no_argument,        0, 0},
        {"output",      required_argument,  0, 'o'},
        {"in-place",    optional_argument,  0, 'i'},
        {"overwrite",   no_argument,        0, 'y'},
        {"delete",      required_argument,  0, 'd'},
        {"add",         required_argument,  0, 'a'},
        {"stream",      required_argument,  0, 0},
        {"set",         required_argument,  0, 's'},
        {"list",        no_argument,        0, 'l'},
        {"delete-all",  no_argument,        0, 'D'},
        {"edit",        no_argument,        0, 'e'},
        {"import",      no_argument,        0, 0},
        {"export",      no_argument,        0, 0},

        // TODO: parse no-colors

        {nullptr, 0, 0, 0}
    };

    // TODO: use --list as default switch

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

            case 'V':
                options.show_version = true;
                break;

            case 'o':
                if (arg.empty())
                    throw ArgumentError("Output path cannot be empty");
                options.path_out = arg;
                break;

            case 'i':
                // TODO: shouldn't we generate random file name here to which
                // we apply the arg, rather than use the arg as a whole?
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

            case 'l':
                options.tags_handler.add_handler(
                    std::make_shared<ListingTagsHandler>(
                        current_streamno, std::cout));
                break;

            case 'e':
                options.tags_handler.add_handler(
                    std::make_shared<ExternalEditTagsHandler>());
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
                else if (long_arg == "full")
                    options.full = true;
                else if (long_arg == "export")
                    options.tags_handler.add_handler(
                        std::make_shared<ExportTagsHandler>(std::cout));
                else if (long_arg == "import")
                    options.tags_handler.add_handler(
                        std::make_shared<ImportTagsHandler>(std::cin));
                break;
            }

            default:
                throw ArgumentError("Invalid flag");
        }
    }

    std::vector<std::string> stray;
    while (optind < argc)
        stray.push_back(argv[optind++]);

    if (!options.show_help && !options.show_version)
    {
        if (stray.empty())
            throw ArgumentError("Missing input path");

        options.path_in = stray.at(0);
        if (options.path_in.empty())
            throw ArgumentError("Input path cannot be empty");

        if (stray.size() > 1)
            throw ArgumentError("Extra argument: " + stray.at(1));
    }

    return options;
}
