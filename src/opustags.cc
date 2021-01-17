/**
 * \file src/opustags.cc
 * \brief Main function for opustags.
 *
 * See opustags.h for the program's documentation.
 */

#include <opustags.h>

#include <locale.h>

/**
 * Main function of the opustags binary.
 *
 * Does practically nothing but call the cli module.
 */
int main(int argc, char** argv) {
	try {
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
