---
title: How to Use the mmd "Library"
author: Michael R Sweet
copyright: Copyright © 2017-2024 by Michael R Sweet
version: 2.0
...

# Contents

[How to Use the mmd "Library"](@)
- [Overview](@)
- [Navigating the Document Tree](@)
- [Retrieving Document Metadata](@)
- [Freeing Memory](@)

[Example: Generating HTML from Markdown](@)

[Reference](@)


# How to Use the mmd "Library"

## Overview

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

- Tables and task items as used by the [Github Flavored Markdown Spec][GFM].

[CommonMark]: https://spec.commonmark.org
[GFM]: https://github.github.com/gfm

`mmd` represents a markdown document as a tree of nodes, each of type `mmd_t`.
The `mmdLoad` function loads a document on disk into memory and returns the root
node of the document:

    mmd_t *doc = mmdLoad(NULL, "filename.md");

Each node has an associated type that can be retrieved using the `mmdGetType`
function.  The value is represented as an enumeration:

- `MMD_TYPE_DOCUMENT` - The root node of a document.
- `MMD_TYPE_METADATA` - The document metadata; child nodes are only of type
  `MMD_TYPE_METADATA_TEXT`.
- `MMD_TYPE_METADATA_TEXT` - Document metadata text items; text is of the form
  "keyword: value".
- `MMD_TYPE_BLOCK_QUOTE` - A collection of quoted blocks.
- `MMD_TYPE_ORDERED_LIST` - An ordered (numbered) list; child nodes are only of
  type `MMD_TYPE_LIST_ITEM`.
- `MMD_TYPE_UNORDERED_LIST` - An unordered (bulleted) list; child nodes are only
  of type `MMD_TYPE_LIST_ITEM`.
