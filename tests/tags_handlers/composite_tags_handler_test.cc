#include "tags_handlers/composite_tags_handler.h"
#include "catch.h"

using namespace opustags;

namespace
{
    struct DummyTagsHandler : ITagsHandler
    {
        DummyTagsHandler();

        bool relevant(const int streamno) override;
        void list(const int streamno, const Tags &) override;
        bool edit(const int streamno, Tags &) override;
        bool done() override;

        bool relevant_ret, edit_ret, done_ret, list_fired;
    };
}

DummyTagsHandler::DummyTagsHandler()
    : relevant_ret(true), edit_ret(true), done_ret(true), list_fired(false)
{
}

bool DummyTagsHandler::relevant(const int streamno)
{
    return relevant_ret;
}

void DummyTagsHandler::list(const int streamno, const Tags &)
{
    list_fired = true;
}

bool DummyTagsHandler::edit(const int streamno, Tags &)
{
    return edit_ret;
}

bool DummyTagsHandler::done()
{
    return done_ret;
}

TEST_CASE("composite tags handler", "[tags_handlers]")
{
    auto handler1 = std::make_shared<DummyTagsHandler>();
    auto handler2 = std::make_shared<DummyTagsHandler>();
    CompositeTagsHandler composite_handler;
    composite_handler.add_handler(handler1);
    composite_handler.add_handler(handler2);

    SECTION("relevance") {
        const int dummy_streamno = 1;
        handler1->relevant_ret = true;
        handler2->relevant_ret = true;
        REQUIRE(composite_handler.relevant(dummy_streamno));
        handler1->relevant_ret = false;
        REQUIRE(composite_handler.relevant(dummy_streamno));
        handler2->relevant_ret = false;
        REQUIRE(!composite_handler.relevant(dummy_streamno));
    }

    SECTION("listing") {
        const int dummy_streamno = 1;
        Tags dummy_tags;
        REQUIRE(!handler1->list_fired);
        REQUIRE(!handler2->list_fired);
        composite_handler.list(dummy_streamno, dummy_tags);
        REQUIRE(handler1->list_fired);
        REQUIRE(handler2->list_fired);
    }

    SECTION("editing") {
        const int dummy_streamno = 1;
        Tags dummy_tags;
        handler1->edit_ret = true;
        handler2->edit_ret = true;
        REQUIRE(composite_handler.edit(dummy_streamno, dummy_tags));
        handler1->edit_ret = false;
        REQUIRE(composite_handler.edit(dummy_streamno, dummy_tags));
        handler2->edit_ret = false;
        REQUIRE(!composite_handler.edit(dummy_streamno, dummy_tags));
    }

    SECTION("finish") {
        handler1->done_ret = true;
        handler2->done_ret = true;
        REQUIRE(composite_handler.done());
        handler1->done_ret = false;
        REQUIRE(!composite_handler.done());
        handler2->done_ret = false;
        REQUIRE(!composite_handler.done());
    }
}
