#include "ogg.h"
#include "actions.h"
#include "catch.h"

#include <fstream>
#include <cstring>

#include "tags_handlers/insertion_tags_handler.h"
#include "tags_handlers/stream_tags_handler.h"

using namespace opustags;

static bool same_streams(ogg::Decoder &a, ogg::Decoder &b)
{
    std::shared_ptr<ogg::Stream> sa, sb;
    ogg_packet pa, pb;
    int ra, rb;

    for (;;) {
        sa = a.read_page();
        sb = b.read_page();
        if (!sa && !sb)
            break; // both reached the end at the same time
        if (!sa || !sb)
            return false; // one stream is shorter than the other
        if (sa->stream.serialno != sb->stream.serialno)
            return false;
        for (;;) {
            ra = ogg_stream_packetout(&sa->stream, &pa);
            rb = ogg_stream_packetout(&sb->stream, &pb);
            if (ra != rb)
                return false;
            else if (ra == 0)
                break;
            else if (ra < 0)
                throw std::runtime_error("ogg_stream_packetout failed");
            // otherwise we got a valid packet on both sides
            if (pa.bytes != pb.bytes || pa.b_o_s != pb.b_o_s || pa.e_o_s != pb.e_o_s)
                return false;
            if (pa.granulepos != pb.granulepos || pa.packetno != pb.packetno)
                return false;
            if (memcmp(pa.packet, pb.packet, pa.bytes) != 0)
                return false;
        }
    }
    return true;
}

static bool same_files(std::istream &a, std::istream &b)
{
    static const size_t block = 1024;
    char ba[block], bb[block];
    while (a && b) {
        a.read(ba, block);
        b.read(bb, block);
        if (a.gcount() != b.gcount())
            return false;
        if (memcmp(ba, bb, a.gcount()) != 0)
            return false;
    }
    if (a || b)
        return false; // one of them is shorter
    return true;
}

TEST_CASE("editing an unknown stream should do nothing", "[actions]")
{
    std::ifstream in("../tests/samples/beep.ogg");
    std::stringstream out;
    ogg::Decoder dec(in);
    ogg::Encoder enc(out);

    StreamTagsHandler editor(StreamTagsHandler::ALL_STREAMS);
    edit_tags(dec, enc, editor);

    in.clear();
    in.seekg(0, in.beg);
    out.clear();
    out.seekg(0, out.beg);
    REQUIRE(same_files(in, out));
}

TEST_CASE("fake editing of an Opus stream should preserve the stream", "[actions]")
{
    std::ifstream in("../tests/samples/mystery-beep.ogg");
    std::stringstream out;
    ogg::Decoder dec(in);
    ogg::Encoder enc(out);

    StreamTagsHandler editor(StreamTagsHandler::ALL_STREAMS);
    edit_tags(dec, enc, editor);

    in.clear();
    in.seekg(0, in.beg);
    out.clear();
    out.seekg(0, out.beg);
    REQUIRE(same_files(in, out));
}

TEST_CASE("editing a specific stream", "[actions]")
{
    std::ifstream in("../tests/samples/mystery-beep.ogg");
    std::stringstream out;

    {
        ogg::Decoder dec(in);
        ogg::Encoder enc(out);
        InsertionTagsHandler editor(2, "pwnd", "yes");
        edit_tags(dec, enc, editor);
    }

    in.clear();
    in.seekg(0, in.beg);
    out.clear();
    out.seekg(0, out.beg);

    {
        ogg::Decoder a(in);
        ogg::Decoder b(out);
        std::shared_ptr<ogg::Stream> s2[6];
        for (int i = 0; i < 6; i++) {
            // get the headers
            a.read_page();
            s2[i] = b.read_page();
        }

        REQUIRE(s2[0]->type == ogg::OPUS_STREAM);
        REQUIRE(s2[1]->type == ogg::UNKNOWN_STREAM);
        REQUIRE(s2[2]->type == ogg::OPUS_STREAM);
        REQUIRE(!s2[0]->tags.contains("pwnd"));
        REQUIRE(s2[2]->tags.get("pwnd") == "yes");

        REQUIRE(same_streams(a, b));
    }
}

class GetLanguages : public StreamTagsHandler {
public:
    GetLanguages() : StreamTagsHandler(StreamTagsHandler::ALL_STREAMS) {}
    std::vector<std::string> languages;
protected:
    void list_impl(const Tags &tags) {
        languages.push_back(tags.get("LANGUAGE"));
    }
};

TEST_CASE("listing tags", "[actions]")
{
    std::ifstream in("../tests/samples/mystery-beep.ogg");
    ogg::Decoder dec(in);
    GetLanguages lister;
    list_tags(dec, lister);

    REQUIRE(lister.languages.size() == 2);
    REQUIRE(lister.languages[0] == "und");
    REQUIRE(lister.languages[1] == "und");
}
