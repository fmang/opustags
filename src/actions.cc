#include "ogg.h"

// Here we're going to illustrate how to use the ogg module.

using namespace opustags;

enum StreamSelection {
	ALL_STREAMS = -1,
	FIRST_STREAM = -2,
};

void list_tags(ogg::Decoder *reader, long select)
{
	ogg::Stream *s;
	while ((s = reader->read_page()) != NULL) {
		if (s->state == ogg::TAGS_READY && (select == ALL_STREAMS || select == FIRST_STREAM || s->stream.serialno == select)) {
			// print_tags(s->tags);
			if (select != ALL_STREAMS)
				break;
		}
	}
}

void delete_tags(ogg::Decoder *reader, opustags::ogg::Encoder *writer, long select)
{
	ogg::Stream *s;
	while ((s = reader->read_page()) != NULL) {
		switch (s->state) {
		case ogg::HEADER_READY:
			// TODO what if select == FIRST_STREAM?
			if (select != ALL_STREAMS && s->stream.serialno != select)
				// act like nothing happened and copy this "unknown" stream identically
				s->type = ogg::UNKNOWN_STREAM;
			// fall through
		case ogg::RAW_READY:
			writer->write_raw_page(reader->current_page);
			break;
		case ogg::TAGS_READY:
			// only streams selected at HEADER_READY reach this point
			writer->write_tags(s->stream.serialno, ogg::Tags());
			break;
		case ogg::DATA_READY:
			writer->write_page(reader->current_page);
			break;
		default:
			;
		}
	}
}

// To write edit_tags, we're gonna need something like the options structure in
// order to know what to do exactly.

// The command-line interface has yet to be done, but it'd be nice it let the
// user edit several streams at once, like :
// $ opustags --stream 1 --set TITLE=Foo --stream 2 --set TITLE=Bar
