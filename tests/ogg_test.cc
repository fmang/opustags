#include "ogg.h"
#include "catch.h"

#include <fstream>

using namespace opustags;

TEST_CASE("decoding a single-stream Ogg Opus file", "[ogg]")
{
    std::ifstream src("../tests/samples/mystery.ogg");
    ogg::Decoder dec(src);

    std::shared_ptr<ogg::Stream> s = dec.read_page();
    REQUIRE(s != nullptr);
    REQUIRE(s->state == ogg::HEADER_READY);
    REQUIRE(s->type == ogg::OPUS_STREAM);

    std::shared_ptr<ogg::Stream> s2 = dec.read_page();
    REQUIRE(s2 == s);
    REQUIRE(s->state == ogg::TAGS_READY);
    REQUIRE(s->type == ogg::OPUS_STREAM);
    REQUIRE(s->tags.get("encoder") == "Lavc57.24.102 libopus");
}

TEST_CASE("decoding garbage", "[ogg]")
{
    std::ifstream src("Makefile");
    ogg::Decoder dec(src);
    REQUIRE_THROWS(dec.read_page());
}

TEST_CASE("decoding an Ogg Vorbis file", "[ogg]")
{
    std::ifstream src("../tests/samples/beep.ogg");
    ogg::Decoder dec(src);

    std::shared_ptr<ogg::Stream> s = dec.read_page();
    REQUIRE(s != nullptr);
    REQUIRE(s->state == ogg::RAW_READY);
    REQUIRE(s->type == ogg::UNKNOWN_STREAM);
}

// TODO decoding a multi-stream file

// Encoding is trickier, and might as well be done in actions_test.cc, given
// opustags::edit_tags covers all of Encoder's regular code.
