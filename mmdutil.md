---
title: mmdutil
author: Michael R Sweet
copyright: Copyright © 2017-2024 by Michael R Sweet
version: 2.0
...

# Name

mmdutil - mini markdown utility program.


# Synopsis

mmdutil \[--cover FILENAME.ext\] \[--css FILENAME.css\] \[--front FILENAME.md\] \[--no-title\] \[--toc LEVELS\] \[-o FILENAME.html\] FILENAME.md \[... FILENAME.md\]

mmdutil \[--cover FILENAME.ext\] \[--css FILENAME.css\] \[--front FILENAME.md\]  \[--no-title\] \[--toc LEVELS\] \[-o FILENAME.html\] -

mmdutil \[--front FILENAME.md\] \[--man SECTION\] \[-o FILENAME.man\] FILENAME.md \[... FILENAME.md\]

mmdutil \[--front FILENAME.md\] \[--man SECTION\] \[-o FILENAME.man\] -

mmdutil --help

mmdutil --version


# Description

**mmdutil** is a simple markdown conversion utility that generates HTML or man
page source from markdown.  **mmdutil** supports most of the CommonMark syntax
as well as the metadata, "@" link, table, and task list markdown extensions.
Because **mmdutil** supports non-HTML output formats, embedded HTML is
explicitly *not* supported.

**mmdutil** reads a list of markdown files and/or from the standard input if the
filename "-" is specified.

If no output file is specified using the "-o" option, **mmdutil** sends the
generated document to the standard output.


# Options

The following options are recognized by **mmdutil**:

- "--cover FILENAME.ext" specifies a cover image for HTML output.
- "--css FILENAME.css" specifies a style sheet for HTML output.
- "--front FILENAME.md" specifies front matter for the output.
- "--help" shows program usage.
- "--man SECTION" produces man page output for the specified section.
- "--no-title" disables the title page for HTML output.
- "--toc LEVELS" produces a table of contents with the specified number of
  levels.
- "--version" shows the program version.
- "-o FILENAME.ext" specifies the output file to write.  The default is the
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

Use **mmdutil** as a markdown to HTML filter:

```
mymarkdownprog | mmdutil - | myhtmlprog
```


# See Also

Mini-Markdown Home Page: https://www.msweet.org/mmd
