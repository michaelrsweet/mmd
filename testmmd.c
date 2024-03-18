//
// Unit test program for Mini Markdown library.
//
//     https://www.msweet.org/mmd
//
// Usage:
//
//     ./testmmd [--ext {all,none}] [--help] [--only-body] [--spec]
//               [-o filename.html] filename.md
//
// Copyright © 2017-2024 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "mmd.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


//
// Local globals...
//

static bool		spec_mode = false;
					// Output HTML according to the CommonMark spec


//
// Local functions...
//

static void		add_spec_text(char *dst, const char *src, size_t dstsize);
static void		indent_puts(FILE *logfile, const char *text, int cursor);
static int		is_equal(const char *generated, const char *expected, int *failed_at);
static const char	*make_anchor(const char *text);
static int		run_spec(const char *filename, FILE *logfile);
static void		usage(void);
static void		write_block(FILE *fp, mmd_t *parent);
static void		write_html(FILE *fp, const char *s);
static void		write_leaf(FILE *fp, mmd_t *node);


//
// 'main()' - Main entry for unit test program.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  int		only_body = 0;		// Only output body content?
  FILE		*fp = stdout;		// Output file
  const char	*filename = NULL;	// File to load
  mmd_t         *doc;                   // Document
  const char    *title;                 // Title


  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--ext"))
    {
      i ++;
      if (i >= argc)
      {
        usage();
        return (1);
      }

      if (!strcmp(argv[i], "all"))
      {
        mmdSetOptions(MMD_OPTION_ALL);
      }
      else if (!strcmp(argv[i], "none"))
      {
        mmdSetOptions(MMD_OPTION_NONE);
      }
      else
      {
        usage();
        return (1);
      }
    }
    else if (!strcmp(argv[i], "--help"))
    {
      usage();
      return (0);
    }
    else if (!strcmp(argv[i], "--only-body"))
    {
      only_body = 1;
    }
    else if (!strcmp(argv[i], "-o"))
    {
      i ++;
      if (i >= argc)
      {
        usage();
        return (1);
      }

      if ((fp = fopen(argv[i], "w")) == NULL)
      {
        perror(argv[i]);
        return (1);
      }
    }
    else if (!strcmp(argv[i], "--spec"))
    {
      spec_mode = true;
    }
    else if (argv[i][0] == '-')
    {
      printf("Unknown option '%s'.\n", argv[i]);
      usage();
      return (1);
    }
    else if (filename)
    {
      usage();
      return (1);
    }
    else
      filename = argv[i];
  }

  if (spec_mode)
    return (run_spec(filename, fp));
  else if (filename)
    doc = mmdLoad(NULL, filename);
  else
    doc = mmdLoadFile(NULL, stdin);

  if (!doc)
  {
    perror(filename ? filename : "(stdin)");
    return (1);
  }

  title = mmdGetMetadata(doc, "title");

  if (!only_body)
  {
    fputs("<!DOCTYPE html>\n"
          "<html>\n"
          "<head>\n"
          "<title>", fp);
    write_html(fp, title ? title : "Unknown");
    fputs("</title>\n"
          "<style><!--\n"
          "body {\n"
	  "  font-family: sans-serif;\n"
	  "  font-size: 18px;\n"
	  "  line-height: 150%;\n"
	  "}\n"
	  "a {\n"
	  "  font: inherit;\n"
	  "}\n"
	  "pre, li code, p code {\n"
	  "  font-family: monospace;\n"
	  "}\n"
	  "pre {\n"
	  "  background: #f8f8f8;\n"
	  "  border: solid thin #666;\n"
	  "  line-height: 120%;\n"
	  "  padding: 10px;\n"
	  "}\n"
	  "li code, p code {\n"
	  "  padding: 2px 5px;\n"
	  "}\n"
	  "table {\n"
	  "  border: solid thin #999;\n"
	  "  border-collapse: collapse;\n"
	  "  border-spacing: 0;\n"
	  "}\n"
	  "td {\n"
	  "  border: solid thin #ccc;\n"
	  "  padding-top: 5px;\n"
	  "}\n"
	  "td.left {\n"
	  "  text-align: left;\n"
	  "}\n"
	  "td.center {\n"
	  "  text-align: center;\n"
	  "}\n"
	  "td.right {\n"
	  "  text-align: right;\n"
	  "}\n"
	  "th {\n"
	  "  background: #ccc;\n"
	  "  border: none;\n"
	  "  border-bottom: solid thin #999;\n"
	  "  padding: 1px 5px;\n"
	  "  text-align: center;\n"
	  "}\n"
	  "--></style>\n"
	  "</head>\n"
	  "<body>\n", fp);
  }

  write_block(fp, doc);

  if (!only_body)
  {
    fputs("</body>\n"
          "</html>\n", fp);
  }

  if (fp != stdout)
    fclose(fp);

  mmdFree(doc);

  return (0);
}


