/*
 * Mini markdown utility.
 *
 *     https://github.com/michaelrsweet/mmd
 *
 * Usage:
 *
 *     mmdutil [options] filename.md [... filenameN.md]
 *
 * Options:
 *
 *    --cover filename.ext	Specify cover image.
 *    --css filename.css	Specify style sheet.
 *    --front filename.md	Specify frontmatter file.
 *    --help			Show usage.
 *    --man section		Produce man page output.
 *    --toc levels		Produce a table of contents.
 *    --version			Show version.
 *    -o filename.ext		Specify output file (default is stdout).
 *
 * Copyright © 2017-2018 by Michael R Sweet.
 *
 * Licensed under Apache License v2.0.	See the file "LICENSE" for more
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
#include <time.h>


/*
 * Local types...
 */

typedef enum
{
  FORMAT_HTML,				/* Output HTML */
  FORMAT_MAN				/* Output man page */
} format_t;

typedef struct toc_s
{
  int	level;				/* Heading level */
  char	*heading;			/* Heading text */
} toc_t;


/*
 * Local functions...
 */

static int		build_toc(mmd_t *parent, int toc_levels, int num_toc, toc_t **toc);

static const char	*html_anchor(const char *text);
static void		html_block(FILE *outfp, mmd_t *parent);
static void		html_head(FILE *outfp, const char *cssfile, const char *coverfile, const char *title, const char *copyright, const char *author, const char *version);
static void		html_leaf(FILE *outfp, mmd_t *node);
static void		html_puts(FILE *outfp, const char *s);
static void		html_toc(FILE *outfp, int num_toc, toc_t *toc);

static void		man_block(FILE *outfp, mmd_t *parent);
static void		man_head(FILE *outfp, int section, const char *title, const char *copyright, const char *author, const char *version);
static void		man_leaf(FILE *outfp, mmd_t *node);
static void		man_puts(FILE *outfp, const char *s, int allcaps);

static void		usage(void);


/*
 * 'main()' - Main entry for mini markdown utility.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int		i;			/* Looping var */
  const char	*opt,			/* Current option */
		*outfile = NULL;	/* Output filename */
  FILE		*outfp;			/* Output file */
  format_t	format = FORMAT_HTML;	/* Output format */
  int		section = 0;		/* Section number for man page output */
  const char	*coverfile = NULL,	/* Cover image filename */
		*cssfile = NULL,	/* CSS filename */
		*title = NULL,		/* Title */
		*copyright = NULL,	/* Copyright */
		*author = NULL,		/* Author */
		*version = NULL;	/* Document version */
  mmd_t		*front = NULL,		/* Cover page/frontmatter */
		*files[100];		/* "Body" files */
  int		num_files = 0,		/* Number of files */
		toc_levels = 0,		/* Number of table of contents levels */
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
      else if (!strcmp(argv[i], "--man"))
      {
	i ++;
	if (i >= argc || (section = atoi(argv[i])) <= 0)
	{
	  fputs("mmdbook: Missing/bad section number after '--man'.", stderr);
	  usage();
	  return (1);
	}

	format = FORMAT_MAN;
      }
      else if (!strcmp(argv[i], "--toc"))
      {
	i ++;
	if (i >= argc || (toc_levels = atoi(argv[i])) <= 0)
	{
	  fputs("mmdbook: Missing/bad levels number after '--toc'.", stderr);
	  usage();
	  return (1);
	}
      }
      else if (!strcmp(argv[i], "--version"))
      {
	puts(VERSION);
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

  if (toc_levels > 0)
  {
    for (i = 0; i < num_files; i ++)
      num_toc = build_toc(files[i], toc_levels, num_toc, &toc);
  }

 /*
  * Write everything...
  */

  switch (format)
  {
    case FORMAT_HTML :
	html_head(outfp, cssfile, coverfile, title, copyright, author, version);

	if (front)
	  html_block(outfp, front);

	if (num_toc)
	  html_toc(outfp, num_toc, toc);

	for (i = 0; i < num_files; i ++)
	  html_block(outfp, files[i]);

	fputs("	 </body>\n", outfp);
	fputs("</html>\n", outfp);
	break;

    case FORMAT_MAN :
	man_head(outfp, section, title, copyright, author, version);

	if (front)
	  man_block(outfp, front);

	for (i = 0; i < num_files; i ++)
	  man_block(outfp, files[i]);

	if (copyright)
	{
	  fputs(".SH COPYRIGHT\n", outfp);
	  man_puts(outfp, copyright, 0);
	  fputs("\n", outfp);
	}
	break;
  }

  if (outfp != stdout)
    fclose(outfp);

  return (0);
}


