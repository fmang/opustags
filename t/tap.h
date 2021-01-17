/**
 * \file t/tap.h
 *
 * \brief
 * Helpers for following the Test Anything Protocol.
 *
 * Its interface mimics Test::More from Perl:
 * https://perldoc.perl.org/Test/More.html
 *
 * Unlike Test::More, a test failure raises an exception and aborts the whole subtest.
 */

#pragma once

#include <exception>
#include <iostream>

inline namespace tap {

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
		std::cerr << "# fail: " << e.what() << "\n";
	} catch (const ot::status &rc) {
		std::cerr << "# unexpected error: " << rc.message << "\n";
	}
	std::cout << (ok ? "ok" : "not ok") << " - " << name << "\n";
}

void plan(int tests)
{
	std::cout << "1.." << tests << "\n";
}

template <typename T, typename U>
void is(const T& got, const U& expected, const char* name)
{
	if (got != expected) {
		std::cerr << "# got: " << got << "\n"
		             "# expected: " << expected << "\n";
		throw failure(name);
	}
}

template <>
void is(const ot::status& got, const ot::st& expected, const char* name)
{
	if (got.code != expected) {
		if (got.code == ot::st::ok)
			std::cerr << "# unexpected success\n";
		else
			std::cerr << "# unexpected error: " << got.message << "\n";
		throw failure(name);
	}
}

}
