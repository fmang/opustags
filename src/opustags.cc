/**
 * \file src/opustags.cc
 * \brief Main function for opustags.
 *
 * See opustags.h for the program's documentation.
 */

#include <opustags.h>

#include <locale.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <signal.h>
#include <stdlib.h>
#include <windows.h>

static UINT initial_console_output_cp; /**< Console code page to restore on exit, if not 0. */

static void restore_console_output_cp()
{
	if (initial_console_output_cp != 0)
		SetConsoleOutputCP(initial_console_output_cp);
}

static BOOL WINAPI restore_on_console_event(DWORD)
{
	restore_console_output_cp();
	return FALSE; // Let the default handler terminate the process.
}

static LONG WINAPI restore_on_crash(EXCEPTION_POINTERS*)
{
	restore_console_output_cp();
	return EXCEPTION_CONTINUE_SEARCH;
}

static void restore_on_abort(int)
{
	restore_console_output_cp();
}

/**
 * Print UTF-8 correctly in the console, which uses the OEM code page by default. The original
 * code page is restored however the program exits, unless it is killed.
 *
 * The input code page is left alone because the Windows 10 console reads non-ASCII characters as
 * NUL bytes with UTF-8.
 */
static void use_utf8_console_output()
{
	UINT cp = GetConsoleOutputCP();
	if (GetACP() != CP_UTF8 || cp == 0 || cp == CP_UTF8)
		return;
	initial_console_output_cp = cp;
	atexit(restore_console_output_cp);
	SetConsoleCtrlHandler(restore_on_console_event, TRUE);
	SetUnhandledExceptionFilter(restore_on_crash);
	signal(SIGABRT, restore_on_abort);
	SetConsoleOutputCP(CP_UTF8);
}
#endif

/**
 * Main function of the opustags binary.
 *
 * Does practically nothing but call the cli module.
 */
int main(int argc, char** argv) {
	try {
#ifdef _WIN32
		// Ogg data goes through the standard streams, so they must not translate line endings.
		_setmode(_fileno(stdin), _O_BINARY);
		_setmode(_fileno(stdout), _O_BINARY);
		_setmode(_fileno(stderr), _O_BINARY);
		// The UTF-8 code page set by the manifest does not change the locale "" selects.
		setlocale(LC_ALL, GetACP() == CP_UTF8 ? ".UTF8" : "");
		use_utf8_console_output();
#else
		setlocale(LC_ALL, "");
#endif
		ot::options opt = ot::parse_options(argc, argv, stdin);
		ot::run(opt);
		return 0;
	} catch (const ot::status& rc) {
		if (!rc.message.empty())
			fprintf(stderr, "error: %s\n", rc.message.c_str());
		return rc == ot::st::bad_arguments ? 2 : 1;
	}
}
