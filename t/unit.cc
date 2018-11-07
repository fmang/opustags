/**
 * \file t/unit.cc
 *
 * Entry point of the unit test suite.
 * Follows the TAP protocol.
 *
 */

#include <opustags.h>

#include <cstring>
#include <exception>
#include <iostream>

class failure : public std::runtime_error {
public:
	failure(const char *message) : std::runtime_error(message) {}
};

template <typename F>
static void run(F test, const char *name)
{
	bool result = false;
	try {
		result = test();
	} catch (failure& e) {
		std::cout << "# " << e.what() << "\n";
	}
	std::cout << (result ? "ok" : "not ok") << " - " << name << "\n";
}

static const char standard_OpusTags[] =
	"OpusTags"
	"\x14\x00\x00\x00" "opustags test packet"
	"\x02\x00\x00\x00"
	"\x09\x00\x00\x00" "TITLE=Foo"
	"\x0a\x00\x00\x00" "ARTIST=Bar";

static bool parse_standard()
{
	ot::opus_tags tags;
	int rc = ot::parse_tags(standard_OpusTags, sizeof(standard_OpusTags) - 1, &tags);
	if (rc != 0)
		throw failure("ot::parse_tags did not return 0");
	if (tags.vendor != ot::string_view( "opustags test packet"))
		throw failure("bad vendor string");
	if (tags.comments.size() != 2)
		throw failure("bad number of comments");
	auto it = tags.comments.begin();
	if (*it != ot::string_view("TITLE=Foo"))
		throw failure("bad title");
	++it;
	if (*it != ot::string_view("ARTIST=Bar"))
		throw failure("bad artist");
	if (tags.extra_data.size != 0)
		throw failure("found mysterious padding data");
	return true;
}

static bool recode_standard()
{
	ot::opus_tags tags;
	int rc = ot::parse_tags(standard_OpusTags, sizeof(standard_OpusTags) - 1, &tags);
	if (rc != 0)
		throw failure("ot::parse_tags did not return 0");
	ogg_packet packet;
	rc = ot::render_tags(&tags, &packet);
	if (rc != 0)
		throw failure("ot::render_tags did not return 0");
	if (packet.b_o_s != 0)
		throw failure("b_o_s should not be set");
	if (packet.e_o_s != 0)
		throw failure("e_o_s should not be set");
	if (packet.granulepos != 0)
		throw failure("granule_post should be 0");
	if (packet.packetno != 1)
		throw failure("packetno should be 1");
	if (packet.bytes != sizeof(standard_OpusTags) - 1)
		throw failure("the packet is not the right size");
	if (memcmp(packet.packet, standard_OpusTags, packet.bytes) != 0)
		throw failure("the rendered packet is not what we expected");
	free(packet.packet);
	return true;
}

static bool recode_padding()
{
	ot::opus_tags tags;
	std::string padded_OpusTags(standard_OpusTags, sizeof(standard_OpusTags));
	// ^ note: padded_OpusTags ends with a null byte here
	padded_OpusTags += "hello";
	int rc = ot::parse_tags(padded_OpusTags.data(), padded_OpusTags.size(), &tags);
	if (rc != 0)
		throw failure("ot::parse_tags did not return 0");
	if (tags.extra_data.size != 6)
		throw failure("unexpected amount of extra bytes");
	if (memcmp(tags.extra_data.data, "\0hello", 6) != 0)
		throw failure("corrupted extra data");
	// recode the packet and ensure it's exactly the same
	ogg_packet packet;
	rc = ot::render_tags(&tags, &packet);
	if (packet.bytes < padded_OpusTags.size())
		throw failure("the packet was truncated");
	if (packet.bytes > padded_OpusTags.size())
		throw failure("the packet got too big");
	if (memcmp(packet.packet, padded_OpusTags.data(), packet.bytes) != 0)
		throw failure("the rendered packet is not what we expected");
	free(packet.packet);
	return true;
}

int main()
{
	std::cout << "1..3\n";
	run(parse_standard, "parse a standard OpusTags packet");
	run(recode_standard, "recode a standard OpusTags packet");
	run(recode_padding, "recode a OpusTags packet with padding");
	return 0;
}
