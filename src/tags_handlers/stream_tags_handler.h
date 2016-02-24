#pragma once

#include "tags_handler.h"

namespace opustags {

    // Base handler that holds the stream number it's supposed to work with
    // and performs the usual boilerplate.
    class StreamTagsHandler : public ITagsHandler
    {
    public:
        StreamTagsHandler(const int streamno);

        int get_streamno() const;

        bool relevant(const int streamno) override;
        void list(const int streamno, const Tags &) override;
        bool edit(const int streamno, Tags &) override;
        bool done() override;

    protected:
        virtual void list_impl(const Tags &);
        virtual bool edit_impl(Tags &);

    private:
        const int streamno;
        bool work_finished;
    };

}
