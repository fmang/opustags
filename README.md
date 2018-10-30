opustags
========

View and edit Opus comments.

The current code quality of this project is in a sorry state, but that might
change in the near future. It is expected to work well though, so please do
open an issue if something doesn't work. New contributions are welcome but make
sure you read CONTRIBUTING.md first.

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

Here's how to install it in your .local, under your home:

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
      -h, --help              print this help
      -o, --output            write the modified tags to a file
      -y, --overwrite         overwrite the output file if it already exists
      -d, --delete FIELD      delete all the fields of a specified type
      -a, --add FIELD=VALUE   add a field
      -s, --set FIELD=VALUE   delete then add a field
      -D, --delete-all        delete all the fields!
      -S, --set-all           read the fields from stdin

See the man page, `opustags.1`, for extensive documentation.
