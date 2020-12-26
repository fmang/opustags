opustags
========

View and edit Ogg Opus comments.

opustags is designed to be fast and as conservative as possible, to the point that if you edit tags
then edit them again to their previous values, you should get a bit-perfect copy of the original
file. No under-the-cover operation like writing "edited with opustags" or timestamp tagging will
ever be performed.

It currently has the following limitations:

- The total size of all tags cannot exceed 64 kB, the maximum size of one Ogg page.
- Multiplexed streams are not supported.
- Newlines inside tags are not supported by `--set-all`.

If you'd like one of these limitations lifted, please do open an issue explaining your use case.
Feel free to ask for new features too.

Requirements
------------

* a POSIX-compliant system,
* a C++17 compiler,
* CMake ≥ 3.9,
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
      --raw                         disable encoding conversion

See the man page, `opustags.1`, for extensive documentation.
