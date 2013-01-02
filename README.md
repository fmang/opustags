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
