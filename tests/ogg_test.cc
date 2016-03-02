#include "ogg.h"
#include "catch.h"

#include <fstream>

using namespace opustags;

TEST_CASE("decoding a single-stream file", "[ogg]")
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