- `MMD_TYPE_LIST_ITEM` - A list item; child nodes can be text or other blocks.
- `MMD_TYPE_TABLE` - A table.
- `MMD_TYPE_TABLE_HEADER` - The table header.
- `MMD_TYPE_TABLE_BODY` - The table body.
- `MMD_TYPE_TABLE_ROW` - A table row.
- `MMD_TYPE_HEADING_1` - A level 1 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_2` - A level 2 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_3` - A level 3 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_4` - A level 4 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_5` - A level 5 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_6` - A level 6 heading; child nodes are text or images.
- `MMD_TYPE_PARAGRAPH` - A paragraph; child nodes are text or images.
- `MMD_TYPE_CODE_BLOCK` - A block of preformatted, monospaced text; child nodes
  are only of type `MMD_TYPE_CODE_TEXT`.
- `MMD_TYPE_THEMATIC_BREAK` - A horizontal rule or page break.
- `MMD_TYPE_TABLE_HEADER_CELL` - A table header cell.
- `MMD_TYPE_TABLE_BODY_CELL_LEFT` - A left-aligned table cell.
- `MMD_TYPE_TABLE_BODY_CELL_CENTER` - A centered table cell.
- `MMD_TYPE_TABLE_BODY_CELL_RIGHT` - A right-aligned table cell.
- `MMD_TYPE_NORMAL_TEXT` - A text fragment with no special formatting.
- `MMD_TYPE_EMPHASIZED_TEXT` -  A text fragment with emphasized formatting,
  typically italics.
- `MMD_TYPE_STRONG_TEXT` - A text fragment with strong formatting, typically
  boldface.
- `MMD_TYPE_STRUCK_TEXT` - A text fragment that is presented with a line through
  it.
- `MMD_TYPE_LINKED_TEXT` - A text fragment that links to a heading within the
  document or an external resource.
- `MMD_TYPE_CODE_TEXT` - A text fragment that contains preformatted, monospaced
  text.
- `MMD_TYPE_IMAGE` - An inline image.
- `MMD_TYPE_HARD_BREAK` - A hard line break.
- `MMD_TYPE_SOFT_BREAK` - A soft line/word break.
- `MMD_TYPE_CHECKBOX` - A checkbox as used in task lists.

Generally there are two categories of nodes: "block" nodes which contain other
nodes and "leaf" nodes that contain text fragments, links, images, and breaks.
The `mmdIsBlock` function provides a quick test whether a given node is a
block or leaf node.

The `mmdGetText` function retrieves the text fragment associated with the
node.  The `mmdGetWhitespace` function reports whether there was leading
whitespace before the text fragment or image.  And the `mmdGetURL` function
retrieves the URL associated with a `MMD_TYPE_LINKED_TEXT` or `MMD_TYPE_IMAGE`
node.

For `MMD_TYPE_CODE_BLOCK` and `MMD_TYPE_LINKED_TEXT` nodes, the `mmdGetExtra`
function retrieves the code language or link title, respectively.


## Navigating the Document Tree

The document tree connects nodes to their parent, children, and siblings. The
following shows a typical markdown document tree:

    Document
       |
    Metadata ---- Heading 1 ---- Paragraph ---- .... ---- Paragraph
       |               |            |                        |
    "title: Moby Dick" |            |                        |
                  "1." "Loomings."  |                        |
                                 "Call" "me" "Ishmael." ...  |
                                                          "It" "so" ...

Except for the document root node, each node has a parent which can be accessed
using the `mmdGetParent` function.  Child nodes are accessed using the
`mmdGetFirstChild` and `mmdGetLastChild` functions.  Sibling nodes are accessed
using the `mmdGetPrevSibling` and `mmdGetNextSibling` functions:

    mmd_t *node;

    mmd_t *parent = mmdGetParent(node);
    mmd_t *first_child = mmdGetFirstChild(node);
    mmd_t *last_child = mmdGetLastChild(node);
    mmd_t *prev_sibling = mmdGetPrevSibling(node);
    mmd_t *next_sibling = mmdGetNextSibling(node);


## Retrieving Document Metadata

The `mmdGetMetadata` function retrieves the metadata associated with a given
keyword.  For example, the following code will retrieve the title of the
document:

    mmd_t *doc; /* previously loaded document */

    const char *title = mmdGetMetadata(doc, "title");


## Freeing Memory

The `mmdFree` function frees the memory used for the document tree:

    mmd_t *doc; /* previously loaded document */

    mmdFree(doc);


# Example: Generating HTML from Markdown

One of the most common uses for markdown is for generating HTML, and the
`testmmd` program included with `mmd` does exactly that using four functions:

- [write_block - Write Block Nodes](@)
- [write_leaf - Write Leaf Nodes for a Block](@)
- [write_html - Write Text and URL Values](@)
- [make_anchor - Make a HTML Anchor String from Text](@)

## write_block - Write Block Nodes

The `write_block` function is responsible for writing HTML blocks and thematic
breaks.  Generally speaking, the function writes an open tag for the block,
iterates through the node's children, and then writes a close tag for the block.
There are two exceptions:

1. `MMD_TYPE_CODE_BLOCK` - Child nodes are code text (`MMD_TYPE_CODE_TEXT`) that
   contain all whitespace and newlines that should be written directly.
2. `MMD_TYPE_THEMATIC_BREAK` - There are no child nodes, so a horizontal rule
   tag (`<hr>`) is written.

This function also generates anchors for each heading so that internal
references ("@" links) work.

Here is the complete function:

    static void
    write_block(mmd_t *parent)
    {
      const char *element, hclass = NULL;
      mmd_t *node;
      mmd_type_t type;


      switch (type = mmdGetType(parent))
      {
        case MMD_TYPE_BLOCK_QUOTE :
            element = "blockquote";
            break;

        case MMD_TYPE_ORDERED_LIST :
            element = "ol";
            break;

        case MMD_TYPE_UNORDERED_LIST :
            element = "ul";
            break;

        case MMD_TYPE_LIST_ITEM :
            element = "li";
            break;

        case MMD_TYPE_HEADING_1 :
            element = "h1";
            break;

        case MMD_TYPE_HEADING_2 :
            element = "h2";
            break;

        case MMD_TYPE_HEADING_3 :
            element = "h3";
            break;

        case MMD_TYPE_HEADING_4 :
            element = "h4";
            break;

        case MMD_TYPE_HEADING_5 :
            element = "h5";
            break;

        case MMD_TYPE_HEADING_6 :
            element = "h6";
            break;

        case MMD_TYPE_PARAGRAPH :
            element = "p";
            break;

        case MMD_TYPE_CODE_BLOCK :
            if ((hclass = mmdGetExtra(parent)) != NULL)
              fprintf(fp, "<pre><code class=\"language-%s\">", hclass);
            else
              fputs("<pre><code>", fp);

            for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
              write_html(mmdGetText(node));
            puts("</code></pre>");
            return;

        case MMD_TYPE_THEMATIC_BREAK :
            puts("    <hr />");
            return;

        case MMD_TYPE_TABLE :
            element = "table";
            break;

        case MMD_TYPE_TABLE_HEADER :
            element = "thead";
            break;

        case MMD_TYPE_TABLE_BODY :
            element = "tbody";
            break;

        case MMD_TYPE_TABLE_ROW :
            element = "tr";
            break;

        case MMD_TYPE_TABLE_HEADER_CELL :
            element = "th";
            break;

        case MMD_TYPE_TABLE_BODY_CELL_LEFT :
            element = "td";
            break;

        case MMD_TYPE_TABLE_BODY_CELL_CENTER :
            element = "td";
            hclass  = "center";
            break;

        case MMD_TYPE_TABLE_BODY_CELL_RIGHT :
            element = "td";
            hclass  = "right";
            break;

        default :
            element = NULL;
            break;
      }

      if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6)
      {
       /*
        * Add an anchor for each heading...
        */

        printf("    <%s id=\"", element);
        for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
        {
          if (mmdGetWhitespace(node))
            fputc('-', fp);

          fputs(make_anchor(mmdGetText(node)), stdout);
        }
        fputs("\">", stdout);
      }
      else if (element)
        printf("    <%s%s%s>%s", element, hclass ? " class=" : "", hclass ? hclass : "", type <= MMD_TYPE_UNORDERED_LIST ? "\n" : "");

      for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
      {
        if (mmdIsBlock(node))
          write_block(node);
        else
          write_inline(node);
      }

      if (element)
        printf("</%s>\n", element);
    }


## write_leaf - Write Leaf Nodes for a Block

The `write_leaf` function is responsible for writing HTML text and inline
elements.  Generally speaking, the function writes the text for the node
surrounded by open and close tags.  If whitespace preceded the node, it writes
a space before the text.  There are four exceptions:

1. `MMD_TYPE_IMAGE` - An `<img>` tag is written using the URL as the source and
   the text as the alternate value.
2. `MMD_TYPE_HARD_BREAK` - A `<br>` tag is written.
3. `MMD_TYPE_SOFT_BREAK` - A `<wbr>` tag is written.
4. `MMD_TYPE_CHECKBOX` - A `<svg>` tag is written for a checked or unchecked
   box depending on the text value.

Linked text gets some optimizations to minimize the number of `<a>` tags that
get written, as well as using the `mmdGetExtra` function to get the link title,
if any.

In addition, some simple text substitutions are performed for "(c)", "(r)", and
"(tm)" to use the corresponding HTML entities for copyright, registered
trademark, and trademark.

Here is the complete function:

    static void
    write_leaf(mmd_t *node)
    {
      const char *element, *text, *url;


      if (mmdGetWhitespace(node))
        putchar(' ');

      text = mmdGetText(node);
      url  = mmdGetURL(node);

      switch (mmdGetType(node))
      {
        case MMD_TYPE_EMPHASIZED_TEXT :
            element = "em";
            break;

        case MMD_TYPE_STRONG_TEXT :
            element = "strong";
            break;

        case MMD_TYPE_STRUCK_TEXT :
            element = "del";
            break;

        case MMD_TYPE_LINKED_TEXT :
            element = NULL;
            break;

        case MMD_TYPE_CODE_TEXT :
            element = "code";
            break;

        case MMD_TYPE_IMAGE :
            fputs("<img src=\"", stdout);
            write_html(url);
            fputs("\" alt=\"", stdout);
            write_html(text);
            fputs("\" />", stdout);
            return;

        case MMD_TYPE_HARD_BREAK :
            puts("<br />");
            return;

        case MMD_TYPE_SOFT_BREAK :
            puts("<wbr />");
            return;

        case MMD_TYPE_METADATA_TEXT :
            return;

        case MMD_TYPE_CHECKBOX :
            if (text)
              fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" fill=\"currentColor\" class=\"bi bi-check-square\" viewBox=\"0 0 16 16\"><path d=\"M14 1a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1h12zM2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2H2z\"/><path d=\"M10.97 4.97a.75.75 0 0 1 1.071 1.05l-3.992 4.99a.75.75 0 0 1-1.08.02L4.324 8.384a.75.75 0 1 1 1.06-1.06l2.094 2.093 3.473-4.425a.235.235 0 0 1 .02-.022z\"/></svg>", stdout);
            else
              fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" fill=\"currentColor\" class=\"bi bi-square\" viewBox=\"0 0 16 16\"><path d=\"M14 1a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1h12zM2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2H2z\"/></svg>", stdout);
            return;

        default :
            element = NULL;
            break;
      }

      if (url)
      {
        const char *prev_url = mmdGetURL(mmdGetPrevSibling(node));
        const char *title = mmdGetExtra(node);

        if (!prev_url || strcmp(prev_url, url))
        {
          if (!strcmp(url, "@"))
            printf("<a href=\"#%s\"", make_anchor(text));
          else
            printf("<a href=\"%s\"", url);

          if (title)
          {
            fputs(" title=\"", stdout);
            write_html(title);
            fputs("\">", stdout);
          }
          else
            putchar('>');
        }
      }

      if (element)
        printf("<%s>", element);

      if (!strcmp(text, "(c)"))
        fputs("&copy;", stdout);
      else if (!strcmp(text, "(r)"))
        fputs("&reg;", stdout);
      else if (!strcmp(text, "(tm)"))
        fputs("&trade;", stdout);
      else
        write_html(text);

      if (element)
        printf("</%s>", element);

      if (url)
      {
        const char *next_url = mmdGetURL(mmdGetNextSibling(node));

        if (!next_url || strcmp(next_url, url))
          fputs("</a>", fp);
      }
    }


## write_html - Write Text and URL Values

The `write_html` function is responsible for writing text and URL values as
HTML, substituting HTML entities for special characters like `&`, `<`, `>`, and
`"`.

