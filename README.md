# mmd

This is a miniature markdown parsing "library" consisting of a single C source
file and accompanying header file.  `mmd` mostly conforms to the CommonMark
version of markdown syntax with the following exceptions:

- Embedded HTML markup and entities are explicitly not supported or allowed;
  the reason for this is to better support different kinds of output from the
  markdown "source", including XHTML, man, and `xml2rfc`.

- Thematic breaks using a mix of whitespace and the separator character are not
  supported ("* * * *", "-- -- -- --", etc.); these could conceivably be added
  but did not seem particularly important.

- Link titles are silently ignored.

In addition, `mmd` supports a couple (otherwise undocumented) CommonMark
extensions:

- Metadata as used by Jekyll and other web markdown solutions.

- "@" links which resolve to headings within the file.

I'm providing this as open source under the "new" 2-clause BSD license which
allows you do pretty much do whatever you like with it.  Please do provide
feedback and report bugs to the Github project page at:

    https://github.com/michaelrsweet/mmd

so that everyone can benefit.


## Requirements

You'll need a C compiler.


## How to Incorporate With Your Project

Add the `mmd.c` and `mmd.h` files to your project.  Include the `mmd.h`
header in any file that needs to read/convert markdown files.


## "Kicking the Tires"

The supplied makefile allows you to build the unit tests on Linux and macOS (at
least), which verify that all of the functions work as expected to produce a
HTML file called `testmmd.html`:

    make


## Legal Stuff

Copyright (c) 2017 by Michael R Sweet.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
