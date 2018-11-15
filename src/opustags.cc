#include <config.h>

#include "opustags.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ogg/ogg.h>

/**
 * Check if two filepaths point to the same file, after path canonicalization.
 * The path "-" is treated specially, meaning stdin for path_in and stdout for path_out.
 */
static bool same_file(const std::string& path_in, const std::string& path_out)
{
	if (path_in == "-" || path_out == "-")
		return false;
	char canon_in[PATH_MAX+1], canon_out[PATH_MAX+1];
	if (realpath(path_in.c_str(), canon_in) && realpath(path_out.c_str(), canon_out)) {
		return (strcmp(canon_in, canon_out) == 0);
	}
	return false;
}

/**
 * Open the input and output streams, then call #ot::process.
 */
static int run(ot::options& opt)
{
    if (!opt.path_out.empty() && same_file(opt.path_in, opt.path_out)) {
        fputs("error: the input and output files are the same\n", stderr);
        return EXIT_FAILURE;
    }
    ot::ogg_reader reader;
    ot::ogg_writer writer;
    if (opt.path_in == "-") {
        reader.file = stdin;
    } else {
        reader.file = fopen(opt.path_in.c_str(), "r");
        if (reader.file == nullptr) {
            perror("fopen");
            return EXIT_FAILURE;
        }
    }
    writer.file = NULL;
    if (opt.inplace != nullptr)
        opt.path_out = opt.path_in + opt.inplace;
    if (!opt.path_out.empty()) {
        if (opt.path_out == "-") {
            writer.file = stdout;
        } else {
            if (!opt.overwrite && !opt.inplace){
                if (access(opt.path_out.c_str(), F_OK) == 0) {
                    fprintf(stderr, "'%s' already exists (use -y to overwrite)\n", opt.path_out.c_str());
                    fclose(reader.file);
                    return EXIT_FAILURE;
                }
            }
            writer.file = fopen(opt.path_out.c_str(), "w");
            if(!writer.file){
                perror("fopen");
                fclose(reader.file);
                return EXIT_FAILURE;
            }
        }
    }
    ot::status rc = process(reader, writer, opt);
    fclose(reader.file);
    if(writer.file)
        fclose(writer.file);
    if (rc != ot::status::ok) {
        if (!opt.path_out.empty() && writer.file != stdout)
            remove(opt.path_out.c_str());
        return EXIT_FAILURE;
    }
    else if (opt.inplace) {
        if (rename(opt.path_out.c_str(), opt.path_in.c_str()) == -1) {
            perror("rename");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	ot::status rc;
	ot::options opt;
	rc = process_options(argc, argv, opt);
	if (rc == ot::status::exit_now)
		return EXIT_SUCCESS;
	else if (rc != ot::status::ok)
		return EXIT_FAILURE;
	return run(opt);
}