Here is the complete function:

    static void
    write_html(const char *text)
    {
      if (!text)
        return;

      while (*text)
      {
        if (*text == '&')
          fputs("&amp;", stdout);
        else if (*text == '<')
          fputs("&lt;", stdout);
        else if (*text == '>')
          fputs("&gt;", stdout);
        else if (*text == '\"')
          fputs("&quot;", stdout);
        else
          putchar(*text);

        text ++;
      }
    }


## make_anchor - Make a HTML Anchor String from Text

The `make_anchor` function is responsible for converting a text string into a
HTML anchor string.  This particular implementation removes characters that are
not letters, numbers, periods, or dashes.

Here is the complete function:

    static const char *
    make_anchor(const char *text)
    {
      char *bufptr;
      static char buffer[1024];


      for (bufptr = buffer; *text && bufptr < (buffer + sizeof(buffer) - 1); text ++)
      {
        if ((*text >= '0' && *text <= '9') || (*text >= 'a' && *text <= 'z') || (*text >= 'A' && *text <= 'Z') || *text == '.' || *text == '-')
          *bufptr++ = *text;
      }

      *bufptr = '\0';

      return (buffer);
    }


# Reference

- [mmd_t](@)
- [mmd_iocb_t](@)
- [mmd_option_t](@)
- [mmd_type_t](@)
- [mmdCopyAllText](@)
- [mmdFree](@)
- [mmdGetExtra](@)
- [mmdGetFirstChild](@)
- [mmdGetLastChild](@)
- [mmdGetMetadata](@)
- [mmdGetNextSibling](@)
- [mmdGetOptions](@)
- [mmdGetParent](@)
- [mmdGetPrevSibling](@)
- [mmdGetText](@)
- [mmdGetType](@)
- [mmdGetURL](@)
- [mmdGetWhitespace](@)
- [mmdIsBlock](@)
- [mmdLoad](@)
- [mmdLoadFile](@)
- [mmdLoadIO](@)
- [mmdLoadString](@)
- [mmdSetOptions](@)

