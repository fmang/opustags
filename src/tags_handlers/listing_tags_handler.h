#pragma once

#include <iostream>
#include "tags_handlers/stream_tags_handler.h"

namespace opustags {

    class ListingTagsHandler : public StreamTagsHandler
    {
    public:
        ListingTagsHandler(const int streamno, std::ostream &output_stream);

    protected:
        void list_impl(const Tags &) override;

    private:
        std::ostream &output_stream;
    };

}