//
// 'add_spec_text()' - Add text from a specification file, substituting
//                     placeholders as needed.
//

static void
add_spec_text(char       *dst,		// I - Destination buffer
              const char *src,		// I - Source string
              size_t     dstsize)	// I - Size of destination buffer
{
  int	col = 0,			// Current column
	in_html = 0;			// Inside a HTML element?
  char	*dstend = dst + dstsize - 1;	// End of destination buffer


  while (*src && dst < dstend)
  {
    if (!memcmp(src, "\342\206\222", 3))
    {
      src    += 3;

      do
      {
        *dst++ = ' ';
        col ++;
      }
      while ((col & 3) && dst < dstend);
    }
    else
    {
      if (*src == '<')
      {
        in_html = 1;
      }
      else if (*src == '>')
      {
        if (in_html)
          in_html = 0;
        else
          col ++;
      }
      else if (!in_html && (!(*src & 0x80) || (*src & 0xc0) != 0x80))
      {
        col ++;
      }

      *dst++ = *src++;
    }
  }

  *dst = '\0';
}


//
// 'indent_puts()' - Write a string to the standard output, indenting each line
//                   by 8 spaces.
//

static void
indent_puts(FILE       *logfile,	// I - Log file
            const char *text,		// I - Text to write
            int        cursor)		// I - Location for cursor or -1 if none
{
  const char	*start,			// Start of line
		*end;			// End of line


  for (start = text; *start; start = end)
  {
    if ((end = strchr(start, '\n')) != NULL)
      end ++;
    else
      end = start + strlen(start);

    fputs("        ", logfile);
    fwrite(start, end - start, 1, logfile);

    if (cursor >= 0 && cursor >= (start - text) && cursor < (end - text))
    {
      int offset = cursor - (start - text);

      fputs("        ", logfile);
      while (offset > 0)
      {
        putc((offset & 1) ? ' ' : '-', logfile);
        offset --;
      }
      fputs("^\n", logfile);
    }
  }
}


//
// 'is_equal()' - Compare two strings, allowing for specific differences.
//

