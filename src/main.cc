#include <iostream>
#include "options.h"

static const auto version = "2.x";

static void show_usage(const bool include_help)
{
    static const auto usage =
        "Usage: opustags --help\n"
        "       opustags [OPTIONS] INPUT\n"
        "       opustags [OPTIONS] -o OUTPUT INPUT\n";

    static const auto help =
        "Options:\n"
        "  -h, --help              print this help\n"
        "  -o, --output            write the modified tags to a file\n"
        "  -i, --in-place [SUFFIX] use a temporary file then replace the original file\n"
        "  -y, --overwrite         overwrite the output file if it already exists\n"
        "  -d, --delete FIELD      delete all the fields of a specified type\n"
        "  -a, --add FIELD=VALUE   add a field\n"
        "  -s, --set FIELD=VALUE   delete then add a field\n"
        "  -D, --delete-all        delete all the fields!\n"
        "  -S, --set-all           read the fields from stdin\n";

    std::cout << "opustags v" << version << "\n";
    std::cout << usage;
    if (include_help) {
        std::cout << "\n";
        std::cout << help;
    }
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        show_usage(false);
        return EXIT_SUCCESS;
    }

    try {
        const auto options = opustags::parse_args(argc, argv);
        if (options.show_help) {
            show_usage(true);
            return EXIT_SUCCESS;
        }

        std::cout << "Working...\n";
        std::cout << "Input path: " << options.path_in << "\n";
    } catch (const std::exception &e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
