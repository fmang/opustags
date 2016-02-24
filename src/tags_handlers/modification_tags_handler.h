#pragma once

#include "tags_handlers/stream_tags_handler.h"

namespace opustags {

    class ModificationTagsHandler : public StreamTagsHandler
    {
    public:
        ModificationTagsHandler(
            const int streamno,
            const std::string &tag_key,
            const std::string &tag_value);

        std::string get_tag_key() const;
        std::string get_tag_value() const;

    protected:
        bool edit_impl(Tags &) override;

    private:
        const std::string tag_key;
        const std::string tag_value;
    };

}
