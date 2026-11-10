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

#ifdef _WIN32
#define _CRT_RAND_S // Declare rand_s in <stdlib.h>.
#endif

#include <opustags.h>

#include <errno.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#ifdef _WIN32

#ifndef HAVE_GETDELIM
ssize_t getdelim(char** lineptr, size_t* n, int delim, FILE* stream)
{
	std::string line;
	int c;
	while ((c = getc(stream)) != EOF) {
		line += static_cast<char>(c);
		if (c == delim)
			break;
	}
	if (line.empty())
		return -1;
	if (*lineptr == nullptr || *n < line.size() + 1) {
		char* buffer = static_cast<char*>(realloc(*lineptr, line.size() + 1));
		if (buffer == nullptr)
			return -1;
		*lineptr = buffer;
		*n = line.size() + 1;
	}
	memcpy(*lineptr, line.c_str(), line.size() + 1);
	return static_cast<ssize_t>(line.size());
}
#endif

#ifndef HAVE_MKSTEMPS
/**
 * Unlike _mktemp, which only tries 26 names and ignores the suffix when checking whether a name
 * is taken, keep picking random names until the file can be created.
 */
int mkstemps(char* tmpl, int suffixlen)
{
	size_t len = strlen(tmpl);
	if (suffixlen < 0 || len < 6 + static_cast<size_t>(suffixlen)) {
		errno = EINVAL;
		return -1;
	}
	char* placeholder = tmpl + len - static_cast<size_t>(suffixlen) - 6;
	if (memcmp(placeholder, "XXXXXX", 6) != 0) {
		errno = EINVAL;
		return -1;
	}
	static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	for (int attempt = 0; attempt < 100; ++attempt) {
		for (int i = 0; i < 6; ++i) {
			unsigned int r;
			if (rand_s(&r) != 0) {
				errno = EIO;
				return -1;
			}
			placeholder[i] = chars[r % (sizeof(chars) - 1)];
		}
		// _O_NOINHERIT is the equivalent of O_CLOEXEC.
		int fd;
		errno_t rc = _sopen_s(&fd, tmpl, _O_CREAT | _O_EXCL | _O_RDWR | _O_BINARY | _O_NOINHERIT,
		                      _SH_DENYNO, _S_IREAD | _S_IWRITE);
		if (rc == 0)
			return fd;
		if (rc != EEXIST) {
			errno = rc;
			return -1;
		}
	}
	errno = EEXIST;
	return -1;
}
#endif

#endif /* _WIN32 */

void ot::close_file(FILE* file)
{
	fclose(file);
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
	file = fdopen(fd, "wb");
	if (file == nullptr)
		throw status {st::standard_error,
		              "Could not get the partial file handle to '" + temporary_name + "': " +
		              strerror(errno)};
}

#ifdef _WIN32
/** Describe an error code returned by GetLastError(). */
static std::string win32_error_message(DWORD code)
{
	char* text = nullptr;
	DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
	                           FORMAT_MESSAGE_IGNORE_INSERTS,
	                           nullptr, code, 0, reinterpret_cast<char*>(&text), 0, nullptr);
	std::string message = len ? std::string(text, len) : "Error " + std::to_string(code);
	LocalFree(text);
	// System messages end with a period and CRLF.
	while (!message.empty() && (message.back() == '\n' || message.back() == '\r' ||
	                            message.back() == '.'))
		message.pop_back();
	return message;
}
#endif

#ifndef _WIN32
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
#endif

