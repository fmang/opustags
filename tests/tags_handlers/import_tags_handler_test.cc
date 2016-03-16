#include "tags_handlers/import_tags_handler.h"
#include "catch.h"
#include <sstream>

using namespace opustags;

TEST_CASE("import tags handler", "[tags_handlers]")
{
    SECTION("tags for streams not mentioned in import get emptied")
    {
        Tags tags = {{{"remove", "me"}}};
        std::stringstream ss;
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().empty());
    }

    SECTION("streams that do not exist in the input file are ignored")
    {
        // TODO: rather than ignoring, at least print a warning
        // requires #6

        Tags tags;
        std::stringstream ss;
        ss << "[Stream 5]\nkey = value\n";
        ImportTagsHandler handler(ss);
        REQUIRE(!handler.edit(1, tags));
        REQUIRE(tags.get_all().empty());
    }

    SECTION("adding unique tags")
    {
        Tags tags;
        std::stringstream ss;
        ss << "[Stream 1]\nkey = value\nkey2 = value2\n";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().size() == 2);
        REQUIRE(tags.get("key") == "value");
        REQUIRE(tags.get("key2") == "value2");
    }

    SECTION("adding tags with the same key")
    {
        Tags tags;
        std::stringstream ss;
        ss << "[Stream 1]\nkey = value1\nkey = value2\n";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().size() == 2);
        REQUIRE(tags.get_all().at(0).key == "key");
        REQUIRE(tags.get_all().at(1).key == "key");
        REQUIRE(tags.get_all().at(0).value == "value1");
        REQUIRE(tags.get_all().at(1).value == "value2");
    }

    SECTION("overwriting existing tags")
    {
        Tags tags = {{{"remove", "me"}}};
        std::stringstream ss;
        ss << "[Stream 1]\nkey = value\nkey2 = value2\n";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().size() == 2);
        REQUIRE(tags.get("key") == "value");
        REQUIRE(tags.get("key2") == "value2");
    }

    SECTION("various whitespace issues are worked around")
    {
        Tags tags;
        std::stringstream ss;
        ss << " [StrEaM  1] \n\n  key = value  \nkey2=value2\n";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().size() == 2);
        REQUIRE(tags.get("key") == "value");
        REQUIRE(tags.get("key2") == "value2");
    }

    SECTION("not specifying stream assumes first stream")
    {
        Tags tags;
        std::stringstream ss;
        ss << "key=value";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().size() == 1);
        REQUIRE(tags.get("key") == "value");
    }

    SECTION("multiple streams")
    {
        Tags tags1;
        Tags tags2;
        std::stringstream ss;
        ss << "[stream 1]\nkey=value\n[stream 2]\nkey2=value2";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags1));
        REQUIRE(handler.edit(2, tags2));
        REQUIRE(tags1.get_all().size() == 1);
        REQUIRE(tags2.get_all().size() == 1);
        REQUIRE(tags1.get("key") == "value");
        REQUIRE(tags2.get("key2") == "value2");
    }

    SECTION("sections listed twice are concatenated")
    {
        Tags tags;
        std::stringstream ss;
        ss << "[stream 1]\nkey=value\n"
            "[stream 2]\nkey=irrelevant\n"
            "[Stream 1]\nkey2=value2";
        ImportTagsHandler handler(ss);
        REQUIRE(handler.edit(1, tags));
        REQUIRE(tags.get_all().size() == 2);
        REQUIRE(tags.get("key") == "value");
        REQUIRE(tags.get("key2") == "value2");
    }

    SECTION("weird input throws errors - malformed section headers")
    {
        Tags tags;
        std::stringstream ss;
        ss << "[stream huh]\n";
        REQUIRE_THROWS({
            ImportTagsHandler handler(ss);
            handler.edit(1, tags);
        });
    }

    SECTION("weird input throws errors - malformed lines")
    {
        Tags tags;
        std::stringstream ss;
        ss << "tag huh\n";
        REQUIRE_THROWS({
            ImportTagsHandler handler(ss);
            handler.edit(1, tags);
        });
    }
}