## mmd\_t

    typedef struct _mmd_s mmd_t;

The `mmd_t` object represents a single node within a markdown document.  Each
node has an associated type and may have text, link, siblings, children, and
a parent.


## mmd\_iocb\_t

    typedef size_t (*mmd_iocb_t)(void *cbdata, char *buffer, size_t bytes);

The `mmd_iocb_t` type represents an I/O callback function that is used to read
data with the [`mmdLoadIO`](@) function.  The function copies up to `bytes`
bytes from the source to the `buffer` and returns the number of bytes copied.


## mmd\_option\_t

    enum mmd_option_e
    {
      MMD_OPTION_NONE,
      MMD_OPTION_METADATA,
      MMD_OPTION_TABLES,
      MMD_OPTION_ALL
    };
    typedef unsigned mmd_option_t;

The `mmd_option_t` enumeration is a bit mask representing which markdown
extensions are supported by [`mmdLoad`](@), [`mmdLoadFile`](@),
[`mmdLoadIO`](@), and [`mmdLoadString`](@).


## mmd\_type\_t

    typedef enum mmd_type_e
    {
      MMD_TYPE_NONE,
      MMD_TYPE_DOCUMENT,
      MMD_TYPE_METADATA,
      MMD_TYPE_BLOCK_QUOTE,
      MMD_TYPE_ORDERED_LIST,
      MMD_TYPE_UNORDERED_LIST,
      MMD_TYPE_LIST_ITEM,
      MMD_TYPE_TABLE,
      MMD_TYPE_TABLE_HEADER,
      MMD_TYPE_TABLE_BODY,
      MMD_TYPE_TABLE_ROW,
      MMD_TYPE_HEADING_1,
      MMD_TYPE_HEADING_2,
      MMD_TYPE_HEADING_3,
      MMD_TYPE_HEADING_4,
      MMD_TYPE_HEADING_5,
      MMD_TYPE_HEADING_6,
      MMD_TYPE_PARAGRAPH,
      MMD_TYPE_CODE_BLOCK,
      MMD_TYPE_THEMATIC_BREAK,
      MMD_TYPE_TABLE_HEADER_CELL,
      MMD_TYPE_TABLE_BODY_CELL_LEFT,
      MMD_TYPE_TABLE_BODY_CELL_CENTER,
      MMD_TYPE_TABLE_BODY_CELL_RIGHT,
      MMD_TYPE_NORMAL_TEXT,
      MMD_TYPE_EMPHASIZED_TEXT,
      MMD_TYPE_STRONG_TEXT,
      MMD_TYPE_STRUCK_TEXT,
      MMD_TYPE_LINKED_TEXT,
      MMD_TYPE_CODE_TEXT,
      MMD_TYPE_IMAGE,
      MMD_TYPE_HARD_BREAK,
      MMD_TYPE_SOFT_BREAK,
      MMD_TYPE_METADATA_TEXT,
      MMD_TYPE_CHECKBOX
    } mmd_type_t;

