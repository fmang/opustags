#include <opustags.h>
#include "tap.h"

static void check_message()
{
	if (strcmp(ot::error_message(ot::status::ok), "OK") != 0)
		throw failure("unexpected message for code ok");
	if (strcmp(ot::error_message(ot::status::overflowing_comment_data), "Overflowing comment data") != 0)
		throw failure("unexpected message for overflowing_comment_data");
}

static void check_sentinel()
{
	if (ot::error_message(ot::status::sentinel) != nullptr)
		throw failure("the sentinel should not have a message");
	auto too_far = static_cast<ot::status>(static_cast<int>(ot::status::sentinel) + 1);
	if (ot::error_message(too_far) != nullptr)
		throw failure("an invalid status should not have a message");
}

int main()
{
	std::cout << "1..2\n";
	run(check_message, "check a few error messages");
	run(check_sentinel, "ensure the sentinel is respected");
	return 0;
}