static int				// O - 1 if equal, 0 if not equal
is_equal(const char *generated,		// I - Generated string
         const char *expected,		// I - Expected string
         int        *failed_at)		// O - Where the comparison failed
{
  const char	*gptr = generated;	// Pointer into generated string


  while (*gptr && *expected)
  {
    // Skip whitespace differences
    while (*gptr && isspace(*gptr & 255))
      gptr ++;

    while (*expected && isspace(*expected & 255))
      expected ++;

    if (!*gptr || !*expected)
      break;

    // Check non-whitespace characters...
    if (*gptr != *expected)
    {
      // The expected list HTML is widely inconsistent in the CommonMark
      // specification WRT the use of paragraph (<p>) elements.  Allow for some
      // variation as long as the visual structure is the same.
      if (!strncmp(gptr, "<p>", 3) && *expected == gptr[3])
      {
        gptr += 3;
      }
      else if (!strncmp(gptr, "</p>", 4))
      {
        if (gptr[4] == '\n' && *expected == gptr[5])
          gptr += 5;
        else if (*expected == gptr[4])
          gptr += 4;
      }
      else if (!strncmp(gptr, "p>\n</", 5) && *expected == gptr[5])
      {
        gptr += 5;
      }
      else if (!strncmp(gptr, "/p>\n<", 5) && *expected == gptr[5])
      {
        gptr += 5;
      }
    }

    if (*gptr != *expected && (!isspace(*gptr & 255) || !isspace(*expected & 255)))
    {
      // If the strings don't match at this point and the difference is not in
      // the type of whitespace used, that is an error...
//      fprintf(stderr, "gptr=\"%s\", expected=\"%s\"\n", gptr, expected);
      break;
    }

    gptr ++;
    expected ++;
  }

  *failed_at = *gptr ? (int)(gptr - generated) : -1;

  return (*gptr == *expected);
}


//
// 'make_anchor()' - Make an anchor for internal links.
//

