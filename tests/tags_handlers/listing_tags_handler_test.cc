#include "tags_handlers/listing_tags_handler.h"
#include "catch.h"
#include <sstream>

using namespace opustags;

TEST_CASE("Listing tags handler test")
{
    const auto streamno = 1;

    Tags tags;
    tags.add("z", "value1");
    tags.add("a", "value2");
    tags.add("y", "value3");
    tags.add("c", "value4");

    std::stringstream ss;
    ListingTagsHandler handler(streamno, ss);
    handler.list(streamno, tags);

    REQUIRE(ss.str() == "z=value1\na=value2\ny=value3\nc=value4\n");
}
