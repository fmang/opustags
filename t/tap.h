/**
 * \file t/tap.h
 *
 * \brief
 * Helpers for following the Test Anything Protocol.
 */

#pragma once

#include <exception>
#include <iostream>

struct failure : std::runtime_error {
	failure(const std::string& what) : std::runtime_error(what) {}
};

template <typename F>
static void run(F test, const char *name)
{
	bool ok = false;
	try {
		test();
		ok = true;
	} catch (failure& e) {
		std::cerr << "# " << e.what() << "\n";
	}
	std::cout << (ok ? "ok" : "not ok") << " - " << name << "\n";
}