static const char *                     // O - Anchor string
make_anchor(const char *text)           // I - Text
{
  char          *bufptr;                // Pointer into buffer
  static char   buffer[1024];           // Buffer for anchor string


  if (!text)
    return ("");

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


//
// 'run_spec()' - Run through all of the examples in the specified markdown
//                file.
//

static int				// O - Exit status
run_spec(const char *filename,		// I - Markdown spec file
         FILE       *logfile)		// I - Log file
{
  FILE	*fp;				// File to read from
  int	number = 0,			// Current example number
	passed = 0,			// Number of passed tests
	skipped = 0,			// Number of skipped tests
	failed = 0;			// Number of failed tests
  char	line[1024],			// Line from file
	markdown[4096],			// Example markdown
	html[4096],			// Expected HTML
	*mptr,				// Pointer into markdown
	*hptr;				// Pointer into HTML


  if (!filename)
  {
    fp = stdin;
  }
  else if ((fp = fopen(filename, "r")) == NULL)
  {
    perror(filename);
    return (1);
  }

  while (fgets(line, sizeof(line), fp))
  {
    if (!strncmp(line, "````", 4) && strstr(line, "``` example"))
    {
      // Start of example...
      int is_html = 0;			// 0 = markdown, 1 = HTML

      number ++;
      fprintf(logfile, "    E%04d: ", number);

      markdown[0] = markdown[sizeof(markdown) - 1] = '\0';
      html[0]     = html[sizeof(html) - 1]         = '\0';

      mptr = markdown;
      hptr = html;

      while (fgets(line, sizeof(line), fp))
      {
        if (line[0] == '.')
        {
          is_html = 1;
        }
        else if (!strncmp(line, "````", 4))
        {
          break;
        }
        else if (is_html)
        {
          add_spec_text(hptr, line, sizeof(html) - (size_t)(hptr - html));
          hptr += strlen(hptr);
        }
        else
        {
          add_spec_text(mptr, line, sizeof(markdown) - (size_t)(mptr - markdown));
          mptr += strlen(mptr);
        }
      }

      if (!markdown[0])
      {
        fputs("SKIP (no markdown)\n", logfile);
        skipped ++;
      }
      else if (!html[0])
      {
        fputs("SKIP (no HTML)\n", logfile);
        skipped ++;
      }
      else if ((mptr = strchr(markdown, '<')) != NULL && (mptr == markdown || (mptr[-1] != '`' && mptr[-1] != '\\')))
      {
        fputs("SKIP (markdown example with embedded HTML)\n", logfile);
        skipped ++;
      }
      else if ((mptr = strchr(markdown, '&')) != NULL && (mptr == markdown || (mptr[-1] != '`' && mptr[-1] != '\\')))
      {
        fputs("SKIP (markdown example with embedded HTML)\n", logfile);
        skipped ++;
      }
      else
      {
        // Run a test...
        FILE	*infile,		// Input file
		*outfile;		// Output file
        char	outbuffer[4097];	// Output buffer
        mmd_t	*doc;			// Markdown document
        int	test_passed = 0,	// Did the test pass?
		failed_at = -1;		// Offset of failure


        infile  = fmemopen(markdown, strlen(markdown), "r");
        outfile = fmemopen(outbuffer, sizeof(outbuffer) - 1, "w");

        outbuffer[0] = outbuffer[sizeof(outbuffer) - 1] = '\0';

        if ((doc = mmdLoadFile(NULL, infile)) == NULL)
        {
          fputs("FAIL (unable to load)\n", logfile);
          failed ++;
        }
        else
        {
	  write_block(outfile, doc);

	  mmdFree(doc);
	}

        fclose(infile);
        fclose(outfile);

        if (is_equal(outbuffer, html, &failed_at))
        {
          fputs("PASS\n", logfile);
          passed ++;
          test_passed = 1;
        }
        else
        {
          fputs("FAIL (HTML differs)\n", logfile);
          failed ++;
        }

        if (!test_passed)
        {
          fputs("    Markdown:\n", logfile);
          indent_puts(logfile, markdown, -1);
          fputs("    Expected:\n", logfile);
          indent_puts(logfile, html, -1);
          fputs("    Got:\n", logfile);
          indent_puts(logfile, outbuffer, failed_at);
          fputs("\n", logfile);
        }
      }
    }
    else if (line[0] == '#')
    {
      // Show heading...
      char *lptr;			// Pointer into line

      for (lptr = line; *lptr; lptr ++)
        if (*lptr != '#' && !isspace(*lptr & 255))
          break;			// Find start of heading...

      fputs(lptr, logfile);
    }
  }

  if (fp != stdin)
    fclose(fp);

  if (number == 0)
    puts("\nNo tests found.");
  else
    printf("\nSummary: %d%% (%d passed, %d skipped, %d failed)\n", 100 * (passed + skipped) / number, passed, skipped, failed);

  return (failed != 0);
}


//
// 'usage()' - Show usage...
//

static void
usage(void)
{
  puts("Usage: ./testmmd [options] [filename.md] > filename.html");
  puts("Options:");
  puts("--ext all         Support all markdown extensions");
  puts("--ext none        Support no markdown extensions");
  puts("--help            Show help");
  puts("--only-body       Only output body content");
  puts("--spec            Markdown file is a specification with example input and");
  puts("                  expected HTML output");
  puts("-o filename.html  Send output to file instead of stdout");
}


//
// 'write_block()' - Write a block node as HTML.
//

static void
write_block(FILE  *fp,			// I - Output file
            mmd_t *parent)              // I - Parent node
{
  const char    *element,               // Enclosing element, if any
		*hclass = NULL;		// HTML class, if any
  mmd_t         *node;                  // Current child node
  mmd_type_t    type;                   // Node type


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
          write_html(fp, mmdGetText(node));
        fputs("</code></pre>\n", fp);
        return;

    case MMD_TYPE_THEMATIC_BREAK :
        fputs("<hr />\n", fp);
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

  if (type >= MMD_TYPE_HEADING_1 && type <= MMD_TYPE_HEADING_6 && !spec_mode)
  {
    // Add an anchor for each heading...
    fprintf(fp, "<%s id=\"", element);
    for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
    {
      if (mmdGetWhitespace(node))
        fputc('-', fp);

      fputs(make_anchor(mmdGetText(node)), fp);
    }
    fputs("\">", fp);
  }
  else if (element)
    fprintf(fp, "<%s%s%s>%s", element, hclass ? " class=" : "", hclass ? hclass : "", type <= MMD_TYPE_LIST_ITEM ? "\n" : "");

  for (node = mmdGetFirstChild(parent); node; node = mmdGetNextSibling(node))
  {
    if (mmdIsBlock(node))
      write_block(fp, node);
    else
      write_leaf(fp, node);
  }

  if (element)
    fprintf(fp, "</%s>\n", element);
}


//
// 'write_html()' - Write text as HTML.
//

static void
write_html(FILE       *fp,		// I - Output file
           const char *text)            // I - Text string
{
  if (!text)
    return;

  while (*text)
  {
    if (*text == '&')
      fputs("&amp;", fp);
    else if (*text == '<')
      fputs("&lt;", fp);
    else if (*text == '>')
      fputs("&gt;", fp);
    else if (*text == '\"')
      fputs("&quot;", fp);
    else
      fputc(*text, fp);

    text ++;
  }
}


//
// 'write_leaf()' - Write a leaf node as HTML.
//

static void
write_leaf(FILE  *fp,			// I - Output file
           mmd_t *node)                 // I - Leaf node
{
  mmd_type_t	type,			// Current leaf node type
		prev_type,		// Previous leaf node type
		next_type;		// Next leaf node type
  const char    *element,               // Encoding element, if any
                *text,                  // Text to write
                *url;                   // URL to write


  if (mmdGetWhitespace(node))
    fputc(' ', fp);

  text = mmdGetText(node);
  url  = mmdGetURL(node);

  switch (type = mmdGetType(node))
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
        fputs("<img src=\"", fp);
        write_html(fp, url);
        fputs("\" alt=\"", fp);
        write_html(fp, text);
        fputs("\" />", fp);
        return;

    case MMD_TYPE_HARD_BREAK :
        if (mmdGetType(mmdGetParent(node)) < MMD_TYPE_HEADING_1 || mmdGetType(mmdGetParent(node)) > MMD_TYPE_HEADING_6)
          fputs("<br />\n", fp);
        return;

    case MMD_TYPE_SOFT_BREAK :
        fputs("<wbr />\n", fp);
        return;

    case MMD_TYPE_METADATA_TEXT :
        return;

    case MMD_TYPE_CHECKBOX :
        // Put a checkbox graphic, checked if the text is non-NULL and empty otherwise...
        if (text)
	  fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" fill=\"currentColor\" class=\"bi bi-check-square\" viewBox=\"0 0 16 16\"><path d=\"M14 1a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1h12zM2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2H2z\"/><path d=\"M10.97 4.97a.75.75 0 0 1 1.071 1.05l-3.992 4.99a.75.75 0 0 1-1.08.02L4.324 8.384a.75.75 0 1 1 1.06-1.06l2.094 2.093 3.473-4.425a.235.235 0 0 1 .02-.022z\"/></svg>", fp);
        else
          fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" fill=\"currentColor\" class=\"bi bi-square\" viewBox=\"0 0 16 16\"><path d=\"M14 1a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1h12zM2 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2H2z\"/></svg>", fp);
        return;

    default :
        element = NULL;
        break;
  }

  prev_type = mmdGetType(mmdGetPrevSibling(node));
  next_type = mmdGetType(mmdGetNextSibling(node));

  if (url)
  {
    const char *prev_url = mmdGetURL(mmdGetPrevSibling(node));
    const char *title = mmdGetExtra(node);

    if (!prev_url || strcmp(prev_url, url))
    {
      if (!strcmp(url, "@"))
	fprintf(fp, "<a href=\"#%s\"", make_anchor(text));
      else
	fprintf(fp, "<a href=\"%s\"", url);

      if (title)
      {
        fputs(" title=\"", fp);
        write_html(fp, title);
        fputs("\">", fp);
      }
      else
        putc('>', fp);
    }
  }

  if (element && prev_type != type)
    fprintf(fp, "<%s>", element);

  write_html(fp, text);

  if (element && next_type != type)
    fprintf(fp, "</%s>", element);

  if (url)
  {
    const char *next_url = mmdGetURL(mmdGetNextSibling(node));

    if (!next_url || strcmp(next_url, url))
      fputs("</a>", fp);
  }
}
