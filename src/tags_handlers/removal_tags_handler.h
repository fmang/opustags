#pragma once

#include "tags_handlers/stream_tags_handler.h"

namespace opustags {

    class RemovalTagsHandler : public StreamTagsHandler
    {
    public:
        RemovalTagsHandler(const int streamno);
        RemovalTagsHandler(const int streamno, const std::string &tag_key);

    protected:
        bool edit_impl(Tags &) override;

    private:
        const std::string tag_key;
    };

}
