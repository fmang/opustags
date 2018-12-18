opustags
========

View and edit Opus comments.

The current code quality of this project is getting better, and is suitable for reliably editing any
Opus file provided it does not contain other multiplexed streams. Only UTF-8 is currently supported.

Until opustags becomes top-quality software, if it ever does, you might want to
check out these more mature tag editors:

- [EasyTAG](https://wiki.gnome.org/Apps/EasyTAG)
- [Beets](http://beets.io/)
- [Picard](https://picard.musicbrainz.org/)
- [puddletag](http://docs.puddletag.net/)
- [Quod Libet](https://quodlibet.readthedocs.io/en/latest/)
- [Goggles Music Manager](https://gogglesmm.github.io/)

See also these libraries if you need a lower-level access:

- [TagLib](http://taglib.org/)
- [mutagen](https://mutagen.readthedocs.io/en/latest/)

Requirements
------------

* a C++14 compiler,
* CMake,
* a POSIX-compliant system,
* libogg.

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
           opustags OPTIONS FILE -o FILE

    Options:
      -h, --help                    print this help
      -o, --output FILE             specify the output file
      -i, --in-place                overwrite the input file
      -y, --overwrite               overwrite the output file if it already exists
      -a, --add FIELD=VALUE         add a comment
      -d, --delete FIELD[=VALUE]    delete previously existing comments
      -D, --delete-all              delete all the previously existing comments
      -s, --set FIELD=VALUE         replace a comment
      -S, --set-all                 import comments from standard input

See the man page, `opustags.1`, for extensive documentation.
