#pragma once

#include <iostream>
#include <map>
#include "tags_handler.h"

namespace opustags {

    class ImportTagsHandler : public ITagsHandler
    {
    public:
        ImportTagsHandler(std::istream &input_stream);

        bool relevant(const int streamno) override;
        void list(const int streamno, const Tags &) override;
        bool edit(const int streamno, Tags &) override;
        bool done() override;

    private:
        void parse_input_stream_if_needed();

        bool parsed;
        std::istream &input_stream;
        std::map<int, Tags> tag_map;
    };

}
