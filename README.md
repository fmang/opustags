opustags
========

View and edit Opus comments.

Requirements
------------

* A POSIX-compliant system,
* libogg.

Installing
----------

    make
    make DESTDIR=/usr/local install

Documentation
-------------

    Usage: opustags --help
           opustags [OPTIONS] INPUT
           opustags [OPTIONS] -o OUTPUT INPUT

    Options:
      -h, --help              print this help
      -V, --version           print version
      -o, --output FILE       write the modified tags to this file
      -i, --in-place [SUFFIX] use a temporary file then replace the original file
      -y, --overwrite         overwrite the output file if it already exists
          --stream ID         select stream for the next operations
      -l, --list              display a pretty listing of all tags
          --no-color          disable colors in --list output
      -d, --delete FIELD      delete all the fields of a specified type
      -a, --add FIELD=VALUE   add a field
      -s, --set FIELD=VALUE   delete then add a field
      -D, --delete-all        delete all the fields!
          --full              enable full file scan
          --export            dump the tags to standard output for --import
          --import            set the tags from scratch basing on stanard input
      -e, --edit              spawn the $EDITOR and apply --import on the result

See the man page, `opustags.1`, for extensive documentation.
