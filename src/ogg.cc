#include "ogg.h"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstring>
#include <endian.h>

using namespace opustags;

////////////////////////////////////////////////////////////////////////////////
// ogg::Stream

ogg::Stream::Stream(int streamno)
{
    state = ogg::BEGIN_OF_STREAM;
    type = ogg::UNKNOWN_STREAM;
    if (ogg_stream_init(&stream, streamno) != 0)
        throw std::runtime_error("ogg_stream_init failed");
}

ogg::Stream::~Stream()
{
    ogg_stream_clear(&stream);
}

void ogg::Stream::flush_packets()
{
    ogg_packet op;
    while (ogg_stream_packetout(&stream, &op) > 0);
}

bool ogg::Stream::page_in(ogg_page &og)
{
    if (state == ogg::RAW_READY)
        return true;
    flush_packets(); // otherwise packet_out keeps returning the same packet
    if (ogg_stream_pagein(&stream, &og) != 0)
        throw std::runtime_error("ogg_stream_pagein failed");

    if (state == ogg::BEGIN_OF_STREAM || state == ogg::HEADER_READY) {
        // We're expecting a header, so we parse it.
        return handle_page();
    } else {
        // We're past the first two headers.
        state = ogg::DATA_READY;
        return true;
    }
}

// Read the first packet of the page and parses it.
bool ogg::Stream::handle_page()
{
    ogg_packet op;
    int rc = ogg_stream_packetpeek(&stream, &op);
    if (rc < 0)
        throw std::runtime_error("ogg_stream_packetout failed");
    else if (rc == 0) // insufficient data
        return false; // asking for a new page
    // We've read the first packet successfully.
    // The headers are supposed to contain only one packet, so this is enough
    // for us. Still, we could ensure there are no other packets.
    handle_packet(op);
    return true;
}

void ogg::Stream::handle_packet(const ogg_packet &op)
{
    if (state == ogg::BEGIN_OF_STREAM)
        parse_header(op);
    else if (state == ogg::HEADER_READY)
        parse_opustags(op);
    // else shrug
}

void ogg::Stream::parse_header(const ogg_packet &op)
{
    if (op.bytes >= 8 && memcmp(op.packet, "OpusHead", 8) == 0) {
        type = OPUS_STREAM;
        state = HEADER_READY;
    } else {
        type = UNKNOWN_STREAM;
        state = RAW_READY;
    }
}

// For reference:
// https://tools.ietf.org/html/draft-ietf-codec-oggopus-14#section-5.2
void ogg::Stream::parse_opustags(const ogg_packet &op)
{
    // This part is gonna be C-ish because I don't see how I'd do this in C++
    // without being inefficient, both in volume of code and performance.
    char *data = reinterpret_cast<char*>(op.packet);
    long remaining = op.bytes;
    if (remaining < 8 || memcmp(data, "OpusTags", 8) != 0)
        throw std::runtime_error("expected OpusTags header");
    data += 8;
    remaining -= 8;

    // Vendor string
    if (remaining < 4)
        throw std::runtime_error("no space for vendor string length");
    uint32_t vendor_length = le32toh(*reinterpret_cast<uint32_t*>(data));
    if (remaining - 4 < vendor_length)
        throw std::runtime_error("invalid vendor string length");
    tags.vendor = std::string(data + 4, vendor_length);
    data += 4 + vendor_length;
    remaining -= 4 + vendor_length;

    // User comments count
    if (remaining < 4)
        throw std::runtime_error("no space for user comment list length");
    long comment_count = le32toh(*reinterpret_cast<uint32_t*>(data));
    data += 4;
    remaining -= 4;

    // Actual comments
    // We iterate on a long type to prevent infinite looping when comment_count == UINT32_MAX.
    for (long i = 0; i < comment_count; i++) {
        if (remaining < 4)
            throw std::runtime_error("no space for user comment length");
        uint32_t comment_length = le32toh(*reinterpret_cast<uint32_t*>(data));
        if (remaining - 4 < comment_length)
            throw std::runtime_error("no space for comment contents");
        tags.add(parse_tag(std::string(data + 4, comment_length)));
        data += 4 + comment_length;
        remaining -= 4 + comment_length;
    }

    // Extra data to keep if the least significant bit of the first byte is 1
    if (remaining > 0 && (*data & 1) == 1 )
        tags.extra = std::string(data, remaining);

    state = TAGS_READY;
}

void ogg::Stream::downgrade()
{
    type = ogg::UNKNOWN_STREAM;
    if (state != ogg::BEGIN_OF_STREAM && state != ogg::END_OF_STREAM)
        state = RAW_READY;
}

////////////////////////////////////////////////////////////////////////////////
// ogg::Decoder

