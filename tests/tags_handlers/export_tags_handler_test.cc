#include "tags_handlers/export_tags_handler.h"
#include "catch.h"
#include <sstream>

using namespace opustags;

TEST_CASE("export tags handler", "[tags_handlers]")
{
    std::stringstream ss;
    ExportTagsHandler handler(ss);
    handler.list(1, {{{"a", "value1"}, {"b", "value2"}}});
    handler.list(2, {{{"c", "value3"}, {"d", "value4"}}});

    REQUIRE(ss.str() == R"([Stream 1]
a=value1
b=value2

[Stream 2]
c=value3
d=value4

)");
}
