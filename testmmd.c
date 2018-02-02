/*
 * Unit test program for Mini Markdown library.
 *
 *     https://github.com/michaelrsweet/mmd
 *
 * Usage:
 *
 *     ./testmmd filename.md
 *
 * Copyright © 2017-2018 by Michael R Sweet.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more
 * information.
 */

/*
 * Include necessary headers...
 */

#include "mmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/*
 * Local functions...
 */

static const char *make_anchor(const char *text);
static void       write_block(mmd_t *parent);
static void       write_html(const char *s);
static void       write_leaf(mmd_t *node);


/*
 * 'main()' - Main entry for unit test program.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  mmd_t         *doc;                   /* Document */
  const char    *title;                 /* Title */


  if (argc != 2)
  {
    puts("Usage: ./testmmd filename.md > filename.html");
    return (1);
  }

  if ((doc = mmdLoad(argv[1])) == NULL)
  {
    perror(argv[1]);
    return (1);
  }

  title = mmdGetMetadata(doc, "title");

  puts("<!DOCTYPE html>");
  puts("<html");
  puts("  <head>");
  fputs("    <title>", stdout);
  write_html(title ? title : "Unknown");
  puts("</title>");
  puts("  <style><!--");
  puts("body {");
  puts("  font-family: sans-serif;");
  puts("  font-size: 18px;");
  puts("  line-height: 150%;");
  puts("}");
  puts("a {");
  puts("  font: inherit;");
  puts("}");
  puts("pre, li code, p code {");
  puts("  font-family: monospace;");
  puts("  font-size: 14px;");
  puts("}");
  puts("pre {");
  puts("  background: #f8f8f8;");
  puts("  border: solid thin #666;");
  puts("  line-height: 120%;");
  puts("  padding: 10px;");
  puts("}");
  puts("li code, p code {");
  puts("  padding: 2px 5px;");
  puts("}");
  puts("table {");
  puts("  border: solid thin #999;");
  puts("  border-collapse: collapse;");
  puts("  border-spacing: 0;");
  puts("}");
  puts("td {");
  puts("  border: solid thin #ccc;");
  puts("  padding-top: 5px;");
  puts("}");
  puts("td.left {");
  puts("  text-align: left;");
  puts("}");
  puts("td.center {");
  puts("  text-align: center;");
  puts("}");
  puts("td.right {");
  puts("  text-align: right;");
  puts("}");
  puts("th {");
  puts("  background: #ccc;");
  puts("  border: none;");
  puts("  border-bottom: solid thin #999;");
  puts("  padding: 1px 5px;");
  puts("  text-align: center;");
  puts("}");
  puts("--></style>");
  puts("  </head>");
  puts("  <body>");

  write_block(doc);

  puts("  </body>");
  puts("</html>");

  mmdFree(doc);

  return (0);
}


/*
 * 'make_anchor()' - Make an anchor for internal links.
 */

static const char *                     /* O - Anchor string */
make_anchor(const char *text)           /* I - Text */
{
  char          *bufptr;                /* Pointer into buffer */
  static char   buffer[1024];           /* Buffer for anchor string */


  for (bufptr = buffer; *text && bufptr < (buffer + sizeof(buffer) - 1); text ++)
  {
    if ((*text >= '0' && *text <= '9') || (*text >= 'a' && *text <= 'z') || (*text >= 'A' && *text <= 'Z') || *text == '.' || *text == '-')
      *bufptr++ = tolower(*text);
    else if (*text == ' ')
      *bufptr++ = '-';
  }

  *bufptr = '\0';

  return (buffer);
}


/*
 * 'write_block()' - Write a block node as HTML.
 */

static void
write_block(mmd_t *parent)              /* I - Parent node */
{
  const char    *element,               /* Enclosing element, if any */
		*hclass = NULL;		/* HTML class, if any */
  mmd_t         *node;                  /* Current child node */
  mmd_type_t    type;                   /* Node type */


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

    case MMD_TYPE_TABLE_CELL_LEFT :
        element = "td";
        break;

    case MMD_TYPE_TABLE_CELL_CENTER :
        element = "td";
        hclass  = "center";
        break;

    case MMD_TYPE_TABLE_CELL_RIGHT :
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
        fputc('-', stdout);

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
      write_leaf(node);
  }

  if (element)
    printf("</%s>\n", element);
}


/*
 * 'write_html()' - Write text to stdout as HTML.
 */

static void
write_html(const char *text)            /* I - Text string */
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


/*
 * 'write_leaf()' - Write a leaf node as HTML.
 */

static void
write_leaf(mmd_t *node)                 /* I - Leaf node */
{
  const char    *element,               /* Encoding element, if any */
                *text,                  /* Text to write */
                *url;                   /* URL to write */


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
    fputs("</a>", stdout);
}
