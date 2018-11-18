#include <opustags.h>

/**
 * Main entry point to the opustags binary.
 *
 * Does practically nothing but call the cli module.
 */
int main(int argc, char** argv) {
	ot::status rc;
	ot::options opt;
	rc = process_options(argc, argv, opt);
	if (rc == ot::st::exit_now) {
		return EXIT_SUCCESS;
	} else if (rc != ot::st::ok) {
		if (!rc.message.empty())
			fprintf(stderr, "error: %s\n", rc.message.c_str());
		return EXIT_FAILURE;
	}

	rc = run(opt);
	if (rc != ot::st::ok && rc != ot::st::exit_now) {
		if (!rc.message.empty())
			fprintf(stderr, "error: %s\n", rc.message.c_str());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
