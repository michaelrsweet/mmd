---
title: How to Use the mmd "Library"
author: Michael R Sweet
copyright: Copyright © 2017 by Michael R Sweet
...

# How to Use the mmd "Library"

## Overview

`mmd` represents a markdown document as a tree of nodes, each of type `mmd_t`.
The `mmdLoad` function loads a document on disk into memory and returns the root
node:

    mmd_t *doc = mmdLoad("filename.md");

Each node has an associated type that can be retrieved using the `mmdGetType` function. The value is represented as an enumeration:

- `MMD_TYPE_DOCUMENT`; The root node of a document.
- `MMD_TYPE_METADATA`; The document metadata; child nodes are only of type
  `MMD_TYPE_METADATA_TEXT`.
- `MMD_TYPE_METADATA_TEXT`; Document metadata text items; text is of the form
  "keyword: value".
- `MMD_TYPE_BLOCK_QUOTE`; A collection of quoted blocks.
- `MMD_TYPE_ORDERED_LIST`; An ordered (numbered) list; child nodes are only of
  type `MMD_TYPE_LIST_ITEM`.
- `MMD_TYPE_UNORDERED_LIST`; An unordered (bulleted) list; child nodes are only
  of type `MMD_TYPE_LIST_ITEM`.
- `MMD_TYPE_LIST_ITEM`; A list item; child nodes can be text or other blocks.
- `MMD_TYPE_HEADING_1`; A level 1 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_2`; A level 2 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_3`; A level 3 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_4`; A level 4 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_5`; A level 5 heading; child nodes are text or images.
- `MMD_TYPE_HEADING_6`; A level 6 heading; child nodes are text or images.
- `MMD_TYPE_PARAGRAPH`; A paragraph; child nodes are text or images.
- `MMD_TYPE_CODE_BLOCK`; A block of preformatted, monospaced text; child nodes
  are only of type `MMD_TYPE_CODE_TEXT`.
- `MMD_TYPE_THEMATIC_BREAK`; A horizontal rule or page break.
- `MMD_TYPE_NORMAL_TEXT`; A text fragment with no special formatting.
- `MMD_TYPE_EMPHASIZED_TEXT`;  A text fragment with emphasized formatting,
  typically italics.
- `MMD_TYPE_STRONG_TEXT`; A text fragment with strong formatting, typically
  boldface.
- `MMD_TYPE_STRUCK_TEXT`; A text fragment that is presented with a line through
  it.
- `MMD_TYPE_LINKED_TEXT`; A text fragment that links to a heading within the
  document or an external resource.
- `MMD_TYPE_CODE_TEXT`; A text fragment that contains preformatted, monospaced
  text.
- `MMD_TYPE_IMAGE`; An inline image.
- `MMD_TYPE_HARD_BREAK`; A hard line break.
- `MMD_TYPE_SOFT_BREAK`; A soft line/word break.

Generally there are two categories of nodes: "block" nodes which contain other
nodes and "leaf" nodes that contain text fragments, links, images, and breaks.
The `mmdIsBlock` function provides a quick test whether a given node is a block
or leak node.

The `mmdGetText` function retrieves the text fragment associated with the node.
The `mmdGetWhitespace` function reports whether there was leading whitespace
before the text fragment or image. And the `mmdGetURL` function retrieves the
URL associated with a `MMD_TYPE_LINKED_TEXT` or `MMD_TYPE_IMAGE` node.


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

One of the most common uses for Markdown is for generating HTML, and the
`testmmd` program included with `mmd` does exactly that using four functions:
`write_block`, `write_inline`, `write_html`, and `make_anchor`.

## write_block - Write Block Nodes

The `write_block` function is responsible for writing HTML blocks and thematic
breaks.  Generally speaking, the function writes an open tag for the block,
iterates through the node's children, and then writes a close tag for the block.
There are two exceptions:

1. `MMD_TYPE_CODE_BLOCK`: Child nodes are code text (`MMD_TYPE_CODE_TEXT`) that
   contain all whitespace and newlines that should be written directly.
2. `MMD_TYPE_THEMATIC_BREAK`: There are no child nodes, so a horizontal rule tag
   (`<hr>`) is written.

This function also generates anchors for each heading so that internal
references ("@" links) work.

Here is the complete function:

    static void
    write_block(mmd_t *parent)
    {
      const char *element;
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
            fputs("    <pre><code>", stdout);
            for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
              write_html(mmdGetText(node));
            puts("</code></pre>");
            return;

        case MMD_TYPE_THEMATIC_BREAK :
            puts("    <hr />");
            return;

        default :
            element = NULL;
            break;
      }

      if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6)
      {
       /*
        * Add an anchor for each heading...
        */

        printf("    <%s><a id=\"", element);
        for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
          fputs(make_anchor(mmdGetText(node)), stdout);
        fputs("\">", stdout);
      }
      else if (element)
        printf("    <%s>%s", element, type <= MMD_TYPE_UNORDERED_LIST ? "\n" : "");

      for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
      {
        if (mmdIsBlock(node))
          write_block(node);
        else
          write_inline(node);
      }

      if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6)
        printf("</a></%s>\n", element);
      else if (element)
        printf("</%s>\n", element);
    }


## write_inline - Write Text Nodes for a Block

