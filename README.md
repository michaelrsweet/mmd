# mmd

`mmd` is a miniature markdown parsing "library" consisting of a single C source
file and accompanying header file.  `mmd` mostly conforms to the [CommonMark][]
version of markdown syntax with the following exceptions:

- Embedded HTML markup and entities are explicitly not supported or allowed;
  the reason for this is to better support different kinds of output from the
  markdown "source", including XHTML, man, and `xml2rfc`.

- Link titles are silently ignored.

- Thematic breaks using a mix of whitespace and the separator character are not
  supported ("* * * *", "-- -- -- --", etc.); these could conceivably be added
  but did not seem particularly important.

In addition, `mmd` supports a couple (otherwise undocumented) CommonMark
extensions:

- Metadata as used by Jekyll and other web markdown solutions.

- "@" links which resolve to headings within the file.

- Tables as used by the [Github Flavored Markdown Spec][GFM].

`mmd` also includes a standalone utility called `mmdutil` that can be used to
generate HTML and man page source from markdown.

I'm providing `mmd` as open source under the Apache License Version 2.0 with
exceptions for use with GPL2/LGPL2 applications which allows you do pretty much
do whatever you like with it.  Please do provide feedback and report bugs to the
Github project page at:

    https://www.msweet.org/mmd

so that everyone can benefit.


## Requirements

You'll need a C compiler.


## How to Incorporate in Your Project

Add the `mmd.c` and `mmd.h` files to your project.  Include the `mmd.h`
header in any file that needs to read/convert markdown files.


## "Kicking the Tires"

The supplied makefile allows you to build the unit tests on Linux and macOS (at
least), which verify that all of the functions work as expected to produce a
HTML file called `testmmd.html`:

    make

The makefile also builds the `mmdutil` program.


## Installing `mmdutil`

You can install the `mmdutil` program by copying it to somewhere appropriate or
run:

    make install

to install it in `/usr/local` along with a man page.


## Changes in v1.6

The following changes were made for v1.6:

- Fixed some parsing bugs (Issue #7)
- Fixed a crash bug in mmdutil (Issue #8)
- Code fences using "~~~" are now supported.
- Auto-links now properly handle preceding text (Issue #8)
- Inline styles can now span multiple lines (Issue #8)
- Links can now span multiple lines (Issue #8)
- Fixed some issues with inline styles being incorrectly applied for things
  like "* *".


## Changes in v1.5

The following changes were made for v1.5:

- Added support for referenced links (Issue #1)
- Added support for `__bold__`, `_italic_`, `~~strikethrough~~`, and hard
  line breaks (Issue #4)


## Changes in v1.4

The following changes were made for v1.4:

- Fixed a table parsing bug where trailing pipes would add empty cells on the
  right side.
- Tweaked the `mmdutil` program's default HTML stylesheet.
- Fixed `mmdutil` error messages that incorrectly called the program `mmdbook`.
- Fixed some Clang static analyzer warnings in `mmd.c`.
- Fixed a build issue with Visual Studio.


## Changes in v1.3

The following changes were made for v1.3:

- Added `mmdCopyAllText` function that returns all of the text under the given
  node.
- Added `mmdutil` program for converting markdown to HTML and man files.


## Changes in v1.2

The following changes were made for v1.2:

- Changed license to Apache License Version 2.0
- Added support for markdown tables (Issue #3)


## Changes in v1.1

The following changes were made for v1.1:

- The `mmd.h` header now includes the C++ `extern "C"` wrapper around the C
  function prototypes.
- Added a `mmdLoadFile` function that loads a markdown document from a `FILE`
  pointer.
- Fixed a parsing bug for emphasized, bold, and code text containing whitespace.
- Fixed a parsing bug for escaped characters followed by unescaped formatting
  sequences.
- Fixed a parsing bug for headings that follow a list.


## Legal Stuff

Copyright © 2017-2019 by Michael R Sweet.

mmd is licensed under the Apache License Version 2.0 with an exception to
allow linking against GPL2/LGPL2 software (like older versions of CUPS).  See
the files "LICENSE" and "NOTICE" for more information.


## References

[CommonMark]: https://spec.commonmark.org
[GFM]: https://github.github.com/gfm
