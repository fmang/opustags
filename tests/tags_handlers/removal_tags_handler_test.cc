#include "tags_handlers/removal_tags_handler.h"
#include "catch.h"

using namespace opustags;

TEST_CASE("Removal tags handler test")
{
    const auto streamno = 1;
    const auto expected_tag_key = "tag_key";
    const auto other_tag_key = "other_tag_key";
    const auto dummy_value = "dummy";
    RemovalTagsHandler handler(streamno, expected_tag_key);

    Tags tags = {{expected_tag_key, dummy_value}, {other_tag_key, dummy_value}};
    REQUIRE(tags.size() == 2);
    REQUIRE(handler.edit(streamno, tags));
    REQUIRE(tags.size() == 1);
    REQUIRE(tags.find(other_tag_key) != tags.end());
    REQUIRE_THROWS(handler.edit(streamno, tags));
}