ogg::Decoder::Decoder(std::istream &in)
    : input(in)
{
    if (!in)
        throw std::runtime_error("invalid stream to decode");
    input.exceptions(std::ifstream::badbit);
    ogg_sync_init(&sync);
}

ogg::Decoder::~Decoder()
{
    ogg_sync_clear(&sync);
}

std::shared_ptr<ogg::Stream> ogg::Decoder::read_page()
{
    while (page_out()) {
        int streamno = ogg_page_serialno(&current_page);
        auto i = streams.find(streamno);
        if (i == streams.end()) {
            // we could check the page number to detect new streams (pageno = 0)
            auto s = std::make_shared<Stream>(streamno);
            i = streams.emplace(streamno, s).first;
        }
        if (i->second->page_in(current_page))
            return i->second;
    }
    return nullptr; // end of stream
}

// Read the next page and return true on success, false on end of stream.
bool ogg::Decoder::page_out()
{
    int rc;
    for (;;) {
        rc = ogg_sync_pageout(&sync, &current_page);
        if (rc < 0) {
            throw std::runtime_error("ogg_sync_pageout failed");
        } else if (rc == 1) {
            break; // page complete
        } else if (!buff()) {
            // more data required but end of file reached
            // TODO check sync.unsynced flag in case we've got an incomplete page
            return false;
        }
    }
    return true;
}

// Read data from the stream into the sync's buffer.
bool ogg::Decoder::buff()
{
    if (input.eof())
        return false;
    char *buf = ogg_sync_buffer(&sync, 65536);
    if (buf == nullptr)
        throw std::runtime_error("ogg_sync_buffer failed");
    input.read(buf, 65536);
    ogg_sync_wrote(&sync, input.gcount());
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// ogg::Encoder

ogg::Encoder::Encoder(std::ostream &out)
    : output(out)
{
    if (!output)
        throw std::runtime_error("invalid stream to decode");
    output.exceptions(std::ifstream::badbit);
}

ogg::Stream& ogg::Encoder::get_stream(int streamno)
{
    auto i = streams.find(streamno);
    if (i == streams.end()) {
        auto s = std::make_shared<Stream>(streamno);
        i = streams.emplace(streamno, s).first;
    }
    return *(i->second);
}

void ogg::Encoder::forward(ogg::Stream &in)
{
    ogg::Stream *out = &get_stream(in.stream.serialno);
    forward_stream(in, *out);
    flush_stream(*out);
}

void ogg::Encoder::forward_stream(ogg::Stream &in, ogg::Stream &out)
{
    int rc;
    ogg_packet op;
    for (;;) {
        rc = ogg_stream_packetout(&in.stream, &op);
        if (rc < 0) {
            throw std::runtime_error("ogg_stream_packetout failed");
        } else if (rc == 0) {
            break;
        } else {
            if (ogg_stream_packetin(&out.stream, &op) != 0)
                throw std::runtime_error("ogg_stream_packetin failed");
        }
    }
}

void ogg::Encoder::flush_stream(ogg::Stream &out)
{
    ogg_page og;
    if (ogg_stream_flush(&out.stream, &og))
            write_raw_page(og);
}

void ogg::Encoder::write_raw_page(const ogg_page &og)
{
    output.write(reinterpret_cast<const char*>(og.header), og.header_len);
    output.write(reinterpret_cast<const char*>(og.body), og.body_len);
}

void ogg::Encoder::write_tags(int streamno, const Tags &tags)
{
    ogg_packet op;
    op.b_o_s = 0;
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = 1; // checked on a file from ffmpeg

    std::string data = render_opustags(tags);
    op.bytes = data.size();
    op.packet = reinterpret_cast<unsigned char*>(const_cast<char*>(data.data()));

    std::shared_ptr<ogg::Stream> s = streams.at(streamno); // assume it exists
    if (ogg_stream_packetin(&s->stream, &op) != 0)
        throw std::runtime_error("ogg_stream_packetin failed");
    flush_stream(*s);
}

std::string ogg::Encoder::render_opustags(const Tags &tags)
{
    std::stringbuf s;
    uint32_t length;
    s.sputn("OpusTags", 8);
    length = htole32(tags.vendor.size());
    s.sputn(reinterpret_cast<char*>(&length), 4);
    s.sputn(tags.vendor.data(), tags.vendor.size());

    auto assocs = tags.get_all();
    length = htole32(assocs.size());
    s.sputn(reinterpret_cast<char*>(&length), 4);

    for (const auto assoc : assocs) {
        length = htole32(assoc.key.size() + 1 + assoc.value.size());
        s.sputn(reinterpret_cast<char*>(&length), 4);
        s.sputn(assoc.key.data(), assoc.key.size());
        s.sputc('=');
        s.sputn(assoc.value.data(), assoc.value.size());
    }

    s.sputn(tags.extra.data(), tags.extra.size());
    return s.str();
}
