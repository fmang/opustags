opustags
========

View and edit Opus comments.

**Please note this project is old and not actively maintained.**
Maybe you should use something else.

It was built because at the time nothing supported Opus, but now things have
changed for the better.

For alternative, check out these projects:

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
      -p, --picture NAME      add cover (<64k in BASE64)
      -D, --delete-all        delete all the fields!
      -S, --set-all           read the fields from stdin

See the man page, `opustags.1`, for extensive documentation.
