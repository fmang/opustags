#include "ogg.h"
#include "catch.h"

#include <fstream>

using namespace opustags;

SCENARIO("decoding a single-stream file", "[ogg]")
{

    GIVEN("an ogg::Decoder") {
        std::ifstream src("../tests/samples/mystery.ogg");
        ogg::Decoder dec(src);

        WHEN("reading the first page") {
            std::shared_ptr<ogg::Stream> s = dec.read_page();
            THEN("the Opus header is ready") {
                REQUIRE(s != nullptr);
                REQUIRE(s->state == ogg::HEADER_READY);
                REQUIRE(s->type == ogg::OPUS_STREAM);
            }
        }
    }

}
