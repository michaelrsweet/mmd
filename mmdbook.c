/*
 * Mini Markdown book maker.
 *
 *     https://github.com/michaelrsweet/mmd
 *
 * Usage:
 *
 *     ./mmdbook [options] filename.md [... filenameN.md]
 *
 * Options:
 *
 *    --cover filename.jpg	Specify cover image.
 *    --css filename.css	Specify style sheet.
 *    --front filename.md	Specify frontmatter file.
 *    --help			Show usage.
 *    --version			Show version.
 *    -o filename.html		Specify output file.
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
#include <errno.h>


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
static void		usage(void);
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
  int		i;			/* Looping var */
  const char	*opt;			/* Current option */
  FILE		*outfp;			/* Output file */
  const char	*outfile = NULL,	/* Output filename */
		*coverfile = NULL,	/* Cover image filename */
		*cssfile = NULL,	/* CSS filename */
		*title = NULL,		/* Title */
		*copyright = NULL,	/* Copyright */
		*author = NULL,		/* Author */
		*version = NULL;	/* Document version */
  mmd_t         *front = NULL,		/* Cover page/frontmatter */
		*files[100];		/* "Body" files */
  int		num_files = 0,		/* Number of files */
		num_toc = 0;		/* Number of table of contents entries */
  toc_t		*toc = NULL;		/* Table of contents entries */


 /*
  * Process command-line arguments...
  */

  for (i = 1; i < argc; i ++)
  {
    if (!strncmp(argv[i], "--", 2))
    {
      if (!strcmp(argv[i], "--cover"))
      {
        i ++;
        if (i >= argc)
        {
          fputs("mmdbook: Missing cover image filename after '--cover'.", stderr);
          usage();
          return (1);
        }

        coverfile = argv[i];
      }
      else if (!strcmp(argv[i], "--css"))
      {
        i ++;
        if (i >= argc)
        {
          fputs("mmdbook: Missing cover image filename after '--css'.", stderr);
          usage();
          return (1);
        }

        cssfile = argv[i];
      }
      else if (!strcmp(argv[i], "--front"))
      {
        i ++;
        if (i >= argc)
        {
          fputs("mmdbook: Missing frontmatter filename after '--front'.", stderr);
          usage();
          return (1);
        }

        if ((front = mmdLoad(argv[i])) == NULL)
        {
          fprintf(stderr, "mmdbook: Unable to load \"%s\": %s\n", argv[i], strerror(errno));
          return (1);
        }

        if (!title)
	  title = mmdGetMetadata(front, "title");
        if (!author)
	  author = mmdGetMetadata(front, "author");
        if (!copyright)
	  copyright = mmdGetMetadata(front, "copyright");
        if (!version)
	  version = mmdGetMetadata(front, "version");
      }
      else if (!strcmp(argv[i], "--help"))
      {
        usage();
        return (0);
      }
      if (!strcmp(argv[i], "--help"))
      {
        usage();
        return (0);
      }
      else if (!strcmp(argv[i], "--version"))
      {
        puts("1.2");
        return (0);
      }
      else
      {
        fprintf(stderr, "mmdbook: Unknown option '%s'.\n", argv[i]);
        usage();
        return (1);
      }
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
        switch (*opt)
        {
          case 'o' :
              i ++;
              if (i >= argc)
              {
                fputs("mmdbook: Missing output filename after '-o'.\n", stderr);
                usage();
                return (1);
              }

              outfile = argv[i];
              break;

          default :
              fprintf(stderr, "mmdbook: Unknown option '-%c'.\n", *opt);
              usage();
              return (1);
        }
      }
    }
    else if (num_files < (int)(sizeof(files) / sizeof(files[0])))
    {
      if ((files[num_files] = mmdLoad(argv[i])) == NULL)
      {
	fprintf(stderr, "mmdbook: Unable to load \"%s\": %s\n", argv[i], strerror(errno));
	return (1);
      }

      if (!title)
	title = mmdGetMetadata(files[num_files], "title");
      if (!author)
	author = mmdGetMetadata(files[num_files], "author");
      if (!copyright)
	copyright = mmdGetMetadata(files[num_files], "copyright");
      if (!version)
	version = mmdGetMetadata(files[num_files], "version");

      num_files ++;
    }
    else
    {
      fputs("mmdbook: Too many input files.\n", stderr);
      return (1);
    }
  }

  if (num_files == 0)
  {
    usage();
    return (1);
  }

  if (outfile)
  {
    if ((outfp = fopen(outfile, "w")) == NULL)
    {
      fprintf(stderr, "mmdbook: Unable to create \"%s\": %s\n", outfile, strerror(errno));
      return (1);
    }
  }
  else
    outfp = stdout;

 /*
  * Generate a table of contents...
  */

  for (i = 0; i < num_files; i ++)
    num_toc = scan_toc(files[i], num_toc, &toc);

 /*
  * Write everything...
  */

  write_head(outfp, cssfile, coverfile, title, copyright, author, version);

  if (front)
    write_block(outfp, front);

  if (num_toc)
    write_toc(outfp, num_toc, toc);

  for (i = 0; i < num_files; i ++)
    write_block(outfp, files[i]);

  fputs("  </body>\n", outfp);
  fputs("</html>\n", outfp);

  if (outfp != stdout)
    fclose(outfp);

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
  mmd_t		*node,			/* Current node */
		*tnode,			/* Title node */
		*next;			/* Next node */
  mmd_type_t	type;			/* Node type */
  char		heading[1024],		/* Heading text */
		*ptr;			/* Pointer into title */
  toc_t		*temp;			/* Table of contents entry */
  int		alloc_toc = num_toc;	/* Allocated entries */


  for (node = mmdGetFirstChild(parent); node; node = next)
  {
    type = mmdGetType(node);

    if (type == MMD_TYPE_HEADING_1 || type == MMD_TYPE_HEADING_2)
    {
      heading[sizeof(heading) - 1] = '\0';

      for (tnode = mmdGetFirstChild(node), ptr = heading; tnode; tnode = mmdGetNextSibling(tnode))
      {
	if (mmdGetWhitespace(tnode) && ptr < (heading + sizeof(heading) - 1))
	  *ptr++ = ' ';

	strncpy(ptr, mmdGetText(tnode), sizeof(heading) - (ptr - heading) - 1);
	ptr += strlen(ptr);
      }

      if (num_toc >= alloc_toc)
      {
        alloc_toc += 10;
        if (alloc_toc == 10)
          temp = malloc(alloc_toc * sizeof(toc_t));
        else
          temp = realloc(*toc, alloc_toc * sizeof(toc_t));

        if (!temp)
        {
          fputs("mmdbook: Unable to allocate memory for table of contents.\n", stderr);
          exit(1);
        }

        *toc = temp;
      }

      temp = *toc + num_toc;
      num_toc ++;

      temp->heading = strdup(heading);
      temp->level   = type - MMD_TYPE_HEADING_1 + 1;
    }

    if ((next = mmdGetNextSibling(node)) == NULL)
    {
      next = mmdGetParent(node);

      while (next && mmdGetNextSibling(next) == NULL)
	next = mmdGetParent(next);

      next = mmdGetNextSibling(next);
    }
  }

  return (num_toc);
}


