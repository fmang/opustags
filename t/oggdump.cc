/**
 * \file t/oggdump.cc
 *
 * Dump brief information about the pages containted in an Ogg file.
 *
 * This tool is not build by default or installed, and is mainly meant to help understand how Ogg
 * files are built, and to debug.
 */

#include <opustags.h>

#include <iostream>
#include <string.h>

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage: oggdump FILE\n";
		return 1;
	}
	ot::file input = fopen(argv[1], "r");
	if (input == nullptr) {
		std::cerr << "Error opening '" << argv[1] << "': " << strerror(errno) << "\n";
		return 1;
	}
	ot::ogg_reader reader(input.get());
	ot::status rc;
	while ((rc = reader.read_page()) == ot::st::ok) {
		std::cout << "Stream " << ogg_page_serialno(&reader.page) << ", "
		             "page #" << ogg_page_pageno(&reader.page) << ", "
		          << ogg_page_packets(&reader.page) << " packet(s)";
		if (ogg_page_bos(&reader.page)) std::cout << ", BoS";
		if (ogg_page_eos(&reader.page)) std::cout << ", EoS";
		if (ogg_page_continued(&reader.page)) std::cout << ", continued";
		std::cout << "\n";
	}
	if (rc != ot::st::ok && rc != ot::st::end_of_stream) {
		std::cerr << "error: " << rc.message << "\n";
		return 1;
	}
	return 0;
}
