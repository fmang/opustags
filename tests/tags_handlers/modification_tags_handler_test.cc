#include "tags_handlers/modification_tags_handler.h"
#include "catch.h"

using namespace opustags;

TEST_CASE("Modification tags handler test")
{
    const auto streamno = 1;
    const auto first_tag_key = "tag_key";
    const auto other_tag_key = "other_tag_key";
    const auto dummy_value = "dummy";
    const auto new_value = "dummy 2";

    Tags tags = {{first_tag_key, dummy_value}};
    REQUIRE(tags.size() == 1);

    // setting nonexistent keys adds them
    ModificationTagsHandler handler1(streamno, other_tag_key, dummy_value);
    REQUIRE(handler1.edit(streamno, tags));
    REQUIRE(tags.size() == 2);
    REQUIRE(tags[other_tag_key] == dummy_value);

    // setting existing keys overrides their values
    ModificationTagsHandler handler2(streamno, other_tag_key, new_value);
    REQUIRE(handler2.edit(streamno, tags));
    REQUIRE(tags.size() == 2);
    REQUIRE(tags[other_tag_key] == new_value);
}
