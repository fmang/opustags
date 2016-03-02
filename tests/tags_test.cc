#include "tags.h"
#include "catch.h"

using namespace opustags;

TEST_CASE("Tag manipulation test", "[tags]")
{
    SECTION("Basic operations") {
        Tags tags;
        REQUIRE(!tags.contains("a"));
        tags.set("a", "1");
        REQUIRE(tags.get("a") == "1");
        REQUIRE(tags.contains("a"));
        tags.remove("a");
        REQUIRE(!tags.contains("a"));
        REQUIRE_THROWS(tags.get("a"));
    }

    SECTION("Maintaing order of insertions") {
        Tags tags;
        tags.set("z", "1");
        tags.set("y", "2");
        tags.set("x", "3");
        tags.set("y", "4");

        REQUIRE(tags.get_all().size() == 3);
        REQUIRE(std::get<0>(tags.get_all()[0]) == "z");
        REQUIRE(std::get<0>(tags.get_all()[1]) == "x");
        REQUIRE(std::get<0>(tags.get_all()[2]) == "y");

        tags.remove("z");
        REQUIRE(tags.get_all().size() == 2);
        REQUIRE(std::get<0>(tags.get_all()[0]) == "x");
        REQUIRE(std::get<0>(tags.get_all()[1]) == "y");

        tags.set("gamma", "5");
        REQUIRE(tags.get_all().size() == 3);
        REQUIRE(std::get<0>(tags.get_all()[0]) == "x");
        REQUIRE(std::get<0>(tags.get_all()[1]) == "y");
        REQUIRE(std::get<0>(tags.get_all()[2]) == "gamma");
    }

    SECTION("Key to multiple values") {
        // ARTIST is set once per artist.
        // https://www.xiph.org/vorbis/doc/v-comment.html
        Tags tags;
        tags.set("ARTIST", "You");
        tags.set("ARTIST", "Me");
        REQUIRE(tags.get_all().size() == 2);
    }

    SECTION("Raw set") {
        Tags tags;
        tags.set("TITLE=Foo=Bar");
        REQUIRE(tags.get("TITLE") == "Foo=Bar");
    }
}
