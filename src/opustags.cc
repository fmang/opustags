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
	setlocale(LC_ALL, "");
	ot::options opt;
	ot::status rc = ot::parse_options(argc, argv, opt, stdin);
	if (rc == ot::st::ok)
		rc = ot::run(opt);
	else if (!rc.message.empty())
		fprintf(stderr, "error: %s\n", rc.message.c_str());

	return rc == ot::st::ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
