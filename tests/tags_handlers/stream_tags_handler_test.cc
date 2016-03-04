#include "tags_handlers/stream_tags_handler.h"
#include "catch.h"

using namespace opustags;

namespace
{
    class DummyTagsHandler final : public StreamTagsHandler
    {
    public:
        DummyTagsHandler(const int streamno);

    protected:
        void list_impl(const Tags &) override;
        bool edit_impl(Tags &) override;

    public:
        bool list_fired, edit_fired;
    };
}

DummyTagsHandler::DummyTagsHandler(const int streamno)
    : StreamTagsHandler(streamno), list_fired(false), edit_fired(false)
{
}

void DummyTagsHandler::list_impl(const Tags &)
{
    list_fired = true;
}

bool DummyTagsHandler::edit_impl(Tags &)
{
    edit_fired = true;
    return true;
}

TEST_CASE("stream-based tags handler", "[tags_handlers]")
{
    SECTION("concrete stream") {
        const auto relevant_stream_number = 1;
        const auto irrelevant_stream_number = 2;
        Tags dummy_tags;
        DummyTagsHandler handler(relevant_stream_number);

        SECTION("relevance") {
            REQUIRE(!handler.relevant(irrelevant_stream_number));
            REQUIRE(handler.relevant(relevant_stream_number));
        }

        SECTION("listing") {
            handler.list(irrelevant_stream_number, dummy_tags);
            REQUIRE(!handler.list_fired);
            handler.list(relevant_stream_number, dummy_tags);
            REQUIRE(handler.list_fired);
        }

        SECTION("Editing") {
            REQUIRE(!handler.edit(irrelevant_stream_number, dummy_tags));
            REQUIRE(!handler.edit_fired);
            REQUIRE(handler.edit(relevant_stream_number, dummy_tags));
            REQUIRE(handler.edit_fired);
        }

        SECTION("finish through listing") {
            REQUIRE(!handler.edit(irrelevant_stream_number, dummy_tags));
            REQUIRE(!handler.done());
            REQUIRE(handler.edit(relevant_stream_number, dummy_tags));
            REQUIRE(handler.done());
        }

        SECTION("finish through editing") {
            handler.list(irrelevant_stream_number, dummy_tags);
            REQUIRE(!handler.done());
            handler.list(relevant_stream_number, dummy_tags);
            REQUIRE(handler.done());
        }
    }

    SECTION("any stream") {
        Tags dummy_tags;
        DummyTagsHandler handler(StreamTagsHandler::ALL_STREAMS);
        REQUIRE(handler.relevant(1));
        REQUIRE(handler.relevant(2));
        REQUIRE(handler.relevant(3));
        REQUIRE(!handler.done());
        handler.list(1, dummy_tags);
        REQUIRE(!handler.done());
        handler.list(2, dummy_tags);
        REQUIRE(!handler.done());
    }
}
