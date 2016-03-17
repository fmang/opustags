#include <regex>
#include "tags_handlers/import_tags_handler.h"

using namespace opustags;

ImportTagsHandler::ImportTagsHandler(std::istream &input_stream)
    : parsed(false), input_stream(input_stream)
{
}

bool ImportTagsHandler::relevant(const int)
{
    return true;
}

void ImportTagsHandler::list(const int, const Tags &)
{
}

bool ImportTagsHandler::edit(const int streamno, Tags &tags)
{
    // the reason why we do it this way is because the tests indirectly create
    // this handler with std::cin, and we do not want the constructor to block!
    parse_input_stream_if_needed();

    const auto old_tags = tags;
    tags.clear();

    if (tag_map.find(streamno) != tag_map.end()) {
        const auto &source_tags = tag_map.at(streamno);
        for (const auto &source_tag : source_tags.get_all())
            tags.add(source_tag.key, source_tag.value);
    }

    return old_tags != tags;
}

bool ImportTagsHandler::done()
{
    return false;
}

void ImportTagsHandler::parse_input_stream_if_needed()
{
    if (parsed)
        return;
    parsed = true;

    const std::regex whitespace_regex("^\\s*$");
    const std::regex stream_header_regex(
        "^\\s*\\[stream\\s+(\\d+)\\]\\s*$", std::regex_constants::icase);
    const std::regex tag_regex(
        "^\\s*([a-z0-9]+)\\s*=\\s*(.*?)\\s*$", std::regex_constants::icase);

    int current_stream_number = 1;
    std::string line;
    while (std::getline(input_stream, line)) {
        std::smatch match;
        if (std::regex_match(line, match, stream_header_regex)) {
            current_stream_number = std::atoi(match[1].str().c_str());
        } else if (std::regex_match(line, match, tag_regex)) {
            tag_map[current_stream_number].add(match[1], match[2]);
        } else if (!std::regex_match(line, match, whitespace_regex)) {
            throw std::runtime_error("Malformed input data near line " + line);
        }
    }
}
