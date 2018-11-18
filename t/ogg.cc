#include <opustags.h>
#include "tap.h"

static void check_ref_ogg()
{
	FILE* input = fopen("gobble.opus", "r");
	if (input == nullptr)
		throw failure("could not open gobble.opus");
	ot::ogg_reader reader(input);

	ot::status rc = reader.read_page();
	if (rc != ot::status::ok)
		throw failure("could not read the first page");
	rc = reader.read_packet();
	if (rc != ot::status::ok)
		throw failure("could not read the first packet");
	if (reader.packet.bytes != 19)
		throw failure("unexpected length for the first packet");
	rc = reader.read_packet();
	if (rc != ot::status::end_of_page)
		throw failure("got an unexpected second packet on the first page");

	rc = reader.read_page();
	if (rc != ot::status::ok)
		throw failure("could not read the second page");
	rc = reader.read_packet();
	if (rc != ot::status::ok)
		throw failure("could not read the second packet");
	if (reader.packet.bytes != 62)
		throw failure("unexpected length for the first packet");
	rc = reader.read_packet();
	if (rc != ot::status::end_of_page)
		throw failure("got an unexpected second packet on the second page");

	while (!ogg_page_eos(&reader.page)) {
		rc = reader.read_page();
		if (rc != ot::status::ok)
			throw failure("failure reading a page");
	}
	rc = reader.read_page();
	if (rc != ot::status::end_of_stream)
		throw failure("did not correctly detect the end of stream");
}

int main(int argc, char **argv)
{
	std::cout << "1..1\n";
	run(check_ref_ogg, "check a reference ogg stream");
	return 0;
}
