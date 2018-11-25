opustags changelog
==================

1.2.0 - 2018-11-25
------------------

- Preserve extra data in OpusTags past the comments.
- Improve error reporting.
- Fix various bugs.

This is the biggest release for opustags. The whole code base was reviewed for robustness and
clarity. The program is now built as C++14, and the code refactored without sacrificing the
original simplicity. It is shipped with a new test suite.

1.1.1 - 2018-10-24
------------------

- Mac OS X support.
- Tolerate but truncate the data in the OpusTags packet past the comments.

1.1 - 2013-01-02
----------------

- Add the --in-place option.
- Fix a bug is --set-all where the last unterminated line was ignored.
- Remove broken output files on failure.

1.0 - 2013-01-01
----------------

This is the first release of opustags. It supports all the main feature for basic tag editing.

It was written in a day, and the code is quick and dirty, though the program is simple and
efficient.