void ot::partial_file::commit()
{
	if (file == nullptr)
		return;
	file.reset();
#ifdef _WIN32
	// The read-only attribute is the only part of the mode on Windows. MoveFileEx refuses to
	// replace a read-only file, so clear the attribute first and restore it on the new file.
	const char* name = final_name.c_str();
	DWORD attributes = GetFileAttributesA(name);
	bool read_only = attributes != INVALID_FILE_ATTRIBUTES &&
	                 (attributes & FILE_ATTRIBUTE_READONLY);
	DWORD writable = attributes & ~FILE_ATTRIBUTE_READONLY;
	if (read_only && !SetFileAttributesA(name, writable ? writable : FILE_ATTRIBUTE_NORMAL))
		throw status {st::standard_error,
		              "Could not clear the read-only attribute of '" + final_name + "': " +
		              win32_error_message(GetLastError()) + "."};
	if (!MoveFileExA(temporary_name.c_str(), name, MOVEFILE_REPLACE_EXISTING)) {
		DWORD error = GetLastError();
		if (read_only)
			SetFileAttributesA(name, attributes);
		throw status {st::standard_error,
		              "Could not move the result file '" + temporary_name + "' to '" +
		              final_name + "': " + win32_error_message(error) + "."};
	}
	if (read_only && !SetFileAttributesA(name, attributes))
		fprintf(stderr, "warning: Could not set the attributes of %s: %s\n", name,
		        win32_error_message(GetLastError()).c_str());
#else
	copy_permissions(final_name.c_str(), temporary_name.c_str());
	if (rename(temporary_name.c_str(), final_name.c_str()) == -1)
		throw status {st::standard_error,
		              "Could not move the result file '" + temporary_name + "' to '" +
		              final_name + "': " + strerror(errno) + "."};
#endif
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
#ifdef _WIN32
	// The Microsoft C runtime does not support freopen with a null path, and main already puts
	// stdin in binary mode.
	file f = strcmp(filename, "-") == 0 ? stdin : fopen(filename, "rb");
#else
	file f = strcmp(filename, "-") == 0 ? freopen(nullptr, "rb", stdin)
	                                    : fopen(filename, "rb");
#endif
	if (f == nullptr)
		throw status { st::standard_error,
		               "Could not open '"s + filename + "': " + strerror(errno) + "." };

	byte_string content;
	long file_size = get_file_size(f.get());
	if (file_size < 0) {
		// Read the input stream block by block and resize the output byte string as needed.
		char buffer[4096];
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
		if (fread(content.data(), 1, file_size, f.get()) < size_t(file_size))
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
#ifdef _WIN32
	// Quote the word so that the C runtime parses it back as is, following
	// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
	// Always quote it, because cmd.exe interprets characters like & | < > ^ outside of quotes.
	std::string escaped_word = "\"";
	size_t backslashes = 0;
	for (char c : word) {
		if (c == '\\') {
			++backslashes;
			continue;
		}
		// Backslashes are only special before a double quote.
		escaped_word.append(c == '"' ? 2 * backslashes + 1 : backslashes, '\\');
		escaped_word += c;
		backslashes = 0;
	}
	escaped_word.append(2 * backslashes, '\\');
	escaped_word += '"';
	return escaped_word;
#else
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
#endif
}

void ot::run_editor(std::string_view editor, std::string_view path)
{
#ifdef _WIN32
	// Pass an absolute path rather than using --, which not all Windows editors understand.
	std::string relative_path(path);
	std::unique_ptr<char, decltype(&free)> absolute_path(
		_fullpath(nullptr, relative_path.c_str(), 0), &free);
	if (absolute_path == nullptr)
		throw ot::status {st::standard_error,
		                  "Could not resolve '" + relative_path + "': " + strerror(errno)};
	// system() runs cmd /c, which removes the first and last quotes of the command line when it
	// contains more than two (see cmd /?), so wrap the command in one more pair.
	std::string command =
		"\"" + std::string(editor) + " " + shell_escape(absolute_path.get()) + "\"";
	int status = system(command.c_str());

	if (status == -1)
		throw ot::status {st::standard_error, "system error: "s + strerror(errno)};
	else if (status != 0)
		throw ot::status {st::child_process_failed,
		                  "Child process exited with " + std::to_string(status)};
#else
	std::string command = std::string(editor) + " -- " + shell_escape(path);
	int status = system(command.c_str());

	if (status == -1)
		throw ot::status {st::standard_error, "waitpid error: "s + strerror(errno)};
	else if (!WIFEXITED(status))
		throw ot::status {st::child_process_failed,
		                 "Child process did not terminate normally: "s + strerror(errno)};
	else if (WEXITSTATUS(status) != 0)
		throw ot::status {st::child_process_failed,
		                  "Child process exited with " + std::to_string(WEXITSTATUS(status))};
#endif
}

timespec ot::get_file_timestamp(const char* path)
{
	timespec mtime;
#ifdef _WIN32
	// stat has a one-second resolution on Windows, so an edit made within that second would go
	// unnoticed. FILETIME counts 100 ns intervals since 1601.
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
		throw status {st::standard_error,
		              path + ": GetFileAttributesEx error: "s + win32_error_message(GetLastError())};
	constexpr uint64_t unix_epoch = 116444736000000000; // 1970-01-01 in FILETIME units
	uint64_t ticks = (uint64_t(data.ftLastWriteTime.dwHighDateTime) << 32 |
	                  data.ftLastWriteTime.dwLowDateTime) - unix_epoch;
	mtime.tv_sec = static_cast<time_t>(ticks / 10000000);
	mtime.tv_nsec = static_cast<long>(ticks % 10000000 * 100);
#else
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
#endif
	return mtime;
}
