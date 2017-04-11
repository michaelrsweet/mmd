---
title: How to Use the mmd "Library"
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
