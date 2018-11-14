#include <opustags.h>

static const char* messages[] = {
	"OK",
	"Need to exit",
	"Bad command-line arguments",
	"Integer overflow",
	nullptr, /* standard error: call stderror */
	"End of file",
	"libogg error",
	"Invalid identification header, not an Opus stream",
	"Invalid comment header",
	"Bad magic number",
	"Overflowing magic number",
	"Overflowing vendor length",
	"Overflowing vendor data",
	"Overflowing comment count",
	"Overflowing comment length",
	"Overflowing comment data",
};

static_assert(sizeof(messages) / sizeof(*messages) == static_cast<size_t>(ot::status::sentinel));

const char* ot::error_message(ot::status code)
{
	if (code == ot::status::standard_error)
		return strerror(errno);
	if (code >= ot::status::sentinel)
		return nullptr;
	auto index = static_cast<size_t>(code);
	return messages[index];
}
