---
title: mmdutil
author: Michael R Sweet
copyright: Copyright © 2017-2018 by Michael R Sweet
version: 1.3
...

# Name

mmdutil - mini markdown utility program.


# Synopsis

mmdutil [--cover filename.ext] [--css filename.css] [--front filename.md] [--toc levels] [-o filename.html] filename.md [... filenameN.md]

mmdutil [--front filename.md] [--man section] [-o filename.man] filename.md [... filenameN.md]

mmdutil --help

mmdutil --version


# Description

**mmdutil** is a simple markdown conversion utility that generates HTML or man
page source from markdown.  **mmdutil** supports most of the CommonMark syntax
as well as the metadata, "@" link, and table markdown extensions.  Because
mmdutil supports non-HTML output formats, embedded HTML is explicitly *not* supported.

If no output file is specified using the "-o" option, **mmdutil** sends the
generated document to the standard output.


# Options

The following options are recognized by **mmdutil**:

- "--cover filename.ext" specifies a cover image for HTML output.
- "--css filename.css" specifies a style sheet for HTML output.
- "--front filename.md" specifies front matter for the output.
- "--help" shows program usage.
- "--man section" produces man page output for the specified section.
- "--toc levels" produces a table of contents with the specified number of
  levels.
- "--version" shows the program version.
- "-o filename.ext" specifies the output file to write.  The default is the
  standard output.


# Exit Status

**mmdutil** returns 0 on success and 1 on error.


# Examples

Generate a HTML file from "example.md":

    mmdutil example.md >example.html

Generate a HTML file with a table of contents from three markdown files:

    mmdutil --toc 2 intro.md basics.md advanced.md >example.html

Generate a man page from "example.md":

    mmdutil --man 1 example.md >example.1


# See Also

Mini-Markdown Home Page: https://michaelrsweet.github.io/mmd