The `mmd_type_t` enumeration represents all of the markdown node types.


## mmdCopyAllText

    char *
    mmdCopyAllText(mmd_t *node);

The `mmdCopyAllText` function copies all of the text under the specified node.
It is typically used for extracting text for headings and other block nodes.
The returned string pointer must be freed using the `free` function.  `NULL` is
returned if there is no text under the node.


## mmdFree

    void
    mmdFree(mmd_t *node);

The `mmdFree` function frees the specified node and all of its children.  It is
typically only used to free the entire markdown document, starting at the root
node.


## mmdGetExtra

    const char *
    mmdGetExtra(mmd_t *node);

The `mmdGetExtra` function returns any extra text associated with the specified
node.  Currently this can be the code language for `MMD_TYPE_CODE_BLOCK` nodes
and the link title for `MMD_TYPE_LINKED_TEXT` nodes.


## mmdGetFirstChild

    mmd_t *
    mmdGetFirstChild(mmd_t *node);

The `mmdGetFirstChild` function returns the first child of the specified node,
if any.


## mmdGetLastChild

    mmd_t *
    mmdGetLastChild(mmd_t *node);

The `mmdGetLastChild` functions returns the last child of the specified node,
if any.


## mmdGetMetadata

    const char *
    mmdGetMetadata(mmd_t *doc, const char *keyword);

