#include "actions.h"
#include "options.h"
#include "version.h"

#include <iostream>
#include <fstream>

static void show_usage(const bool include_help)
{
    static const auto usage =
        "Usage: opustags --help\n"
        "       opustags [OPTIONS] INPUT\n"
        "       opustags [OPTIONS] -o OUTPUT INPUT\n";

    static const auto help =
        "Options:\n"
        "  -h, --help              print this help\n"
        "  -V, --version           print version\n"
        "  -o, --output FILE       write the modified tags to this file\n"
        "  -i, --in-place [SUFFIX] use a temporary file then replace the original file\n"
        "  -y, --overwrite         overwrite the output file if it already exists\n"
        "      --stream ID         select stream for the next operations\n"
        "  -l, --list              display a pretty listing of all tags\n"
        "      --no-color          disable colors in --list output\n"
        "  -d, --delete FIELD      delete all the fields of a specified type\n"
        "  -a, --add FIELD=VALUE   add a field\n"
        "  -s, --set FIELD=VALUE   delete then add a field\n"
        "  -D, --delete-all        delete all the fields!\n"
        "      --full              enable full file scan\n"
        "      --export            dump the tags to standard output for --import\n"
        "      --import            set the tags from scratch basing on stanard input\n"
        "  -e, --edit              spawn the $EDITOR and apply --import on the result\n";

    std::cout << "opustags v" << opustags::version_short << "\n";
    std::cout << usage;
    if (include_help) {
        std::cout << "\n";
        std::cout << help;
    }
}

static void show_version()
{
    std::cout << "opustags v" << opustags::version_long << "\n";
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        show_usage(false);
        return EXIT_SUCCESS;
    }

    try {
        auto options = opustags::parse_args(argc, argv);
        if (options.show_help) {
            show_usage(true);
            return EXIT_SUCCESS;
        }
        if (options.show_version) {
            show_version();
            return EXIT_SUCCESS;
        }

        if (options.path_out.empty()) {
            std::ifstream in(options.path_in);
            opustags::ogg::Decoder dec(in);
            list_tags(dec, options.tags_handler);
            // TODO: report errors if user tries to edit the stream
        } else {
            std::ifstream in(options.path_in);
            std::ofstream out(options.path_out);
            opustags::ogg::Decoder dec(in);
            opustags::ogg::Encoder enc(out);
            edit_tags(dec, enc, options.tags_handler);
        }

    } catch (const std::exception &e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
