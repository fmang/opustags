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

#include <errno.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

ot::byte_string operator""_bs(const char* data, size_t size)
{
	return ot::byte_string(reinterpret_cast<const uint8_t*>(data), size);
}

ot::byte_string_view operator""_bsv(const char* data, size_t size)
{
	return ot::byte_string_view(reinterpret_cast<const uint8_t*>(data), size);
}

void ot::partial_file::open(const char* destination)
{
	final_name = destination;
	temporary_name = final_name + ".XXXXXX.part";
	int fd = mkstemps(const_cast<char*>(temporary_name.data()), 5);
	if (fd == -1)
		throw status {st::standard_error,
		              "Could not create a partial file for '" + final_name + "': " +
		              strerror(errno)};
	file = fdopen(fd, "w");
	if (file == nullptr)
		throw status {st::standard_error,
		              "Could not get the partial file handle to '" + temporary_name + "': " +
		              strerror(errno)};
}

static mode_t get_umask()
{
	// libc doesn’t seem to provide a way to get umask without changing it, so we need this workaround.
	// https://www.gnu.org/software/libc/manual/html_node/Setting-Permissions.html
	mode_t mask = umask(0);
	umask(mask);
	return mask;
}

/**
 * Try reproducing the file permissions of file `source` onto file `dest`. If
 * this fails for whatever reason, print a warning and leave the current
 * permissions. When the source doesn’t exist, use the default file creation
 * permissions according to umask.
 */
static void copy_permissions(const char* source, const char* dest)
{
	mode_t target_mode;
	struct stat source_stat;
	if (stat(source, &source_stat) == 0) {
		// We could technically preserve a bit more than that but who
		// would ever need S_ISUID and friends on an Opus file?
		target_mode = source_stat.st_mode & 0777;
	} else if (errno == ENOENT) {
		target_mode = 0666 & ~get_umask();
	} else {
		fprintf(stderr, "warning: Could not read mode of %s: %s\n", source, strerror(errno));
		return;
	}
	if (chmod(dest, target_mode) == -1)
		fprintf(stderr, "warning: Could not set mode of %s: %s\n", dest, strerror(errno));
}

void ot::partial_file::commit()
{
	if (file == nullptr)
		return;
	file.reset();
	copy_permissions(final_name.c_str(), temporary_name.c_str());
	if (rename(temporary_name.c_str(), final_name.c_str()) == -1)
		throw status {st::standard_error,
		              "Could not move the result file '" + temporary_name + "' to '" +
		              final_name + "': " + strerror(errno) + "."};
}

void ot::partial_file::abort()
{
	if (file == nullptr)
		return;
	file.reset();
	remove(temporary_name.c_str());
}

/**
 * Determine the file size, in bytes, of the given file. Return -1 on for streams.
 */
static long get_file_size(FILE* f)
{
	if (fseek(f, 0L, SEEK_END) != 0) {
		clearerr(f); // Recover.
		return -1;
	}
	long file_size = ftell(f);
	rewind(f);
	return file_size;
}

ot::byte_string ot::slurp_binary_file(const char* filename)
{
	file f = strcmp(filename, "-") == 0 ? freopen(nullptr, "rb", stdin)
	                                    : fopen(filename, "rb");
	if (f == nullptr)
		throw status { st::standard_error,
		               "Could not open '"s + filename + "': " + strerror(errno) + "." };

	byte_string content;
	long file_size = get_file_size(f.get());
	if (file_size == -1) {
		// Read the input stream block by block and resize the output byte string as needed.
		uint8_t buffer[4096];
		while (!feof(f.get())) {
			size_t read_len = fread(buffer, 1, sizeof(buffer), f.get());
			content.append(buffer, read_len);
			if (ferror(f.get()))
				throw status { st::standard_error,
					       "Could not read '"s + filename + "': " + strerror(errno) + "." };
		}
	} else {
		// Lucky! We know the file size, so let’s slurp it at once.
		content.resize(file_size);
		if (fread(content.data(), 1, file_size, f.get()) < file_size)
			throw status { st::standard_error,
				       "Could not read '"s + filename + "': " + strerror(errno) + "." };
	}

	return content;
}

