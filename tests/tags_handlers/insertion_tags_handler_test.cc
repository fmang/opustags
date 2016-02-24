#include "tags_handlers/insertion_tags_handler.h"
#include "catch.h"

using namespace opustags;

TEST_CASE("Insertion tags handler test")
{
    Tags tags;
    const auto streamno = 1;
    const auto expected_tag_key = "tag_key";
    const auto expected_tag_value = "tag_value";
    InsertionTagsHandler handler(
        streamno, expected_tag_key, expected_tag_value);

    REQUIRE(handler.edit(streamno, tags));
    REQUIRE(tags.get_all().size() == 1);
    REQUIRE(tags.get(expected_tag_key) == expected_tag_value);
    REQUIRE_THROWS(handler.edit(streamno, tags));
}
