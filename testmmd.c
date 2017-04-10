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

static void     write_block(mmd_t *parent);
static void     write_inline(mmd_t *node);


/*
 * 'main()' - Main entry for unit test program.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  mmd_t *doc;                           /* Document */


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

  puts("<!DOCTYPE html>");
  puts("<html");
  puts("  <head>");
  puts("    <title>Test Document</title>");
  puts("  </head>");
  puts("  <body>");

  write_block(doc);

  puts("  </body>");
  puts("</html>");

  mmdFree(doc);

  return (0);
}


/*
 * 'write_block()' - Write a block as HTML.
 */

static void
write_block(mmd_t *parent)              /* I - Parent node */
{
  const char    *element;               /* Enclosing element, if any */
  mmd_t         *node;                  /* Current child node */


  switch (mmdGetType(parent))
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
        element = "pre";
        break;

    case MMD_TYPE_THEMATIC_BREAK :
        puts("<hr />");
        return;

    default :
        element = NULL;
        break;
  }

  if (element)
    printf("    <%s>%s", element, mmdGetType(parent) <= MMD_TYPE_UNORDERED_LIST ? "\n" : "");

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


/*
 * 'write_inline()' - Write an inline node as HTML.
 */

static void
write_inline(mmd_t *node)               /* I - Inline node */
{
  const char    *element,               /* Encoding element, if any */
                *text,                  /* Text to write */
                *url;                   /* URL to write */


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

    case MMD_TYPE_HARD_BREAK :
        puts("<br />");
        return;

    default :
        element = NULL;
        break;
  }

  if (mmdGetWhitespace(node))
    putchar(' ');

  text = mmdGetText(node);
  url  = mmdGetURL(node);

  if (url)
    printf("<a href=\"%s\">", url);
  else if (element)
    printf("<%s>", element);

  while (*text)
  {
    if (*text == '&')
    {
     /*
      * See if this is an HTML entity...
      */

      if ((isalpha(text[1] & 255) || text[1] == '#') && strchr(text, ';'))
      {
       /*
        * Yes, copy it over...
        */

        while (*text != ';')
          putchar(*text++);

        putchar(';');
      }
      else
      {
       /*
        * No, just escape this ampersand...
        */

        fputs("&amp;", stdout);
      }
    }
    else if (*text == '<')
      fputs("&lt;", stdout);
    else
      putchar(*text);

    text ++;
  }

  if (element)
    printf("</%s>", element);
}
