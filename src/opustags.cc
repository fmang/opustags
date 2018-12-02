/**
 * \file src/opustags.cc
 * \brief Main function for opustags.
 *
 * See opustags.h for the program's documentation.
 */

#include <opustags.h>

/**
 * Main function of the opustags binary.
 *
 * Does practically nothing but call the cli module.
 */
int main(int argc, char** argv) {
	ot::options opt;
	ot::status rc = ot::parse_options(argc, argv, opt);
	if (rc == ot::st::ok)
		rc = ot::run(opt);

	if (rc != ot::st::ok) {
		if (!rc.message.empty())
			fprintf(stderr, "error: %s\n", rc.message.c_str());
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
