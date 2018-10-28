# Contributing to opustags

opustags is not quite active nor mature, but contributions are welcome.

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

The current low-expectation plan to improve this project comprises the
following directions, in whatever order:

- Write a complete test suite in Perl, calling opustags as a subprocess.
- Build opustags in C++14. The code will remain mostly C.
- Build the project with CMake, or maybe autotools.
- Refactor the code to improve the readability.
- Integrate code from the `next` branch.
- Fuzz it. Memcheck it.