/*
 * 'build_toc()' - Scan for headings to include in the table of contents.
 */

static int				/* O  - Number of table of contents entries */
build_toc(mmd_t *parent,		/* I  - Parent node */
	  int	toc_levels,		/* Number of levels in table of contents */
	  int	num_toc,		/* I  - Number of table of contents entries */
	  toc_t **toc)			/* IO - Table of contents entries */
{
  mmd_t		*node,			/* Current node */
		*next;			/* Next node */
  mmd_type_t	type;			/* Node type */
  toc_t		*temp;			/* Table of contents entry */
  int		alloc_toc = num_toc;	/* Allocated entries */


  for (node = mmdGetFirstChild(parent); node; node = next)
  {
    type = mmdGetType(node);

    if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6 && (type - MMD_TYPE_HEADING_1) < toc_levels)
    {
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

      temp->heading = mmdCopyAllText(node);
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
 * 'html_anchor()' - Make an anchor for internal links.
 */

static const char *			/* O - Anchor string */
html_anchor(const char *text)		/* I - Text */
{
  char		*bufptr;		/* Pointer into buffer */
  static char	buffer[1024];		/* Buffer for anchor string */


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
 * 'html_block()' - Write a block node as HTML.
 */

static void
html_block(FILE	 *outfp,		/* I - Output file */
	   mmd_t *parent)		/* I - Parent node */
{
  const char	*element,		/* Enclosing element, if any */
		*hclass = NULL;		/* HTML class, if any */
  mmd_t		*node;			/* Current child node */
  mmd_type_t	type;			/* Node type */


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
	fputs("	   <pre>", outfp);
	for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
	{
	  fputs("<code>", outfp);
	  html_puts(outfp, mmdGetText(node));
	  fputs("</code>", outfp);
	}
	fputs("</pre>\n", outfp);
	return;

    case MMD_TYPE_THEMATIC_BREAK :
	fputs("	   <hr />\n", outfp);
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
	hclass	= "center";
	break;

    case MMD_TYPE_TABLE_BODY_CELL_RIGHT :
	element = "td";
	hclass	= "right";
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

    fprintf(outfp, "	<%s id=\"", element);
    for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
    {
      if (mmdGetWhitespace(node))
	putc('-', outfp);

      fputs(html_anchor(mmdGetText(node)), outfp);
    }
    fputs("\">", outfp);
  }
  else if (element)
    fprintf(outfp, "	<%s%s%s>%s", element, hclass ? " class=" : "", hclass ? hclass : "", type <= MMD_TYPE_UNORDERED_LIST ? "\n" : "");

  for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
  {
    if (mmdIsBlock(node))
      html_block(outfp, node);
    else
      html_leaf(outfp, node);
  }

  if (element)
    fprintf(outfp, "</%s>\n", element);
}


/*
 * 'html_head()' - Write HTML header.
 */

static void
html_head(FILE	     *outfp,		/* I - Output file */
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
  html_puts(outfp, title ? title : "Unknown");
  fputs("</title>\n", outfp);
  if (version)
  {
    fputs("    <meta name=\"version\" content=\"", outfp);
    html_puts(outfp, version);
    fputs("\">\n", outfp);
  }
  if (author)
  {
    fputs("    <meta name=\"author\" content=\"", outfp);
    html_puts(outfp, author);
    fputs("\">\n", outfp);
  }
  if (copyright)
  {
    fputs("    <meta name=\"copyright\" content=\"", outfp);
    html_puts(outfp, copyright);
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
    fputs("  margin: 54pt 36pt;\n", outfp);
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
    fputs("}\n", outfp);
    fputs("pre {\n", outfp);
    fputs("  background: #f8f8f8;\n", outfp);
    fputs("  border: solid thin #666;\n", outfp);
    fputs("  font-size: 14px;\n", outfp);
    fputs("  line-height: 120%;\n", outfp);
    fputs("  margin-left: 2em;\n", outfp);
    fputs("  padding: 10px;\n", outfp);
    fputs("  text-indent: -2em;\n", outfp);
    fputs("  white-space: pre-wrap;\n", outfp);
    fputs("}\n", outfp);
    fputs("li code, p code {\n", outfp);
    fputs("  padding: 2px 5px;\n", outfp);
    fputs("}\n", outfp);
    fputs("blockquote {\n", outfp);
    fputs("  background: #f8f8f8;\n", outfp);
    fputs("  border-left: solid 2px #666;\n", outfp);
    fputs("  margin: 1em 0;\n", outfp);
    fputs("  padding: 0.1em 1em;\n", outfp);
    fputs("}\n", outfp);
    fputs("table {\n", outfp);
    fputs("  border-collapse: collapse;\n", outfp);
    fputs("  border-spacing: 0;\n", outfp);
    fputs("}\n", outfp);
    fputs("td {\n", outfp);
    fputs("  border: solid 1px #666;\n", outfp);
    fputs("  padding: 5px 10px;\n", outfp);
    fputs("  vertical-align: top;\n", outfp);
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
    fputs("  border-bottom: solid 2px #000;\n", outfp);
    fputs("  padding: 1px 5px;\n", outfp);
    fputs("  text-align: center;\n", outfp);
    fputs("  vertical-align: bottom;\n", outfp);
    fputs("}\n", outfp);
    fputs("tr:nth-child(odd) td {\n", outfp);
    fputs("  background: #f8f8f8;\n", outfp);
    fputs("}\n", outfp);
  }
  fputs("--></style>\n", outfp);
  fputs("  </head>\n", outfp);
  fputs("  <body>\n", outfp);

  if (coverfile)
  {
    fputs("    <img src=\"", outfp);
    html_puts(outfp, coverfile);
    fputs("\">\n", outfp);
  }

  fputs("    <h1 class=\"title\">", outfp);
  html_puts(outfp, title ? title : "Unknown");
  fputs("</h1>\n", outfp);

  if (version)
  {
    fputs("    <p class=\"title\">Version ", outfp);
    html_puts(outfp, version);
    fputs("</p>\n", outfp);
  }
  if (author)
  {
    fputs("    <p class=\"title\">by ", outfp);
    html_puts(outfp, author);
    fputs("</p>\n", outfp);
  }
  if (copyright)
  {
    fputs("    <p class=\"title\">", outfp);
    html_puts(outfp, copyright);
    fputs("</p>\n", outfp);
  }
}


/*
 * 'html_leaf()' - Write a leaf node as HTML.
 */

static void
html_leaf(FILE	*outfp,			/* I - Output file */
	  mmd_t *node)			/* I - Leaf node */
{
  const char	*element,		/* Encoding element, if any */
		*text,			/* Text to write */
		*url;			/* URL to write */


  if (mmdGetWhitespace(node))
    putc(' ', outfp);

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
	html_puts(outfp, url);
	fputs("\" alt=\"", outfp);
	html_puts(outfp, text);
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
      fprintf(outfp,  "<a href=\"#%s\">", html_anchor(text));
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
    html_puts(outfp, text);

  if (element)
    fprintf(outfp, "</%s>", element);

  if (url)
    fputs("</a>", outfp);
}


/*
 * 'html_puts()' - Write text string as safe HTML.
 */

static void
html_puts(FILE	     *outfp,		/* I - Output file */
	  const char *text)		/* I - Text string */
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
      putc(*text, outfp);

    text ++;
  }
}


