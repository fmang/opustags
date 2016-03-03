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

TEST_CASE("decoding a multi-stream file", "[ogg]")
{
    std::ifstream src("../tests/samples/mystery-beep.ogg");
    ogg::Decoder dec(src);
    std::shared_ptr<ogg::Stream> first, second, third, s;

    s = dec.read_page();
    first = s;
    REQUIRE(s != nullptr);
    REQUIRE(s->state == ogg::HEADER_READY);
    REQUIRE(s->type == ogg::OPUS_STREAM);

    s = dec.read_page();
    second = s;
    REQUIRE(s != nullptr);
    REQUIRE(s != first);
    REQUIRE(s->state == ogg::RAW_READY);
    REQUIRE(s->type == ogg::UNKNOWN_STREAM);

    s = dec.read_page();
    third = s;
    REQUIRE(s != nullptr);
    REQUIRE(s != first);
    REQUIRE(s != second);
    REQUIRE(s->state == ogg::HEADER_READY);
    REQUIRE(s->type == ogg::OPUS_STREAM);

    s = dec.read_page();
    REQUIRE(s == first);
    REQUIRE(s->state == ogg::TAGS_READY);

    s = dec.read_page();
    REQUIRE(s == second);
    REQUIRE(s->state == ogg::RAW_READY);

    s = dec.read_page();
    REQUIRE(s == third);
    REQUIRE(s->state == ogg::TAGS_READY);
}

void craft_stream(std::ostream &out, const std::string &tags_data)
{
    ogg::Encoder enc(out);
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    ogg_stream_init(&os, 0);

    op.packet = reinterpret_cast<unsigned char*>(const_cast<char*>("OpusHead"));
    op.bytes = 8;
    op.b_o_s = 1;
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = 0;
    ogg_stream_packetin(&os, &op);
    ogg_stream_flush(&os, &og);
    enc.write_raw_page(og);

    op.packet = reinterpret_cast<unsigned char*>(const_cast<char*>(tags_data.data()));
    op.bytes = tags_data.size();
    op.b_o_s = 0;
    op.e_o_s = 1;
    op.granulepos = 0;
    op.packetno = 1;
    ogg_stream_packetin(&os, &op);
    ogg_stream_flush(&os, &og);
    enc.write_raw_page(og);

    ogg_stream_clear(&os);
}

const char *evil_tags =
    "OpusTags"
    "\x00\x00\x00\x00" "" /* vendor */
    "\x01\x00\x00\x00" /* one comment */
    "\xFA\x00\x00\x00" "TITLE=Evil"
    /* ^ should be \x0A as the length of the comment is 10 */
;

TEST_CASE("decoding a malicious Ogg Opus file", "[ogg]")
{
    std::stringstream buf;
    craft_stream(buf, std::string(evil_tags, 8 + 4 + 4 + 4 + 10));
    buf.seekg(0, buf.beg);
    ogg::Decoder dec(buf);

    std::shared_ptr<ogg::Stream> s = dec.read_page();
    REQUIRE(s != nullptr);
    REQUIRE(s->state == ogg::HEADER_READY);
    REQUIRE(s->type == ogg::OPUS_STREAM);

    REQUIRE_THROWS(dec.read_page());
}

TEST_CASE("decoding a bad stream", "[ogg]")
{
    std::ifstream in("uioprheuio");
    REQUIRE_THROWS(std::make_shared<ogg::Decoder>(in));
}

// Encoding is trickier, and might as well be done in actions_test.cc, given
// opustags::edit_tags covers all of Encoder's regular code.
