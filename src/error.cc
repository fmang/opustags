#include <opustags.h>

static const char* messages[] = {
	"OK",
	"Need to exit",
	"Bad command-line arguments",
	"Integer overflow",
	"Standard error",
	"End of file",
	"libogg error",
	"Bad magic number",
	"Overflowing magic number",
	"Overflowing vendor length",
	"Overflowing vendor data",
	"Overflowing comment count",
	"Overflowing comment length",
	"Overflowing comment data",
};

const char* ot::error_message(ot::status code)
{
if (code >= ot::status::sentinel)
	return nullptr;
auto index = static_cast<size_t>(code);
return messages[index];
}
