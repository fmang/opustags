opustags
========

View and edit Ogg Opus comments.

opustags supports the following features:

- interactive editing using your preferred text editor,
- batch editing with command-line flags,
- tags exporting and importing through text files.

opustags is designed to be fast and as conservative as possible, to the point that if you edit tags
then edit them again to their previous values, you should get a bit-perfect copy of the original
file. No under-the-cover operation like writing "edited with opustags" or timestamp tagging will
ever be performed.

opustags is tag-agnostic: you can write arbitrary key-value tags, and none of them will be treated
specially. After all, common tags like TITLE or ARTIST are nothing more than conventions.

The project’s homepage is located at <https://github.com/fmang/opustags>.

Requirements
------------

* a POSIX-compliant system,
* a C++20 compiler,
* CMake ≥ 3.11,
* libogg 1.3.3.

The version numbers are indicative, and it's very likely opustags will build and work fine with
other versions too, as CMake and libogg are quite mature.

Installing
----------

opustags is a commonplace CMake project.

Here's how to install it in your `.local`, under your home:

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=~/.local ..
    make
    make install

Note that you don't need to install opustags in order to run it, as the executable is standalone.

Documentation
-------------

    Usage: opustags --help
           opustags [OPTIONS] FILE
           opustags OPTIONS -i FILE...
           opustags OPTIONS FILE -o FILE

    Options:
      -h, --help                    print this help
      -o, --output FILE             specify the output file
      -i, --in-place                overwrite the input files
      -y, --overwrite               overwrite the output file if it already exists
      -a, --add FIELD=VALUE         add a comment
      -d, --delete FIELD[=VALUE]    delete previously existing comments
      -D, --delete-all              delete all the previously existing comments
      -s, --set FIELD=VALUE         replace a comment
      -S, --set-all                 import comments from standard input
      -e, --edit                    edit tags interactively in VISUAL/EDITOR
      --output-cover FILE           extract and save the cover art, if any
      --set-cover FILE              sets the cover art
      --vendor                      print the vendor string
      --set-vendor VALUE            set the vendor string
      --raw                         disable encoding conversion

See the man page, `opustags.1`, for extensive documentation.
