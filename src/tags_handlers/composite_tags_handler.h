#pragma once

#include <memory>
#include <vector>
#include "tags_handler.h"

namespace opustags {

    class CompositeTagsHandler final : public ITagsHandler
    {
    public:
        void add_handler(std::shared_ptr<ITagsHandler> handler);

        bool relevant(const int streamno) override;
        void list(const int streamno, const Tags &) override;
        bool edit(const int streamno, Tags &) override;
        bool done() override;

        const std::vector<std::shared_ptr<ITagsHandler>> get_handlers() const;

    private:
        std::vector<std::shared_ptr<ITagsHandler>> handlers;
    };

}