/*
 * 'usage()' - Show program usage.
 */

static void
usage(void)
{
  puts("Usage: mmdbook [options] filename.md [... filenameN.md]");
  puts("Options:");
  puts("  --cover filename.jpg        Specify cover image.");
  puts("  --css filename.css          Specify style sheet.");
  puts("  --front filename.md         Specify frontmatter file.");
  puts("  --help                      Show usage.");
  puts("  --version                   Show version.");
  puts("  -o filename.html            Specify output filename.");
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
  FILE *cssfp;				/* CSS file */


  fputs("<!DOCTYPE html>\n", outfp);
  fputs("<html>\n", outfp);
  fputs("  <head>\n", outfp);
  fputs("    <title>", outfp);
  write_html(outfp, title ? title : "Unknown");
  fputs("</title>\n", outfp);
  if (version)
  {
    fputs("    <meta name=\"version\" content=\"", outfp);
    write_html(outfp, version);
    fputs("\">\n", outfp);
  }
  if (author)
  {
    fputs("    <meta name=\"author\" content=\"", outfp);
    write_html(outfp, author);
    fputs("\">\n", outfp);
  }
  if (copyright)
  {
    fputs("    <meta name=\"copyright\" content=\"", outfp);
    write_html(outfp, copyright);
    fputs("\">\n", outfp);
  }
  fputs("    <style><!--\n", outfp);
  if (cssfile)
  {
    if ((cssfp = fopen(cssfile, "r")) != NULL)
    {
      char line[1024];			/* Line from file */

      while (fgets(line, sizeof(line), cssfp))
	fputs(line, outfp);

      fclose(cssfp);
    }
    else
    {
      fprintf(stderr, "mmdbook: Unable to open \"%s\": %s\n", cssfile, strerror(errno));
      exit(1);
    }
  }
  else
  {
    fputs("body {\n", outfp);
    fputs("  font-family: sans-serif;\n", outfp);
    fputs("  font-size: 18px;\n", outfp);
    fputs("  line-height: 150%;\n", outfp);
    fputs("}\n", outfp);
    fputs("h1 {\n", outfp);
    fputs("  page-break-before: always;\n", outfp);
    fputs("}\n", outfp);
    fputs(".title {\n", outfp);
    fputs("  text-align: center;\n", outfp);
    fputs("}\n", outfp);
    fputs(".toc {\n", outfp);
    fputs("  list-style-type: none;\n", outfp);
    fputs("}\n", outfp);
    fputs("a {\n", outfp);
    fputs("  font: inherit;\n", outfp);
    fputs("}\n", outfp);
    fputs("pre, li code, p code {\n", outfp);
    fputs("  font-family: monospace;\n", outfp);
    fputs("  font-size: 14px;\n", outfp);
    fputs("}\n", outfp);
    fputs("pre {\n", outfp);
    fputs("  background: #f8f8f8;\n", outfp);
    fputs("  border: solid thin #666;\n", outfp);
    fputs("  line-height: 120%;\n", outfp);
    fputs("  padding: 10px;\n", outfp);
    fputs("}\n", outfp);
    fputs("li code, p code {\n", outfp);
    fputs("  padding: 2px 5px;\n", outfp);
    fputs("}\n", outfp);
    fputs("table {\n", outfp);
    fputs("  border: solid thin #999;\n", outfp);
    fputs("  border-collapse: collapse;\n", outfp);
    fputs("  border-spacing: 0;\n", outfp);
    fputs("}\n", outfp);
    fputs("td {\n", outfp);
    fputs("  border: solid thin #ccc;\n", outfp);
    fputs("  padding-top: 5px;\n", outfp);
    fputs("}\n", outfp);
    fputs("td.left {\n", outfp);
    fputs("  text-align: left;\n", outfp);
    fputs("}\n", outfp);
    fputs("td.center {\n", outfp);
    fputs("  text-align: center;\n", outfp);
    fputs("}\n", outfp);
    fputs("td.right {\n", outfp);
    fputs("  text-align: right;\n", outfp);
    fputs("}\n", outfp);
    fputs("th {\n", outfp);
    fputs("  background: #ccc;\n", outfp);
    fputs("  border: none;\n", outfp);
    fputs("  border-bottom: solid thin #999;\n", outfp);
    fputs("  padding: 1px 5px;\n", outfp);
    fputs("  text-align: center;\n", outfp);
    fputs("}\n", outfp);
  }
  fputs("--></style>\n", outfp);
  fputs("  </head>\n", outfp);
  fputs("  <body>\n", outfp);

  if (coverfile)
  {
    fputs("    <img src=\"", outfp);
    write_html(outfp, coverfile);
    fputs("\">\n", outfp);
  }

  fputs("    <h1 class=\"title\">", outfp);
  write_html(outfp, title ? title : "Unknown");
  fputs("</h1>\n", outfp);

  if (version)
  {
    fputs("    <p class=\"title\">Version ", outfp);
    write_html(outfp, version);
    fputs("</p>\n", outfp);
  }
  if (author)
  {
    fputs("    <p class=\"title\">by ", outfp);
    write_html(outfp, author);
    fputs("</p>\n", outfp);
  }
  if (copyright)
  {
    fputs("    <p class=\"title\">", outfp);
    write_html(outfp, copyright);
    fputs("</p>\n", outfp);
  }
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
  int	level = 1;			/* Current indentation level */


  fputs("    <h1 class=\"title\">Table of Contents</h1>\n", outfp);
  fputs("    <ul>\n", outfp);

  while (num_toc > 0)
  {
    while (level > toc->level)
    {
      level --;

      fprintf(outfp, "%*s</ul></li>\n", level * 2 + 4, "");
    }

    fprintf(outfp, "%*s<li class=\"toc\"><a href=\"#%s\">", level * 2 + 4, "", make_anchor(toc->heading));
    write_html(outfp, toc->heading);

    num_toc --;
    toc ++;

    if (num_toc == 0 || toc->level == level)
    {
      fputs("</a></li>\n", outfp);
    }
    else
    {
      level ++;
      fputs("</a><ul>\n", outfp);

      while (level < toc->level)
      {
	level ++;
	fprintf(outfp, "%*s<li><ul>\n", level * 2 + 4, "");
      }
    }
  }

  while (level > 0)
  {
    level --;

    if (level > 0)
      fprintf(outfp, "%*s</ul></li>\n", level * 2 + 4, "");
    else
      fputs("    </ul>\n", outfp);
  }
}
