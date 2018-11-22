# Contributing to opustags

opustags is slowing getting more mature, and contributions are welcome.

Before you open a pull request, you might want to talk about the change you'd
like to make to make sure it's relevant. In that case, feel free to open an
issue. You can expect a response within a week.

## Submitting pull requests

opustags has nothing really special, so basic git etiquette is just enough.

Please make focused pull requests, one feature at a time. Don't make huge
commits. Give clear names to your commits and pull requests. Extended
descriptions are welcome.

Stay objective in your changes. Adding a feature or fixing a bug is a clear
improvement, but stylistic changes like renaming a function or moving a few
braces around won't help the project move forward.

You should check that your changes don't break the test suite by running
`make check`

Following these practices is important to keep the history clean, and to allow
for better code reviews.

## History of opustags

opustags is originally a small project made to fill a need to edit tags in Opus
audio files when most taggers didn't support Opus at all. It was written in C
with libogg, and should be very light and fast compared to most alternatives.
However, because it was written on a whim, the code is hardly structured and
might even be fragile, who knows.

An ambitious desire to rewrite it in C++ with bells and whistles gave birth to
the `next` branch, but sadly it wasn't finalized and is currently not usable,
though it contains good pieces of code.

With the growing support of Opus in tag editors, the usefulness of opustags was
questioned, and it was thus abandoned for a few years. Judging by the
inquiries and contributions, albeit few, on GitHub, it looks like it remains
relevant, so let's dust it off a bit.

Today, opustags is written in C++14 and features a unit test suite in C++, and
an integration test suite in Perl. The code was refactored, organized into
modules, and reviewed for safety.

The next release will focus on correctness, with the following technical
objectives:

1. Validate the comments: field name in ASCII and value in UTF-8.
2. Allow selecting the stream to edit, instead of assuming the Ogg contains only
   one Opus stream.
3. Provide an --escape option for escaping the newlines inside comment strings.
4. Take into account the system's encoding: the tags must always be stored as
   UTF-8, and converted from and to the console encoding when reading input or
   printing.
5. Maybe provide a --binary option to dump the raw OpusTags packet, that can be
   combined to --set-all to read it back.
