#include "ogg.h"

#include <stdexcept>
#include <fstream>

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
    // set type
    // set state
}

void ogg::Stream::parse_tags(const ogg_packet &op)
{
    // TODO
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

ogg::Decoder::Decoder(std::istream *in)
    : input(in)
{
    input->exceptions(std::ifstream::badbit);
    ogg_sync_init(&sync);
}

ogg::Decoder::~Decoder()
{
    ogg_sync_clear(&sync);
}

ogg::Stream* ogg::Decoder::read_page()
{
    while (page_out()) {
        int streamno = ogg_page_serialno(&current_page);
        auto i = streams.find(streamno);
        if (i == streams.end()) {
            // we could check the page number to detect new streams (pageno = 0)
            i = streams.emplace(streamno, Stream(streamno)).first;
        }
        if (i->second.page_in(current_page))
            return &(i->second);
    }
    return NULL; // end of stream
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
    if (input->eof())
        return false;
    char *buf = ogg_sync_buffer(&sync, 65536);
    if (buf == NULL)
        throw std::runtime_error("ogg_sync_buffer failed");
    input->read(buf, 65536);
    ogg_sync_wrote(&sync, input->gcount());
    return true;
}