/** C++ wrapper for iconv. */
class encoding_converter {
public:
	/**
	 * Allocate the iconv conversion state, initializing the given source and destination
	 * character encodings. If it's okay to have some information lost, make sure `to` ends with
	 * "//TRANSLIT", otherwise the conversion will fail when a character cannot be represented
	 * in the target encoding. See the documentation of iconv_open for details.
	 */
	encoding_converter(const char* from, const char* to);
	~encoding_converter();
	/**
	 * Convert text using iconv. If the input sequence is invalid, return #st::badly_encoded and
	 * abort the processing, leaving out in an undefined state.
	 */
	template<class InChar, class OutChar>
	std::basic_string<OutChar> convert(std::basic_string_view<InChar>);
private:
	iconv_t cd; /**< conversion descriptor */
};

encoding_converter::encoding_converter(const char* from, const char* to)
{
	cd = iconv_open(to, from);
	if (cd == (iconv_t) -1)
		throw std::bad_alloc();
}

encoding_converter::~encoding_converter()
{
	iconv_close(cd);
}

template<class InChar, class OutChar>
std::basic_string<OutChar> encoding_converter::convert(std::basic_string_view<InChar> in)
{
	iconv(cd, nullptr, nullptr, nullptr, nullptr);
	std::basic_string<OutChar> out;
	out.reserve(in.size());
	const char* in_data = reinterpret_cast<const char*>(in.data());
	char* in_cursor = const_cast<char*>(in_data);
	size_t in_left = in.size();
	constexpr size_t chunk_size = 1024;
	char chunk[chunk_size];
	for (;;) {
		char *out_cursor = chunk;
		size_t out_left = chunk_size;
		size_t rc = iconv(cd, &in_cursor, &in_left, &out_cursor, &out_left);

		if (rc == (size_t) -1 && errno == E2BIG) {
			// Loop normally.
		} else if (rc == (size_t) -1) {
			throw ot::status {ot::st::badly_encoded, strerror(errno) + "."s};
		} else if (rc != 0) {
			throw ot::status {ot::st::badly_encoded,
			                 "Some characters could not be converted into the target encoding."};
		}

		out.append(reinterpret_cast<OutChar*>(chunk), out_cursor - chunk);
		if (in_cursor == nullptr)
			break;
		else if (in_left == 0)
			in_cursor = nullptr;
	}
	return out;
}

std::u8string ot::encode_utf8(std::string_view in)
{
	static encoding_converter to_utf8_cvt("", "UTF-8");
	return to_utf8_cvt.convert<char, char8_t>(in);
}

std::string ot::decode_utf8(std::u8string_view in)
{
	static encoding_converter from_utf8_cvt("UTF-8", "");
	return from_utf8_cvt.convert<char8_t, char>(in);
}

std::string ot::shell_escape(std::string_view word)
{
	std::string escaped_word;
	// Pre-allocate the result, assuming most of the time enclosing it in single quotes is enough.
	escaped_word.reserve(2 + word.size());

	escaped_word += '\'';
	for (char c : word) {
		if (c == '\'')
			escaped_word += "'\\''";
		else if (c == '!')
			escaped_word += "'\\!'";
		else
			escaped_word += c;
	}
	escaped_word += '\'';

	return escaped_word;
}

void ot::run_editor(std::string_view editor, std::string_view path)
{
	std::string command = std::string(editor) + " " + shell_escape(path);
	int status = system(command.c_str());

	if (status == -1)
		throw ot::status {st::standard_error, "waitpid error: "s + strerror(errno)};
	else if (!WIFEXITED(status))
		throw ot::status {st::child_process_failed,
		                 "Child process did not terminate normally: "s + strerror(errno)};
	else if (WEXITSTATUS(status) != 0)
		throw ot::status {st::child_process_failed,
		                  "Child process exited with " + std::to_string(WEXITSTATUS(status))};
}

timespec ot::get_file_timestamp(const char* path)
{
	timespec mtime;
	struct stat st;
	if (stat(path, &st) == -1)
		throw status {st::standard_error, path + ": stat error: "s + strerror(errno)};
#if defined(HAVE_STAT_ST_MTIM)
	mtime = st.st_mtim;
#elif defined(HAVE_STAT_ST_MTIMESPEC)
	mtime = st.st_mtimespec;
#else
	mtime.tv_sec = st.st_mtime;
	mtime.tv_nsec = st.st_mtimensec;
#endif
	return mtime;
}
