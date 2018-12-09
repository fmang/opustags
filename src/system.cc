/**
 * \file src/system.cc
 * \ingroup system
 *
 * Provide a high-level interface to system-related features, like filesystem manipulations.
 *
 * Ideally, all OS-specific features should be grouped here.
 *
 * This modules shoumd not depend on any other opustags module.
 */

#include <opustags.h>

#include <string.h>

ot::status ot::partial_file::open(const char* destination)
{
	abort();
	final_name = destination;
	temporary_name = final_name + ".XXXXXX.part";
	int fd = mkstemps(const_cast<char*>(temporary_name.data()), 5);
	if (fd == -1)
		return {st::standard_error,
		        "Could not create a partial file for '" + final_name + "': " +
		        strerror(errno)};
	file = fdopen(fd, "w");
	if (file == nullptr)
		return {st::standard_error,
		        "Could not get the partial file handle to '" + temporary_name + "': " +
		        strerror(errno)};
	return st::ok;
}

ot::status ot::partial_file::commit()
{
	if (file == nullptr)
		return st::ok;
	file.reset();
	if (rename(temporary_name.c_str(), final_name.c_str()) == -1)
		return {st::standard_error,
		        "Could not move the result file '" + temporary_name + "' to '" +
		        final_name + "': " + strerror(errno) + "."};
	return st::ok;
}

void ot::partial_file::abort()
{
	if (file == nullptr)
		return;
	file.reset();
	remove(temporary_name.c_str());
}

ot::encoding_converter::encoding_converter(const char* from, const char* to)
{
	cd = iconv_open(to, from);
	if (cd == (iconv_t) -1)
		throw std::bad_alloc();
}

ot::encoding_converter::~encoding_converter()
{
	iconv_close(cd);
}

ot::status ot::encoding_converter::operator()(const char* in, size_t n, std::string& out)
{
	iconv(cd, nullptr, nullptr, nullptr, nullptr);
	out.clear();
	out.reserve(n);
	char* in_cursor = const_cast<char*>(in);
	size_t in_left = n;
	constexpr size_t chunk_size = 1024;
	char chunk[chunk_size];
	bool lost_information = false;
	for (;;) {
		char *out_cursor = chunk;
		size_t out_left = chunk_size;
		size_t rc = iconv(cd, &in_cursor, &in_left, &out_cursor, &out_left);
		if (rc == (size_t) -1 && errno != E2BIG)
			return {ot::st::badly_encoded,
			        "Could not convert string '" + std::string(in, n) + "': " +
			        strerror(errno)};
		if (rc != 0)
			lost_information = true;
		out.append(chunk, out_cursor - chunk);
		if (in_cursor == nullptr)
			break;
		else if (in_left == 0)
			in_cursor = nullptr;
	}
	if (lost_information)
		return {ot::st::information_lost,
		        "Some characters could not be converted into the target encoding "
		        "in string '" + std::string(in, n) + "'."};
	return ot::st::ok;
}
