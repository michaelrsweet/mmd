/*
 * Mini Markdown book maker.
 *
 *     https://github.com/michaelrsweet/mmd
 *
 * Usage:
 *
 *     ./mmdbook [-o filename.html] filename.md [... filenameN.md]
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
 * Local types...
 */

typedef struct toc_s
{
  int	level;				/* Heading level */
  char	*heading;			/* Heading text */
} toc_t;


/*
 * Local functions...
 */

static const char	*make_anchor(const char *text);
static int		scan_toc(mmd_t *parent, int num_toc, toc_t **toc);
static void		write_block(FILE *outfp, mmd_t *parent);
static void		write_head(FILE *outfp, const char *cssfile, const char *coverfile, const char *title, const char *copyright, const char *author, const char *version);
static void		write_html(FILE *outfp, const char *s);
static void		write_leaf(FILE *outfp, mmd_t *node);
static void		write_toc(FILE *outfp, int num_toc, toc_t *toc);


/*
 * 'main()' - Main entry for unit test program.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  FILE		*outfp;			/* Output file */
  const char	*outfile = NULL,	/* Output filename */
		*coverfile = NULL,	/* Cover image filename */
		*cssfile = NULL,	/* CSS filename */
		*title,                 /* Title */
		*copyright,		/* Copyright */
		*author,		/* Author */
		*version;		/* Document version */
  mmd_t         *front = NULL,		/* Cover page/frontmatter */
		*files[100];		/* "Body" files */
  int		num_files = 0,		/* Number of files */
		num_toc = 0;		/* Number of table of contents entries */
  toc_t		*toc = NULL;		/* Table of contents entries */


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
 * 'scan_toc()' - Scan for headings to include in the table of contents.
 */

static int				/* O  - Number of table of contents entries */
scan_toc(mmd_t *parent,			/* I  - Parent node */
         int   num_toc,			/* I  - Number of table of contents entries */
         toc_t **toc)			/* IO - Table of contents entries */
{
  mmd_t	*node;				/* Current node */
  toc_t	*temp;				/* Table of contents entry */


}


/*
 * 'write_block()' - Write a block node as HTML.
 */

static void
write_block(FILE  *outfp,		/* I - Output file */
            mmd_t *parent)              /* I - Parent node */
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
        fputs("    <pre><code>", outfp);
        for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
          write_html(outfp, mmdGetText(node));
        fputs("</code></pre>\n", outfp);
        return;

    case MMD_TYPE_THEMATIC_BREAK :
        fputs("    <hr />\n", outfp);
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

    fprintf(outfp, "    <%s id=\"", element);
    for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
    {
      if (mmdGetWhitespace(node))
        fputc('-', outfp);

      fputs(make_anchor(mmdGetText(node)), outfp);
    }
    fputs("\">", outfp);
  }
  else if (element)
    fprintf(outfp, "    <%s%s%s>%s", element, hclass ? " class=" : "", hclass ? hclass : "", type <= MMD_TYPE_UNORDERED_LIST ? "\n" : "");

  for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
  {
    if (mmdIsBlock(node))
      write_block(outfp, node);
    else
      write_leaf(outfp, node);
  }

  if (element)
    fprintf(outfp, "</%s>\n", element);
}


/*
 * 'write_head()' - Write HTML header.
 */

static void
write_head(FILE       *outfp,		/* I - Output file */
           const char *cssfile,		/* I - CSS file, if any */
           const char *coverfile,	/* I - Cover image, if any */
           const char *title,		/* I - Title of book, if any */
           const char *copyright,	/* I - Copyright, if any */
           const char *author,		/* I - Author, if any */
           const char *version)		/* I - Version of book, if any */
{
  puts("<!DOCTYPE html>");
  puts("<html>");
  puts("  <head>");
  fputs("    <title>", stdout);
  write_html(outfp, title ? title : "Unknown");
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
}


/*
 * 'write_html()' - Write text to stdout as HTML.
 */

static void
write_html(FILE       *outfp,		/* I - Output file */
           const char *text)            /* I - Text string */
{
  if (!text)
    return;

  while (*text)
  {
    if (*text == '&')
      fputs("&amp;", outfp);
    else if (*text == '<')
      fputs("&lt;", outfp);
    else if (*text == '>')
      fputs("&gt;", outfp);
    else if (*text == '\"')
      fputs("&quot;", outfp);
    else
      fputc(*text, outfp);

    text ++;
  }
}


/*
 * 'write_leaf()' - Write a leaf node as HTML.
 */

static void
write_leaf(FILE  *outfp,		/* I - Output file */
           mmd_t *node)                 /* I - Leaf node */
{
  const char    *element,               /* Encoding element, if any */
                *text,                  /* Text to write */
                *url;                   /* URL to write */


  if (mmdGetWhitespace(node))
    fputc(' ', outfp);

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
        fputs("<img src=\"", outfp);
        write_html(outfp, url);
        fputs("\" alt=\"", outfp);
        write_html(outfp, text);
        fputs("\" />", outfp);
        return;

    case MMD_TYPE_HARD_BREAK :
        fputs("<br />\n", outfp);
        return;

    case MMD_TYPE_SOFT_BREAK :
        fputs("<wbr />\n", outfp);
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
      fprintf(outfp,  "<a href=\"#%s\">", make_anchor(text));
    else
      fprintf(outfp, "<a href=\"%s\">", url);
  }

  if (element)
    fprintf(outfp, "<%s>", element);

  if (!strcmp(text, "(c)"))
    fputs("&copy;", outfp);
  else if (!strcmp(text, "(r)"))
    fputs("&reg;", outfp);
  else if (!strcmp(text, "(tm)"))
    fputs("&trade;", outfp);
  else
    write_html(outfp, text);

  if (element)
    fprintf(outfp, "</%s>", element);

  if (url)
    fputs("</a>", outfp);
}


/*
 * 'write_toc()' - Write the table-of-contents.
 */

static void
write_toc(FILE  *outfp,			/* I - Output file */
          int   num_toc,		/* I - Number of table of contents entries */
          toc_t *toc)			/* I - Table of contents entries */
{
}