/*
 * 'html_toc()' - Write the table-of-contents.
 */

static void
html_toc(FILE  *outfp,			/* I - Output file */
	 int   num_toc,			/* I - Number of table of contents entries */
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

    fprintf(outfp, "%*s<li class=\"toc\"><a href=\"#%s\">", level * 2 + 4, "", html_anchor(toc->heading));
    html_puts(outfp, toc->heading);

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
      fputs("	 </ul>\n", outfp);
  }
}


/*
 * 'man_block()' - Write a block node as man page source.
 */

static void
man_block(FILE	 *outfp,		/* I - Output file */
	  mmd_t *parent)		/* I - Parent node */
{
  mmd_t		*node;			/* Current child node */
  mmd_type_t	type;			/* Node type */


  type = mmdGetType(parent);

  switch (type)
  {
    case MMD_TYPE_BLOCK_QUOTE :
	break;

    case MMD_TYPE_ORDERED_LIST :
	break;

    case MMD_TYPE_UNORDERED_LIST :
	break;

    case MMD_TYPE_LIST_ITEM :
	fputs(".IP \\(bu 5\n", outfp);
	break;

    case MMD_TYPE_HEADING_1 :
	fputs(".SH ", outfp);
	break;

    case MMD_TYPE_HEADING_2 :
	fputs(".SS ", outfp);
	break;

    case MMD_TYPE_HEADING_3 :
    case MMD_TYPE_HEADING_4 :
    case MMD_TYPE_HEADING_5 :
    case MMD_TYPE_HEADING_6 :
    case MMD_TYPE_PARAGRAPH :
	fputs(".PP\n", outfp);
	break;

    case MMD_TYPE_CODE_BLOCK :
	fputs(".nf\n\n", outfp);
	for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
	{
	  fputs("    ", outfp);
	  man_puts(outfp, mmdGetText(node), 0);
	}
	fputs(".fi\n", outfp);
	return;

    case MMD_TYPE_METADATA :
	return;

    case MMD_TYPE_TABLE : /* No table support for man output at present */
	fputs(".PP\n", outfp);
	fputs("[Table Omitted]\n", outfp);
        return;

    default :
	break;
  }

  for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
  {
    if (mmdIsBlock(node))
      man_block(outfp, node);
    else
      man_leaf(outfp, node);
  }

  fputs("\n", outfp);

  if (type == MMD_TYPE_TABLE)
    fputs(".TE\n", outfp);
}