The `write_inline` function is responsible for writing HTML text and inline
elements.  Generally speaking, the function writes the text for the node
surrounded by open and close tags.  If whitespace preceded the node, it writes
a space before the text.  There are three exceptions:

1. `MMD_TYPE_IMAGE`: An `<img>` tag is written using the URL as the source and
   the text as the alternate value.
2. `MMD_TYPE_HARD_BREAK`: A `<br>` tag is written.
3. `MMD_TYPE_SOFT_BREAK`: A `<wbr>` tag is written.

In addition, some simple text substitutions are performed for "(c)", "(r)", and
"(tm)" to use the corresponding HTML entities for copyright, registered
trademark, and trademark.

Here is the complete function:

    static void
    write_inline(mmd_t *node)
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
            element = "a";
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

        default :
            element = NULL;
            break;
      }

      if (url)
      {
        if (!strcmp(url, "@"))
          printf("<a href=\"#%s\">", make_anchor(text));
        else
          printf("<a href=\"%s\">", url);
      }
      else if (element)
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

# mmd\_t - Markdown Node

    typedef struct _mmd_s mmd_t;

The `mmd_t` object represents a single node within a Markdown document.  Each
node has an associated type and may have text, link, siblings, children, and
a parent.


# mmd\_type\_t - Markdown Node Type Enumeration

    typedef enum mmd_type_e
    {
      MMD_TYPE_NONE,
      MMD_TYPE_DOCUMENT,
      MMD_TYPE_METADATA,
      MMD_TYPE_BLOCK_QUOTE,
      MMD_TYPE_ORDERED_LIST,
      MMD_TYPE_UNORDERED_LIST,
      MMD_TYPE_LIST_ITEM,
      MMD_TYPE_HEADING_1,
      MMD_TYPE_HEADING_2,
      MMD_TYPE_HEADING_3,
      MMD_TYPE_HEADING_4,
      MMD_TYPE_HEADING_5,
      MMD_TYPE_HEADING_6,
      MMD_TYPE_PARAGRAPH,
      MMD_TYPE_CODE_BLOCK,
      MMD_TYPE_THEMATIC_BREAK,
      MMD_TYPE_NORMAL_TEXT,
      MMD_TYPE_EMPHASIZED_TEXT,
      MMD_TYPE_STRONG_TEXT,
      MMD_TYPE_STRUCK_TEXT,
      MMD_TYPE_LINKED_TEXT,
      MMD_TYPE_CODE_TEXT,
      MMD_TYPE_IMAGE,
      MMD_TYPE_HARD_BREAK,
      MMD_TYPE_SOFT_BREAK,
      MMD_TYPE_METADATA_TEXT
    } mmd_type_t;

The `mmd_type_t` enumeration represents all of the Markdown node types.


## mmdFree - Free a Markdown Document

    void
    mmdFree(mmd_t *node);

The `mmdFree` function frees the specified node and all of its children.  It is
typically only used to free the entire Markdown document, starting at the root
node.


## mmdGetFirstChild - Get the First Child of a Node

    mmd_t *
    mmdGetFirstChild(mmd_t *node);

The `mmdGetFirstChild` function returns the first child of the specified node,
if any.


## mmdGetLastChild - Get the Last Child of a Node

    mmd_t *
    mmdGetLastChild(mmd_t *node);

The `mmdGetLastChild` functions returns the last child of the specified node,
if any.


## mmdGetMetadata - Get Metadata for a Document

    const char *
    mmdGetMetadata(mmd_t *doc, const char *keyword);

The `mmdGetMetadata` function returns the document metadata for the specified
keyword.  Standard keywords include "author", "copyright", and "title".


## mmdGetNextSibling - Get the Next Sibling of a Node

    mmd_t *
    mmdGetNextSibling(mmd_t *node);

The `mmdGetNextSibling` function returns the next sibling of the specified node,
if any.


## mmdGetParent - Get the Parent of a Node

    mmd_t *
    mmdGetParent(mmd_t *node);

The `mmdGetParent` function returns the parent of the specified node, if any.


## mmdGetPrevSibling - Get the Previous Sibling of a Node

    mmd_t *
    mmdGetPrevSibling(mmd_t *node);

The `mmdGetPrevSibling` function returns the previous sibling of the specified
node, if any.


## mmdGetText - Get Text Associated with a Node

    const char *
    mmdGetText(mmd_t *node);

The `mmdGetText` function returns any text that is associated with the specified
node.


## mmdGetType - Get the Node Type

    mmd_type_t
    mmdGetType(mmd_t *node);

The `mmdGetType` function returns the type of the specified node.


## mmdGetURL - Get a URL Associated with a Node

    const char *
    mmdGetURL(mmd_t *node);

The `mmdGetURL` function returns any URL that is associated with the specified
node.


## mmdGetWhitespace - Get Whitespace Associated with a Node

    int
    mmdGetWhitespace(mmd_t *node);

The `mmdGetWhitespace` function returns whether whitespace preceded the
specified node.


## mmdIsBlock - Report Whether a Node is a Block

    int
    mmdIsBlock(mmd_t *node);

The `mmdIsBlock` function returns `1` when the specified node is a Markdown
block and `0` otherwise.


## mmdLoad - Load a Markdown Document

    mmd_t *
    mmdLoad(const char *filename);

The `mmdLoad` function loads a Markdown document from the specified file.  The
function understands the CommonMark syntax and Jekyll metadata.
