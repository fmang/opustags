#include <opustags.h>
#include "tap.h"

const char *user_comments = R"raw(
TITLE=a b c

ARTIST=X
Artist=Y
)raw";

void check_read_comments()
{
	auto input = ot::make_file(fmemopen, const_cast<char*>(user_comments), strlen(user_comments), "r");
	auto comments = ot::read_comments(input.get());
	auto&& expected = {"TITLE=a b c", "ARTIST=X", "Artist=Y"};
	if (!std::equal(comments.begin(), comments.end(), expected.begin(), expected.end()))
		throw failure("parsed user comments did not match expectations");
}

int main(int argc, char **argv)
{
	std::cout << "1..1\n";
	run(check_read_comments, "check tags parsing");
	return 0;
}