/*
 * 'man_head()' - Write man page header.
 */

static void
man_head(FILE	     *outfp,		/* I - Output file */
	 int        section,		/* I - Man page section */
	 const char *title,		/* I - Title of book, if any */
	 const char *copyright,		/* I - Copyright, if any */
	 const char *author,		/* I - Author, if any */
	 const char *version)		/* I - Version of book, if any */
{
  time_t	curtime;		/* Current time */
  struct tm	*curdate;		/* Current date */
  const char	*source_date_epoch;	/* SOURCE_DATE_EPOCH environment variable */


  fprintf(outfp, ".\" Man page for %s version %s.\n", title ? title : "unknown", version ? version : "unknown");

  if (copyright)
    fprintf(outfp, ".\" %s\n", copyright);

  if ((source_date_epoch = getenv("SOURCE_DATE_EPOCH")) != NULL)
    curtime = (time_t)strtol(source_date_epoch, NULL, 10);
  else
    curtime = time(NULL);

  curdate = localtime(&curtime);

  fprintf(outfp, ".TH \"%s\" %d \"%04d-%02d-%02d\" \"%s\"\n", title ? title : "unknown", section, curdate->tm_year + 1900, curdate->tm_mon + 1, curdate->tm_mday, author ? author : "Unknown");
}


/*
 * 'man_leaf()' - Write a leaf node as man page source.
 */