The `mmdGetMetadata` function returns the document metadata for the specified
keyword.  Standard keywords include "author", "copyright", and "title".


## mmdGetNextSibling

    mmd_t *
    mmdGetNextSibling(mmd_t *node);

The `mmdGetNextSibling` function returns the next sibling of the specified node,
if any.


## mmdGetOptions

    mmd_option_t
    mmdGetOptions(void);

The `mmdGetOptions` function returns the current load options for `mmd` as an
[enumerated bit mask](#mmd_option_t).


## mmdGetParent

    mmd_t *
    mmdGetParent(mmd_t *node);

The `mmdGetParent` function returns the parent of the specified node, if any.


## mmdGetPrevSibling

    mmd_t *
    mmdGetPrevSibling(mmd_t *node);

The `mmdGetPrevSibling` function returns the previous sibling of the specified
node, if any.


## mmdGetText

    const char *
    mmdGetText(mmd_t *node);

The `mmdGetText` function returns any text that is associated with the specified
node.  For `MMD_TYPE_CHECKBOX` nodes, the text is "x" for checked boxes and
`NULL` for unchecked boxes.


## mmdGetType

    mmd_type_t
    mmdGetType(mmd_t *node);

The `mmdGetType` function returns the type of the specified node.


## mmdGetURL

    const char *
    mmdGetURL(mmd_t *node);

The `mmdGetURL` function returns any URL that is associated with the specified
node.


## mmdGetWhitespace

    bool
    mmdGetWhitespace(mmd_t *node);

The `mmdGetWhitespace` function returns `true` if whitespace preceded the
specified node and `false` otherwise.


## mmdIsBlock

    bool
    mmdIsBlock(mmd_t *node);

The `mmdIsBlock` function returns `true` when the specified node is a markdown
block and `false` otherwise.


## mmdLoad

    mmd_t *
    mmdLoad(mmd_t *root, const char *filename);

The `mmdLoad` function loads a markdown document from the named file.  The
function understands the CommonMark syntax and Jekyll metadata.

The return value is a pointer to the root document node on success or `NULL` on
failure.  Due to the nature of markdown, the only failures are file open errors
and out-of-memory conditions.


## mmdLoadFile

    mmd_t *
    mmdLoadFile(mmd_t *root, FILE *fp);

The `mmdLoadFile` function loads a markdown document from the specified `FILE`
pointer.  The function understands the CommonMark syntax and Jekyll metadata.

The return value is a pointer to the root document node on success or `NULL` on
failure.  Due to the nature of markdown, the only failures are out-of-memory
conditions.


## mmdLoadIO

    mmd_t *
    mmdLoadIO(mmd_t *root, mmd_iocb_t cb, void *cbdata);

The `mmdLoadIO` function loads a markdown document using the specified read
callback function `cb` and data `cbdata`.  The function understands the
CommonMark syntax and Jekyll metadata.

The return value is a pointer to the root document node on success or `NULL` on
failure.  Due to the nature of markdown, the only failures are out-of-memory
conditions.


## mmdLoadString

    mmd_t *
    mmdLoadString(mmd_t *root, const char *s);

The `mmdLoadString` function loads a markdown document from the specified
string.  The function understands the CommonMark syntax and Jekyll metadata.

The return value is a pointer to the root document node on success or `NULL` on
failure.  Due to the nature of markdown, the only failures are out-of-memory
conditions.


## mmdSetOptions

    void
    mmdSetOptions(mmd_option_t options);

The `mmdSetOptions` function sets the current load options for [`mmdLoad`](@)
and [`mmdLoadFile`](@). The options are an [enumerated bit mask](#mmd_option_t)
whose values are:

- `MMD_OPTION_NONE`: No markdown extensions are enabled when loading.
- `MMD_OPTION_METADATA`: The Jekyll metadata extension is enabled when
  loading.
- `MMD_OPTION_TABLES`: The Github table extension is enabled when loading.
- `MMD_OPTION_TASKS`: The Github task item extension is enabled when loading.
- `MMD_OPTION_ALL`: All supported markdown extensions are enabled when loading.

The default value is `MMD_OPTION_ALL`.
