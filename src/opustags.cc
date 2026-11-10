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
#endif
		setlocale(LC_ALL, "");
		ot::options opt = ot::parse_options(argc, argv, stdin);
		ot::run(opt);
		return 0;
	} catch (const ot::status& rc) {
		if (!rc.message.empty())
			fprintf(stderr, "error: %s\n", rc.message.c_str());
		return rc == ot::st::bad_arguments ? 2 : 1;
	}
}
