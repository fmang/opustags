#pragma once

#include <iostream>
#include "tags_handler.h"

namespace opustags {

    class ExternalEditTagsHandler : public ITagsHandler
    {
    public:
        bool relevant(const int streamno) override;
        void list(const int streamno, const Tags &) override;
        bool edit(const int streamno, Tags &) override;
        bool done() override;
    };

}
