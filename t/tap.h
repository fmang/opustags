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

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <sys/stat.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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

#ifdef _WIN32
/**
 * Windows has no fmemopen, so copy the buffer into a temporary file instead. Unlike with
 * fmemopen, writes don't reach the buffer.
 */
FILE* fmemopen(void* buf, size_t size, const char* mode)
{
	char dir[MAX_PATH + 1];
	char path[MAX_PATH + 1];
	if (GetTempPathA(sizeof(dir), dir) == 0 || GetTempFileNameA(dir, "ot", 0, path) == 0)
		return nullptr;
	int fd;
	if (_sopen_s(&fd, path, _O_RDWR | _O_BINARY | _O_TEMPORARY, _SH_DENYNO,
	             _S_IREAD | _S_IWRITE) != 0) {
		DeleteFileA(path);
		return nullptr;
	}
	FILE* file = _fdopen(fd, "w+b");
	if (file == nullptr) {
		_close(fd);
		return nullptr;
	}
	if (mode[0] == 'r') {
		if (fwrite(buf, 1, size, file) != size) {
			fclose(file);
			return nullptr;
		}
		rewind(file);
	}
	return file;
}
#endif

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

template <typename T, typename U>
void opaque_is(const T& got, const U& expected, const char* name)
{
	if (got != expected)
		throw failure(name);
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
