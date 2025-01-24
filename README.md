mmd - Miniature Markdown Library
================================

![Version](https://img.shields.io/github/v/release/michaelrsweet/mmd?include_prereleases)
![Apache 2.0](https://img.shields.io/github/license/michaelrsweet/mmd)
![Build Status](https://img.shields.io/github/actions/workflow/status/michaelrsweet/mmd/build.yml?branch=master)
![Coverity Scan Status](https://img.shields.io/coverity/scan/22387.svg)

`mmd` is a miniature markdown parsing "library" consisting of a single C source
file and accompanying header file.  `mmd` mostly conforms to the [CommonMark][]
version of markdown syntax with the following exceptions:

- Embedded HTML markup and entities are explicitly not supported or allowed;
  the reason for this is to better support different kinds of output from the
  markdown "source", including XHTML, man, and `xml2rfc`.

- Tabs are silently expanded to the markdown standard of four spaces since HTML
  uses eight spaces per tab.

- Some pathological nested link and inline style features supported by
  CommonMark (`******Really Strong Text******`) are not supported by `mmd`.

In addition, `mmd` supports a couple (otherwise undocumented) markdown
extensions:

- Metadata as used by Jekyll and other web markdown solutions.

- "@" links which resolve to headings within the file.

- Tables and task lists as used by the [Github Flavored Markdown Spec][GFM].

`mmd` also includes a standalone utility called `mmdutil` that can be used to
generate HTML and man page source from markdown.

I'm providing `mmd` as open source under the Apache License Version 2.0 with
exceptions for use with GPL2/LGPL2 applications which allows you do pretty much
do whatever you like with it.  Please do provide feedback and report bugs to the
Github project page at <https://www.msweet.org/mmd> so that everyone can
benefit.

[CommonMark]: https://spec.commonmark.org
[GFM]: https://github.github.com/gfm


Requirements
------------

You'll need a C compiler.


How to Incorporate in Your Project
----------------------------------

Add the `mmd.c` and `mmd.h` files to your project.  Include the `mmd.h`
header in any file that needs to read/convert markdown files.


"Kicking the Tires"
-------------------

The supplied makefile allows you to build the unit tests on Linux and macOS (at
least), which verify that all of the functions work as expected to produce a
HTML file called `testmmd.html`:

    make test

The makefile also builds the `mmdutil` program.


Installing `mmdutil`
--------------------

You can install the `mmdutil` program by copying it to somewhere appropriate or
run:

    make install

to install it in `/usr/local` along with a man page.


Legal Stuff
-----------

Copyright © 2017-2025 by Michael R Sweet.

mmd is licensed under the Apache License Version 2.0 with an (optional)
exception to allow linking against GPL2/LGPL2-only software.  See the files
"LICENSE" and "NOTICE" for more information.
