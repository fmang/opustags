#include "ogg.h"

#include <stdexcept>

using namespace opustags;

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

bool ogg::Stream::page_in(ogg_page &og)
{
    if (state == ogg::RAW_READY)
        return true;
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
    int rc = ogg_stream_packetout(&stream, &op);
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
        parse_tags(op);
    // else shrug
}
void ogg::Stream::parse_header(const ogg_packet &op)
{
    // TODO
}

void ogg::Stream::parse_tags(const ogg_packet &op)
{
    // TODO
}

void ogg::Stream::downgrade()
{
    type = ogg::UNKNOWN_STREAM;
    if (state != ogg::BEGIN_OF_STREAM && state != ogg::END_OF_STREAM)
        state = RAW_READY;
}
