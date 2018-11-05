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
	if (tags.vendor_length != 20)
		throw failure("the vendor string length is invalid");
	if (memcmp(tags.vendor_string, "opustags test packet", 20) != 0)
		throw failure("the vendor string is invalid");
	if (tags.count != 2)
		throw failure("bad number of comments");
	if (tags.lengths[0] != 9 || tags.lengths[1] != 10)
		throw failure("bad comment lengths");
	if (memcmp(tags.comment[0], "TITLE=Foo", tags.lengths[0]) != 0)
		throw failure("bad title comment");
	if (memcmp(tags.comment[1], "ARTIST=Bar", tags.lengths[1]) != 0)
		throw failure("bad artist comment");
	ot::free_tags(&tags);
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
	ot::free_tags(&tags);
	free(packet.packet);
	return true;
}

int main()
{
	std::cout << "1..2\n";
	run(parse_standard, "parse a standard OpusTags packet");
	run(recode_standard, "recode a standard OpusTags packet");
	return 0;
}
