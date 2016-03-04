#include "tags_handlers/modification_tags_handler.h"
#include "catch.h"

using namespace opustags;

TEST_CASE("modification tags handler", "[tags_handlers]")
{
    const auto streamno = 1;
    const auto first_tag_key = "tag_key";
    const auto other_tag_key = "other_tag_key";
    const auto dummy_value = "dummy";
    const auto new_value = "dummy 2";

    Tags tags;
    tags.add(first_tag_key, dummy_value);

    REQUIRE(tags.get_all().size() == 1);

    // setting nonexistent keys adds them
    ModificationTagsHandler handler1(streamno, other_tag_key, dummy_value);
    REQUIRE(handler1.edit(streamno, tags));
    REQUIRE(tags.get_all().size() == 2);
    REQUIRE(tags.get(other_tag_key) == dummy_value);

    // setting existing keys overrides their values
    ModificationTagsHandler handler2(streamno, other_tag_key, new_value);
    REQUIRE(handler2.edit(streamno, tags));
    REQUIRE(tags.get_all().size() == 2);
    REQUIRE(tags.get(other_tag_key) == new_value);

    // setting existing keys reports no modifications if values are the same
    REQUIRE(!handler2.edit(streamno, tags));
}
