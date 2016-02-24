#include "tags_handlers/listing_tags_handler.h"
#include "catch.h"
#include <sstream>

using namespace opustags;

TEST_CASE("Listing tags handler test")
{
    const auto streamno = 1;

    Tags tags;
    tags.set("z", "value1");
    tags.set("a", "value2");
    tags.set("y", "value3");
    tags.set("c", "value4");

    std::stringstream ss;
    ListingTagsHandler handler(streamno, ss);
    handler.list(streamno, tags);

    REQUIRE(ss.str() == "z=value1\na=value2\ny=value3\nc=value4\n");
}
