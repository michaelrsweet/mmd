/*
 * Unit test program for Mini Markdown library.
 *
 *     https://github.com/michaelrsweet/mmd
 *
 * Usage:
 *
 *     ./testmmd filename.md
 *
 * Copyright 2017 by Michael R Sweet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  puts("  background: #f8f8f8;");
  puts("  border: solid thin #666;");
  puts("  font-family: monospace;");
  puts("  font-size: 14px;");
  puts("}");
  puts("pre {");
  puts("  line-height: 120%;");
  puts("  margin-left: 1em;");
  puts("  margin-right: 1em;");
  puts("  padding: 10px;");
  puts("}");
  puts("li code, p code {");
  puts("  padding: 2px 5px;");
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
      *bufptr++ = *text;
  }

  *bufptr = '\0';

  return (buffer);
}


/*
 * 'write_block()' - Write a block as HTML.
 */

static void
write_block(mmd_t *parent)              /* I - Parent node */
{
  const char    *element;               /* Enclosing element, if any */
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
      write_leaf(node);
  }

  if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6)
    printf("</a></%s>\n", element);
  else if (element)
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
 * 'write_leaf()' - Write an leaf node as HTML.
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