static void
man_leaf(FILE  *outfp,			/* I - Output file */
         mmd_t *node)			/* I - Leaf node */
{
  mmd_type_t	ptype;			/* Parent node type */
  const char    *text,                  /* Text to write */
		*suffix = NULL;		/* Trailing string */


  text = mmdGetText(node);

  switch (mmdGetType(node))
  {
    case MMD_TYPE_EMPHASIZED_TEXT :
	if (mmdGetWhitespace(node))
	  putc('\n', outfp);

	fputs(".I ", outfp);
	suffix = "\n";
	break;

    case MMD_TYPE_STRONG_TEXT :
	if (mmdGetWhitespace(node))
	  putc('\n', outfp);

	fputs(".B ", outfp);
	suffix = "\n";
	break;

    case MMD_TYPE_HARD_BREAK :
	if (mmdGetWhitespace(node))
	  putc('\n', outfp);

	fputs(".PP\n", outfp);
	return;

    case MMD_TYPE_SOFT_BREAK :
    case MMD_TYPE_METADATA_TEXT :
	return;

    default :
        ptype = mmdGetType(mmdGetPrevSibling(node));
	if (mmdGetWhitespace(node) && ptype != MMD_TYPE_EMPHASIZED_TEXT && ptype != MMD_TYPE_STRONG_TEXT && ptype != MMD_TYPE_HARD_BREAK)
	  putc(' ', outfp);
	break;
  }

  ptype = mmdGetType(mmdGetParent(node));

  man_puts(outfp, text, ptype >= MMD_TYPE_HEADING_1 && ptype <= MMD_TYPE_HEADING_6);

  if (suffix)
    fputs(suffix, outfp);
}


/*
 * 'man_puts()' - Write a string as safe man page source.
 */

static void
man_puts(FILE       *outfp,		/* I - Output file */
         const char *s,			/* I - Text string */
         int        allcaps)		/* I - Output in all caps? */
{
  int	ch;				/* Character */


  while (*s)
  {
    if ((*s & 0xe0) == 0xc0 && (s[1] & 0x80))
    {
     /*
      * 2-byte UTF-8...
      */

      ch = ((*s & 0x1f) << 6) | (s[1] & 0x3f);
      s += 2;

      fprintf(outfp, "\\[u%04X]", ch);
    }
    else if ((*s & 0xf0) == 0xe0 && (s[1] & 0x80) && (s[2] & 0x80))
    {
     /*
      * 3-byte UTF-8...
      */

      ch = ((((*s & 0x1f) << 6) | (s[1] & 0x3f)) << 6) | (s[2] & 0x3f);
      s += 3;

      fprintf(outfp, "\\[u%04X]", ch);
    }
    else if ((*s & 0xf8) == 0xf0 && (s[1] & 0x80) && (s[2] & 0x80) && (s[3] & 0x80))
    {
     /*
      * 4-byte UTF-8...
      */

      ch = ((((((*s & 0x1f) << 6) | (s[1] & 0x3f)) << 6) | (s[2] & 0x3f)) << 6) | (s[3] & 0x3f);
      s += 4;

      fprintf(outfp, "\\[u%04X]", ch);
    }
    else
    {
      if (*s == '\\' || *s == '-')
	putc('\\', outfp);

      if (allcaps)
        putc(toupper(*s++), outfp);
      else
	putc(*s++, outfp);
    }
  }
}


/*
 * 'usage()' - Show program usage.
 */

static void
usage(void)
{
  puts("Usage: mmdutil [options] filename.md [... filenameN.md]");
  puts("Options:");
  puts("  --cover filename.jpg	      Specify cover image.");
  puts("  --css filename.css	      Specify style sheet.");
  puts("  --front filename.md	      Specify frontmatter file.");
  puts("  --help		      Show usage.");
  puts("  --man section		      Produce man page output.");
  puts("  --toc levels		      Produce a table of contents.");
  puts("  --version		      Show version.");
  puts("  -o filename.html	      Specify output filename.");
}
