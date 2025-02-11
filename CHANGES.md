Changes in mmd
==============


Changes in v2.1 (YYYY-MM-DD)
----------------------------

- Fixed '>' incorrectly exiting a code block.
- Fixed an off-by-1 error in the read buffer that could yield corrupt output.


Changes in v2.0 (2024-03-22)
----------------------------

- Added `mmdLoadIO` and `mmdLoadString` APIs (Issue #12)
- Added mmdutil "-" filename/option to read markdown from stdin (Issue #16)
- Added a document pointer to the other load functions to allow concatenation of
  markdown files.
- Added mmdutil "--no-title" option to disable the generated HTML title page.
- Updated `mmdGetWhitespace` and `mmdIsBlock` functions to return `bool` values.
- Fixed support for emphasized or strong linked text (Issue #15)
- Fixed an issue with headings directly after a table.
- Fixed some more issues with the Commonmark tests.
- Fixed lists in man page output.


Changes in v1.9 (2022-03-01)
----------------------------

- Added support for the Github-flavored markdown task list extension (check
  boxes in lists)
- Addressed some issues found by the Clang static analyzer.


Changes in v1.8 (2021-01-04)
----------------------------

- Markdown of the form `([title](link))` did not parse correctly.
- Addressed an issue identified by the LGTM code scanner.
- Addressed some issues identified by the Cppcheck code scanner.
- Addressed some issues identified by the Coverity code scanner.
- Changed the makefile to only run the unit test program when using the "test"
  target.
- Added a Cppcheck target ("cppcheck") to use this code scanning program against
  the `mmd` sources.


Changes in v1.7 (2019-08-28)
----------------------------

The following changes were made for v1.7:

- Fixed table parsing (Issue #11)
- Fixed block-quoted Setext heading parsing.


Changes in v1.6 (2019-08-18)
----------------------------

The following changes were made for v1.6:

- Fixed some parsing bugs (Issue #7)
- Fixed a crash bug in mmdutil (Issue #8)
- Code fences using "~~~" are now supported.
- Auto-links now properly handle preceding text (Issue #8)
- Inline styles can now span multiple lines (Issue #8)
- Links can now span multiple lines (Issue #8)
- Shortcut links (`[reference]`) didn't work (Issue #8)
- Fixed some issues with inline styles being incorrectly applied for things
  like "* *".
- The `testmmd` program now supports running tests from the CommonMark
  specification and/or from the CommonMark test suite (Issue #9)
- More CommonMark features (code languages, link titles, space-filled thematic
  breaks) and edge cases are now supported (Issue #10)
- Added new `mmdGetOptions` and `mmdSetOptions` functions to control which
  extensions are supported.
- Added new `mmdGetExtra` function to get the link title or code language
  string associated with certain nodes.


Changes in v1.5 (2019-02-17)
----------------------------

The following changes were made for v1.5:

- Added support for referenced links (Issue #1)
- Added support for `__bold__`, `_italic_`, `~~strikethrough~~`, and hard
  line breaks (Issue #4)


Changes in v1.4 (2019-01-04)
----------------------------

The following changes were made for v1.4:

- Fixed a table parsing bug where trailing pipes would add empty cells on the
  right side.
- Tweaked the `mmdutil` program's default HTML stylesheet.
- Fixed `mmdutil` error messages that incorrectly called the program `mmdbook`.
- Fixed some Clang static analyzer warnings in `mmd.c`.
- Fixed a build issue with Visual Studio.


Changes in v1.3 (2018-02-10)
----------------------------

The following changes were made for v1.3:

- Added `mmdCopyAllText` function that returns all of the text under the given
  node.
- Added `mmdutil` program for converting markdown to HTML and man files.


Changes in v1.2 (2018-02-02)
----------------------------

The following changes were made for v1.2:

- Changed license to Apache License Version 2.0
- Added support for markdown tables (Issue #3)


Changes in v1.1 (2017-10-30)
----------------------------

The following changes were made for v1.1:

- The `mmd.h` header now includes the C++ `extern "C"` wrapper around the C
  function prototypes.
- Added a `mmdLoadFile` function that loads a markdown document from a `FILE`
  pointer.
- Fixed a parsing bug for emphasized, bold, and code text containing whitespace.
- Fixed a parsing bug for escaped characters followed by unescaped formatting
  sequences.
- Fixed a parsing bug for headings that follow a list.

Changes in v1.0 (2017-04-23)
----------------------------

- Initial release.
