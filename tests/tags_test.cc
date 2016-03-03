#include "tags.h"
#include "catch.h"

using namespace opustags;

TEST_CASE("Tag manipulation test", "[tags]")
{
    SECTION("Basic operations") {
        Tags tags;
        REQUIRE(!tags.contains("a"));
        tags.add("a", "1");
        REQUIRE(tags.get("a") == "1");
        REQUIRE(tags.contains("a"));
        tags.remove("a");
        REQUIRE(!tags.contains("a"));
        REQUIRE_THROWS(tags.get("a"));
    }

    SECTION("Clearing") {
        Tags tags;
        tags.add("a", "1");
        tags.add("b", "2");
        REQUIRE(tags.get_all().size() == 2);
        tags.clear();
        REQUIRE(tags.get_all().size() == 0);
    }

    SECTION("Maintaing order of insertions") {
        Tags tags;
        tags.add("z", "1");
        tags.add("y", "2");
        tags.add("x", "3");
        tags.add("y", "4");

        REQUIRE(tags.get_all().size() == 4);
        REQUIRE(tags.get_all()[0].key == "z");
        REQUIRE(tags.get_all()[1].key == "y");
        REQUIRE(tags.get_all()[2].key == "x");
        REQUIRE(tags.get_all()[3].key == "y");

        tags.remove("z");
        REQUIRE(tags.get_all().size() == 3);
        REQUIRE(tags.get_all()[0].key == "y");
        REQUIRE(tags.get_all()[1].key == "x");
        REQUIRE(tags.get_all()[2].key == "y");

        tags.add("gamma", "5");
        REQUIRE(tags.get_all().size() == 4);
        REQUIRE(tags.get_all()[0].key == "y");
        REQUIRE(tags.get_all()[1].key == "x");
        REQUIRE(tags.get_all()[2].key == "y");
        REQUIRE(tags.get_all()[3].key == "gamma");
    }

    SECTION("Key to multiple values") {
        // ARTIST is set once per artist.
        // https://www.xiph.org/vorbis/doc/v-comment.html
        Tags tags;
        tags.add("ARTIST", "You");
        tags.add("ARTIST", "Me");
        REQUIRE(tags.get_all().size() == 2);
    }

    SECTION("Removing multivalues should remove all of them") {
        Tags tags;
        tags.add("ARTIST", "You");
        tags.add("ARTIST", "Me");
        tags.remove("ARTIST");
        REQUIRE(tags.get_all().size() == 0);
    }

    SECTION("Raw set") {
        Tags tags;
        tags.add("TITLE=Foo=Bar");
        REQUIRE(tags.get("TITLE") == "Foo=Bar");
    }

    SECTION("Case insensitiveness for keys") {
        Tags tags;
        tags.add("TiTlE=Boop");
        REQUIRE(tags.get("tiTLE") == "Boop");
    }
}
