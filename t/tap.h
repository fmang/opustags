/**
 * \file t/tap.h
 *
 * \brief
 * Helpers for following the Test Anything Protocol.
 */

#pragma once

#include <exception>
#include <iostream>

class failure : public std::runtime_error {
public:
	failure(const char *message) : std::runtime_error(message) {}
};

template <typename F>
static void run(F test, const char *name)
{
	bool ok = false;
	try {
		test();
		ok = true;
	} catch (failure& e) {
		std::cout << "# " << e.what() << "\n";
	}
	std::cout << (ok ? "ok" : "not ok") << " - " << name << "\n";
}
