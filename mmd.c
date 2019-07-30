/*
 * Implementation of miniature markdown library.
 *
 *     https://github.com/michaelrsweet/mmd
 *
 * Copyright © 2017-2019 by Michael R Sweet.
 *
 * Licensed under Apache License v2.0.  See the file "LICENSE" for more
 * information.
 */

/*
 * Define DEBUG to get debug printf messages to stderr.
 */

#define DEBUG 0
#if DEBUG > 0
#  define DEBUG_printf(...)	fprintf(stderr, __VA_ARGS__)
#  define DEBUG_puts(s)		fputs(s, stderr);
#else
#  define DEBUG_printf(...)
#  define DEBUG_puts(s)
#endif /* DEBUG > 0 */
#if DEBUG > 1
#  define DEBUG2_printf(...)	fprintf(stderr, __VA_ARGS__)
#  define DEBUG2_puts(s)	fputs(s, stderr);
#else
#  define DEBUG2_printf(...)
#  define DEBUG2_puts(s)
#endif /* DEBUG > 1 */


/*
 * Beginning with VC2005, Microsoft breaks ISO C and POSIX conformance
 * by deprecating a number of functions in the name of security, even
 * when many of the affected functions are otherwise completely secure.
 * The _CRT_SECURE_NO_DEPRECATE definition ensures that we won't get
 * warnings from their use...
 *
 * Then Microsoft decided that they should ignore this in VC2008 and use
 * yet another define (_CRT_SECURE_NO_WARNINGS) instead...
 */

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS


/*
 * Include necessary headers...
 */

#include "mmd.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


/*
 * Microsoft renames the POSIX functions to _name, and introduces a broken
 * compatibility layer using the original names.  As a result, random crashes
 * can occur when, for example, strdup() allocates memory from a different heap
 * than used by malloc() and free().
 *
 * To avoid moronic problems like this, we #define the POSIX function names to
 * the corresponding non-standard Microsoft names.
 */

#ifdef _WIN32
#  define snprintf 	_snprintf
#  define strdup	_strdup
#endif /* _WIN32 */


/*
 * Structures...
 */

struct _mmd_s
{
  mmd_type_t    type;                   /* Node type */
  int           whitespace;             /* Leading whitespace? */
  char          *text,                  /* Text */
                *url,                   /* Reference URL (image/link/etc.) */
                *extra;			/* Title, language name, etc. */
  mmd_t         *parent,                /* Parent node */
                *first_child,           /* First child node */
                *last_child,            /* Last child node */
                *prev_sibling,          /* Previous sibling node */
                *next_sibling;          /* Next sibling node */
};

typedef struct _mmd_filebuf_s		/**** Buffered file ****/
{
  FILE		*fp;			/* File pointer */
  char		buffer[65536],		/* Buffer */
		*bufptr,		/* Pointer into buffer */
		*bufend;		/* End of buffer */
} _mmd_filebuf_t;

typedef struct _mmd_ref_s		/**** Reference link ****/
{
  char		*name,			/* Name of reference */
		*url;			/* Reference URL */
  size_t	num_pending;		/* Number of pending nodes */
  mmd_t		**pending;		/* Pending nodes */
} _mmd_ref_t;

typedef struct _mmd_doc_s		/**** Markdown document ****/
{
  mmd_t		*root;			/* Root node */
  size_t	num_references;		/* Number of references */
  _mmd_ref_t	*references;		/* References */
} _mmd_doc_t;

typedef struct _mmd_stack_s		/**** Markdown block stack ****/
{
  mmd_t		*parent;		/* Parent node */
  int		indent;			/* Indentation */
  char		fence;			/* Code fence character */
  size_t	fencelen;		/* Length of code fence */
} _mmd_stack_t;


/*
 * Local globals...
 */

static mmd_option_t	mmd_options = MMD_OPTION_ALL;
					/* Markdown extensions to support */


/*
 * Local functions...
 */

static mmd_t    *mmd_add(mmd_t *parent, mmd_type_t type, int whitespace, char *text, char *url);
static void     mmd_free(mmd_t *node);
static int	mmd_has_continuation(const char *line, _mmd_filebuf_t *file, int indent);
static size_t	mmd_is_chars(const char *lineptr, const char *chars, size_t minchars);
static size_t	mmd_is_codefence(char *lineptr, char fence, size_t fencelen, char **language);
static int	mmd_is_table(_mmd_filebuf_t *file);
static void     mmd_parse_inline(_mmd_doc_t *doc, mmd_t *parent, char *lineptr);
static char     *mmd_parse_link(_mmd_doc_t *doc, char *lineptr, char **text, char **url, char **refname);
static void	mmd_read_buffer(_mmd_filebuf_t *file);
static char	*mmd_read_line(_mmd_filebuf_t *file, char *line, size_t linesize);
static void	mmd_ref_add(_mmd_doc_t *doc, mmd_t *node, const char *name, const char *url);
static _mmd_ref_t *mmd_ref_find(_mmd_doc_t *doc, const char *name);
static void     mmd_remove(mmd_t *node);
#if DEBUG
static const char *mmd_type_string(mmd_type_t type);
#endif /* DEBUG */


/*
 * 'mmdCopyAllText()' - Make a copy of all the text under a given node.
 *
 * The returned string must be freed using free().
 */

char *					/* O - Copied string */
mmdCopyAllText(mmd_t *node)		/* I - Parent node */
{
  char		*all = NULL,		/* String buffer */
		*allptr = NULL,		/* Pointer into string buffer */
		*temp;			/* Temporary pointer */
  size_t	allsize = 0,		/* Size of "all" buffer */
		textlen;		/* Length of "text" string */
  mmd_t		*current,		/* Current node */
		*next;			/* Next node */


  current = mmdGetFirstChild(node);

  while (current != node)
  {
    if (current->text)
    {
     /*
      * Append this node's text to the string...
      */

      textlen = strlen(current->text);

      if (allsize == 0)
      {
        allsize = textlen + (size_t)current->whitespace + 1;
        all     = malloc(allsize);
        allptr  = all;

	if (!all)
	  return (NULL);
      }
      else
      {
        allsize += textlen + (size_t)current->whitespace;
        temp    = realloc(all, allsize);

        if (!temp)
        {
          free(all);
          return (NULL);
        }

        allptr = temp + (allptr - all);
        all    = temp;
      }

      if (current->whitespace)
        *allptr++ = ' ';

      memcpy(allptr, current->text, textlen);
      allptr += textlen;
    }

   /*
    * Find the next logical node...
    */

    if ((next = mmdGetNextSibling(current)) == NULL)
    {
      next = mmdGetParent(current);

      while (next && next != node && mmdGetNextSibling(next) == NULL)
	next = mmdGetParent(next);

      if (next != node)
        next = mmdGetNextSibling(next);
    }

    current = next;
  }

  if (allptr)
    *allptr = '\0';

  return (all);
}


/*
 * 'mmdFree()' - Free a markdown tree.
 */

void
mmdFree(mmd_t *node)                    /* I - First node */
{
  mmd_t *current,		        /* Current node */
	*next;			        /* Next node */


  mmd_remove(node);

  for (current = node->first_child; current; current = next)
  {
   /*
    * Get the next node...
    */

    if ((next = current->first_child) != NULL)
    {
     /*
      * Free parent nodes after child nodes have been freed...
      */

      current->first_child = NULL;
      continue;
    }

    if ((next = current->next_sibling) == NULL)
    {
     /*
      * Next node is the parent, which we'll free as needed...
      */

      if ((next = current->parent) == node)
        next = NULL;
    }

   /*
    * Free child...
    */

    mmd_free(current);
  }

 /*
  * Then free the memory used by the parent node...
  */

  mmd_free(node);
}


/*
 * 'mmdGetExtra()' - Get extra text (title, language, etc.) associated with a
 *                   node.
 */

const char *				/* O - Extra text or NULL if none */
mmdGetExtra(mmd_t *node)		/* I - Node */
{
  return (node ? node->extra : NULL);
}


/*
 * 'mmdGetFirstChild()' - Return the first child of a node, if any.
 */

mmd_t *                                 /* O - First child or @code NULL@ if none */
mmdGetFirstChild(mmd_t *node)           /* I - Node */
{
  return (node ? node->first_child : NULL);
}


/*
 * 'mmdGetLastChild()' - Return the last child of a node, if any.
 */

mmd_t *                                 /* O - Last child or @code NULL@ if none */
mmdGetLastChild(mmd_t *node)            /* I - Node */
{
  return (node ? node->last_child : NULL);
}


/*
 * 'mmdGetMetadata()' - Return the metadata for the given keyword.
 */

const char *                            /* O - Value or @code NULL@ if none */
mmdGetMetadata(mmd_t      *doc,         /* I - Document */
               const char *keyword)     /* I - Keyword */
{
  mmd_t         *metadata,              /* Metadata node */
                *current;               /* Current node */
  char          prefix[256];            /* Prefix string */
  size_t        prefix_len;             /* Length of prefix string */
  const char    *value;                 /* Pointer to value */


  if (!doc || (metadata = doc->first_child) == NULL || metadata->type != MMD_TYPE_METADATA)
    return (NULL);

  snprintf(prefix, sizeof(prefix), "%s:", keyword);
  prefix_len = strlen(prefix);

  for (current = metadata->first_child; current; current = current->next_sibling)
  {
    if (strncmp(current->text, prefix, prefix_len))
      continue;

    value = current->text + prefix_len;
    while (isspace(*value & 255))
      value ++;

    return (value);
  }

  return (NULL);
}


/*
 * 'mmdGetNextSibling()' - Return the next sibling of a node, if any.
 */

mmd_t *                                 /* O - Next sibling or @code NULL@ if none */
mmdGetNextSibling(mmd_t *node)          /* I - Node */
{
  return (node ? node->next_sibling : NULL);
}


/*
 * 'mmdGetOptions()' - Get the enabled markdown processing options/extensions.
 */

mmd_option_t				/* O - Enabled options */
mmdGetOptions(void)
{
  return (mmd_options);
}


/*
 * 'mmdGetParent()' - Return the parent of a node, if any.
 */

mmd_t *                                 /* O - Parent node or @code NULL@ if none */
mmdGetParent(mmd_t *node)               /* I - Node */
{
  return (node ? node->parent : NULL);
}


/*
 * 'mmdGetPrevSibling()' - Return the previous sibling of a node, if any.
 */

mmd_t *                                 /* O - Previous sibling or @code NULL@ if none */
mmdGetPrevSibling(mmd_t *node)          /* I - Node */
{
  return (node ? node->prev_sibling : NULL);
}


/*
 * 'mmdGetText()' - Return the text associated with a node, if any.
 */

const char *                            /* O - Text or @code NULL@ if none */
mmdGetText(mmd_t *node)                 /* I - Node */
{
  return (node ? node->text : NULL);
}


/*
 * 'mmdGetType()' - Return the type of a node, if any.
 */

mmd_type_t                              /* O - Type or @code MMD_TYPE_NONE@ if none */
mmdGetType(mmd_t *node)                 /* I - Node */
{
  return (node ? node->type : MMD_TYPE_NONE);
}


/*
 * 'mmdGetURL()' - Return the URL associated with a node, if any.
 */

const char *                            /* O - URL or @code NULL@ if none */
mmdGetURL(mmd_t *node)                  /* I - Node */
{
  return (node ? node->url : NULL);
}


/*
 * 'mmdGetWhitespace()' - Return whether whitespace preceded a node.
 */

int                                     /* O - 1 for whitespace, 0 for none */
mmdGetWhitespace(mmd_t *node)           /* I - Node */
{
  return (node ? node->whitespace : 0);
}


/*
 * 'mmdIsBlock()' - Return whether the node is a block.
 */

int                                     /* O - 1 for block nodes, 0 otherwise */
mmdIsBlock(mmd_t *node)                 /* I - Node */
{
  return (node ? node->type < MMD_TYPE_NORMAL_TEXT : 0);
}


/*
 * 'mmdLoad()' - Load a markdown file into nodes.
 */

mmd_t *                                 /* O - First node in markdown */
mmdLoad(const char *filename)           /* I - File to load */
{
  FILE          *fp;                    /* File */
  mmd_t         *doc;                   /* Document */


 /*
  * Open the file and create an empty document...
  */

  if ((fp = fopen(filename, "r")) == NULL)
    return (NULL);

  doc = mmdLoadFile(fp);

  fclose(fp);

  return (doc);
}


/*
 * 'mmdLoadFile()' - Load a markdown file into nodes from a stdio file.
 */

mmd_t *                                 /* O - First node in markdown */
mmdLoadFile(FILE *fp)                   /* I - File to load */
{
  size_t	i;			/* Looping var */
  _mmd_doc_t	doc;			/* Document */
  _mmd_ref_t	*reference;		/* Current reference */
  mmd_t         *block = NULL;          /* Current block */
  mmd_type_t    type;                   /* Type for line */
  _mmd_filebuf_t file;			/* File buffer */
  char		line[8192],		/* Read line */
		*linestart,		/* Start of line */
		*lineptr,               /* Pointer into line */
		*lineend,               /* End of line */
		*temp;			/* Temporary pointer */
  int		newindent;		/* New indentation */
  int           blank_code = 0;         /* Saved indented blank code line */
  mmd_type_t	columns[256];		/* Alignment of table columns */
  int		num_columns = 0,	/* Number of columns in table */
		rows = 0;		/* Number of rows in table */
  _mmd_stack_t	stack[32],		/* Block stack */
		*stackptr = stack;	/* Pointer to top of stack */


 /*
  * Create an empty document...
  */

  memset(&doc, 0, sizeof(doc));

  doc.root = mmd_add(NULL, MMD_TYPE_DOCUMENT, 0, NULL, NULL);

  if (!doc.root)
  {
    fclose(fp);
    return (NULL);
  }

 /*
  * Initialize the block stack...
  */

  memset(stack, 0, sizeof(stack));
  stackptr->parent = doc.root;

 /*
  * Read lines until end-of-file...
  */

  memset(&file, 0, sizeof(file));
  file.fp = fp;

  while ((lineptr = mmd_read_line(&file, line, sizeof(line))) != NULL)
  {
    DEBUG_printf("%03d  %-12s  %s", stackptr->indent, mmd_type_string(stackptr->parent->type) + 9, lineptr);

    linestart = lineptr;

    while (isspace(*lineptr & 255))
      lineptr ++;

    if (*lineptr == '>' && (lineptr - linestart) < 4)
    {
     /*
      * Block quote.  See if there is an existing blockquote...
      */

      if (stackptr == stack || stack[1].parent->type != MMD_TYPE_BLOCK_QUOTE)
      {
        block            = NULL;
        stackptr         = stack + 1;
        stackptr->parent = mmd_add(doc.root, MMD_TYPE_BLOCK_QUOTE, 0, NULL, NULL);
        stackptr->indent = 2;
        stackptr->fence  = '\0';
      }

     /*
      * Skip whitespace after the ">"...
      */

      lineptr ++;
      if (isspace(*lineptr & 255))
        lineptr ++;

      linestart = lineptr;

      while (isspace(*lineptr & 255))
	lineptr ++;
    }
    else if (*lineptr != '>' && stackptr > stack && stack[1].parent->type == MMD_TYPE_BLOCK_QUOTE && (!block || *lineptr == '\n' || mmd_is_chars(lineptr, "- \t", 3) || mmd_is_chars(lineptr, "_ \t", 3) || mmd_is_chars(lineptr, "* \t", 3)))
    {
     /*
      * Not a lazy continuation so terminate this block quote...
      */

      block    = NULL;
      stackptr = stack;
    }

    if ((lineptr - line - stackptr->indent) < 4 && ((stackptr->parent->type != MMD_TYPE_CODE_BLOCK && !stackptr->fence && mmd_is_codefence(lineptr, '\0', 0, NULL)) || (stackptr->fence && mmd_is_codefence(lineptr, stackptr->fence, stackptr->fencelen, NULL))))
    {
     /*
      * Code fence...
      */

      DEBUG2_printf("stackptr->indent=%d, fence='%c', fencelen=%d\n", stackptr->indent, stackptr->fence, (int)stackptr->fencelen);

      if (stackptr->parent->type == MMD_TYPE_CODE_BLOCK)
      {
        DEBUG2_puts("Ending code block...\n");
        stackptr --;
      }
      else if (stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
      {
        char	*language;		/* Language name, if any */

        DEBUG2_printf("Starting code block with fence '%c'.\n", *lineptr);

        block                = NULL;
        stackptr[1].parent   = mmd_add(stackptr->parent, MMD_TYPE_CODE_BLOCK, 0, NULL, NULL);
        stackptr[1].indent   = lineptr - line;
        stackptr[1].fence    = *lineptr;
        stackptr[1].fencelen = mmd_is_codefence(lineptr, '\0', 0, &language);
        stackptr ++;

        DEBUG2_printf("Code language=\"%s\"\n", language);

        if (language)
          stackptr->parent->extra = strdup(language);
      }
      continue;
    }
    else if (stackptr->parent->type == MMD_TYPE_CODE_BLOCK && (lineptr - line) >= stackptr->indent)
    {
      if (line[stackptr->indent] == '\n')
      {
        blank_code ++;
      }
      else
      {
	while (blank_code > 0)
	{
	  mmd_add(stackptr->parent, MMD_TYPE_CODE_TEXT, 0, "\n", NULL);
	  blank_code --;
	}

	mmd_add(stackptr->parent, MMD_TYPE_CODE_TEXT, 0, line + stackptr->indent, NULL);
      }
      continue;
    }
    else if (stackptr->parent->type == MMD_TYPE_CODE_BLOCK && stackptr->fence)
    {
      if (!*lineptr)
      {
        blank_code ++;
      }
      else
      {
	while (blank_code > 0)
	{
	  mmd_add(stackptr->parent, MMD_TYPE_CODE_TEXT, 0, "\n", NULL);
	  blank_code --;
	}

	mmd_add(stackptr->parent, MMD_TYPE_CODE_TEXT, 0, lineptr, NULL);
      }
      continue;
    }
    else if (!strncmp(lineptr, "---", 3) && doc.root->first_child == NULL && (mmd_options & MMD_OPTION_METADATA))
    {
     /*
      * Document metadata...
      */

      block = mmd_add(doc.root, MMD_TYPE_METADATA, 0, NULL, NULL);

      while ((lineptr = mmd_read_line(&file, line, sizeof(line))) != NULL)
      {
        while (isspace(*lineptr & 255))
          lineptr ++;

        if (!strncmp(lineptr, "---", 3) || !strncmp(lineptr, "...", 3))
          break;

        lineend = lineptr + strlen(lineptr) - 1;
        if (lineend > lineptr && *lineend == '\n')
          *lineend = '\0';

        mmd_add(block, MMD_TYPE_METADATA_TEXT, 0, lineptr, NULL);
      }
      continue;
    }
    else if (block && block->type == MMD_TYPE_PARAGRAPH && (lineptr - linestart) < 4 && (lineptr - linestart) >= stackptr->indent && (mmd_is_chars(lineptr, "-", 1) || mmd_is_chars(lineptr, "=", 1)))
    {
      int ch = *lineptr;

      lineptr += 3;
      while (*lineptr == ch)
        lineptr ++;
      while (isspace(*lineptr & 255))
        lineptr ++;

      if (!*lineptr)
      {
        if (ch == '=')
          block->type = MMD_TYPE_HEADING_1;
        else
          block->type = MMD_TYPE_HEADING_2;

        block = NULL;
        continue;
      }

      type = MMD_TYPE_PARAGRAPH;
    }
    else if ((lineptr - linestart) < 4 && (mmd_is_chars(lineptr, "- \t", 3) || mmd_is_chars(lineptr, "_ \t", 3) || mmd_is_chars(lineptr, "* \t", 3)))
    {
      if (line[0] == '>')
        stackptr = stack + 1;
      else
        stackptr = stack;

      mmd_add(stackptr->parent, MMD_TYPE_THEMATIC_BREAK, 0, NULL, NULL);
      type  = MMD_TYPE_PARAGRAPH;
      block = NULL;
      continue;
    }
    else if ((*lineptr == '-' || *lineptr == '+' || *lineptr == '*') && isspace(lineptr[1] & 255))
    {
     /*
      * Bulleted list...
      */

      lineptr   += 2;
      linestart = lineptr;
      newindent = linestart - line;

      while (isspace(*lineptr & 255))
        lineptr ++;

      while (stackptr > stack && stackptr->indent > newindent)
        stackptr --;

      if (stackptr->parent->type == MMD_TYPE_LIST_ITEM && stackptr->indent == newindent)
        stackptr --;

      if (stackptr->parent->type == MMD_TYPE_ORDERED_LIST && stackptr->indent == newindent)
	stackptr --;

      if (stackptr->parent->type != MMD_TYPE_UNORDERED_LIST && stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
      {
        stackptr[1].parent = mmd_add(stackptr->parent, MMD_TYPE_UNORDERED_LIST, 0, NULL, NULL);
        stackptr[1].indent = linestart - line;
        stackptr[1].fence  = '\0';
        stackptr ++;
      }

      if (stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
      {
        stackptr[1].parent = mmd_add(stackptr->parent, MMD_TYPE_LIST_ITEM, 0, NULL, NULL);
        stackptr[1].indent = linestart - line;
        stackptr[1].fence  = '\0';
        stackptr ++;
      }

      type  = MMD_TYPE_PARAGRAPH;
      block = NULL;

      if (mmd_is_chars(lineptr, "- \t", 3) || mmd_is_chars(lineptr, "_ \t", 3) || mmd_is_chars(lineptr, "* \t", 3))
      {
	mmd_add(stackptr->parent, MMD_TYPE_THEMATIC_BREAK, 0, NULL, NULL);
	continue;
      }
    }
    else if (isdigit(*lineptr & 255))
    {
     /*
      * Ordered list?
      */

      temp = lineptr + 1;

      while (isdigit(*temp & 255))
        temp ++;

      if ((*temp == '.' || *temp == ')') && isspace(temp[1] & 255))
      {
       /*
        * Yes, ordered list.
        */

        lineptr   = temp + 2;
        linestart = lineptr;
        newindent = linestart - line;

        while (isspace(*lineptr & 255))
          lineptr ++;

	while (stackptr > stack && stackptr->indent > newindent)
	  stackptr --;

	if (stackptr->parent->type == MMD_TYPE_LIST_ITEM && stackptr->indent == newindent)
	  stackptr --;

	if (stackptr->parent->type == MMD_TYPE_UNORDERED_LIST && stackptr->indent == newindent)
	  stackptr --;

	if (stackptr->parent->type != MMD_TYPE_ORDERED_LIST && stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
	{
	  stackptr[1].parent = mmd_add(stackptr->parent, MMD_TYPE_ORDERED_LIST, 0, NULL, NULL);
	  stackptr[1].indent = linestart - line;
	  stackptr[1].fence  = '\0';
	  stackptr ++;
	}

	if (stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
	{
	  stackptr[1].parent = mmd_add(stackptr->parent, MMD_TYPE_LIST_ITEM, 0, NULL, NULL);
	  stackptr[1].indent = linestart - line;
	  stackptr[1].fence  = '\0';
	  stackptr ++;
	}

	type  = MMD_TYPE_PARAGRAPH;
	block = NULL;
      }
      else
      {
       /*
        * No, just a regular paragraph...
        */

        type = block ? block->type : MMD_TYPE_PARAGRAPH;
      }
    }
    else if (*lineptr == '#' && (lineptr - linestart) < 4)
    {
     /*
      * Heading, count the number of '#' for the heading level...
      */

      newindent = lineptr - line;
      temp      = lineptr + 1;

      while (*temp == '#')
        temp ++;

      if ((temp - lineptr) <= 6 && isspace(*temp & 255))
      {
       /*
        * Heading 1-6...
        */

        type  = MMD_TYPE_HEADING_1 + (temp - lineptr - 1);
        block = NULL;

       /*
        * Skip whitespace after "#"...
        */

        lineptr = temp;
        while (isspace(*lineptr & 255))
          lineptr ++;

        linestart = lineptr;

       /*
        * Strip trailing "#" characters and whitespace...
        */

        temp = lineptr + strlen(lineptr) - 1;
	while (temp > lineptr && isspace(*temp & 255))
          *temp-- = '\0';
        while (temp > lineptr && *temp == '#')
          temp --;
        if (isspace(*temp & 255))
        {
          while (temp > lineptr && isspace(*temp & 255))
            *temp-- = '\0';
        }
        else if (temp == lineptr)
          *temp = '\0';

        block = mmd_add(stackptr->parent, type, 0, NULL, NULL);
      }
      else
      {
       /*
        * More than 6 #'s, just treat as a paragraph...
        */

        type = MMD_TYPE_PARAGRAPH;
      }

      while (stackptr > stack && stackptr->indent >= newindent)
        stackptr --;
    }
    else if (block && block->type >= MMD_TYPE_HEADING_1 && block->type <= MMD_TYPE_HEADING_6)
    {
      type  = MMD_TYPE_PARAGRAPH;
      block = NULL;
    }
    else if (!block)
    {
      type = MMD_TYPE_PARAGRAPH;

      if (lineptr == line)
        stackptr = stack;
    }
    else
      type = block->type;

    if (!*lineptr)
    {
      if (stackptr->parent->type == MMD_TYPE_CODE_BLOCK)
        blank_code ++;

      block = NULL;
      continue;
    }
    else if (!strcmp(lineptr, "+"))
    {
      if (block)
      {
        if (block->type == MMD_TYPE_LIST_ITEM)
          block = mmd_add(block, MMD_TYPE_PARAGRAPH, 0, NULL, NULL);
        else if (block->parent->type == MMD_TYPE_LIST_ITEM)
          block = mmd_add(block->parent, MMD_TYPE_PARAGRAPH, 0, NULL, NULL);
        else
          block = NULL;
      }
      continue;
    }
    else if ((mmd_options & MMD_OPTION_TABLES) && strchr(lineptr, '|') && (stackptr->parent->type == MMD_TYPE_TABLE || mmd_is_table(&file)))
    {
     /*
      * Table...
      */

      int	col;			/* Current column */
      char	*start,			/* Start of column/cell */
		*end;			/* End of column/cell */
      mmd_t	*row = NULL,		/* Current row */
		*cell;			/* Current cell */

      DEBUG2_printf("TABLE stackptr->parent=%p (%d), rows=%d\n", stackptr->parent, stackptr->parent->type, rows);

      if (stackptr->parent->type != MMD_TYPE_TABLE && stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
      {
        DEBUG2_printf("ADDING NEW TABLE to %p (%s)\n", stackptr->parent, mmd_type_string(stackptr->parent->type));

        stackptr[1].parent = mmd_add(stackptr->parent, MMD_TYPE_TABLE, 0, NULL, NULL);
        stackptr[1].indent = stackptr->indent;
        stackptr[1].fence  = '\0';
        stackptr ++;

        block = mmd_add(stackptr->parent, MMD_TYPE_TABLE_HEADER, 0, NULL, NULL);

        for (col = 0; col < (int)(sizeof(columns) / sizeof(columns[0])); col ++)
          columns[col] = MMD_TYPE_TABLE_BODY_CELL_LEFT;

        num_columns = 0;
        rows        = -1;
      }
      else if (rows > 0)
      {
        if (rows == 1)
          block = mmd_add(stackptr->parent, MMD_TYPE_TABLE_BODY, 0, NULL, NULL);
      }
      else
        block = NULL;

      if (block)
        row = mmd_add(block, MMD_TYPE_TABLE_ROW, 0, NULL, NULL);

      if (*lineptr == '|')
        lineptr ++;			/* Skip leading pipe */

      if ((end = lineptr + strlen(lineptr) - 1) > lineptr)
      {
        while ((*end == '\n' || *end == 'r') && end > lineptr)
          end --;

        if (end > lineptr && *end == '|')
	  *end = '\0';			/* Truncate trailing pipe */
      }

      for (col = 0; lineptr && *lineptr && col < (int)(sizeof(columns) / sizeof(columns[0])); col ++)
      {
       /*
        * Get the bounds of the stackptr->parent cell...
        */

        start = lineptr;
        if ((lineptr = strchr(lineptr + 1, '|')) != NULL)
          *lineptr++ = '\0';

        if (block)
        {
         /*
          * Add a cell to this row...
          */

          if (block->type == MMD_TYPE_TABLE_HEADER)
            cell = mmd_add(row, MMD_TYPE_TABLE_HEADER_CELL, 0, NULL, NULL);
          else
            cell = mmd_add(row, columns[col], 0, NULL, NULL);

          mmd_parse_inline(&doc, cell, start);
        }
        else
        {
         /*
          * Process separator row for alignment...
          */

	  while (isspace(*start & 255))
	    start ++;

          for (end = start + strlen(start) - 1; end > start && isspace(*end & 255); end --);

          if (*start == ':' && *end == ':')
            columns[col] = MMD_TYPE_TABLE_BODY_CELL_CENTER;
          else if (*end == ':')
            columns[col] = MMD_TYPE_TABLE_BODY_CELL_RIGHT;

          DEBUG2_printf("COLUMN %d SEPARATOR=\"%s\", TYPE=%d\n", col, start, columns[col]);
        }
      }

     /*
      * Make sure the table is balanced...
      */

      if (col > num_columns)
      {
        num_columns = col;
      }
      else if (block && block->type != MMD_TYPE_TABLE_HEADER)
      {
        while (col < num_columns)
        {
          mmd_add(row, columns[col], 0, NULL, NULL);
          col ++;
        }
      }

      rows ++;
      continue;
    }
    else if (stackptr->parent->type == MMD_TYPE_TABLE)
    {
      DEBUG2_puts("END TABLE\n");
      stackptr --;
      block = NULL;
    }

    if (stackptr->parent->type != MMD_TYPE_CODE_BLOCK && (!block || block->type == MMD_TYPE_CODE_BLOCK) && (lineptr - linestart) >= (stackptr->indent + 4))
    {
     /*
      * Indented code block.
      */

      if (stackptr->parent->type != MMD_TYPE_CODE_BLOCK && stackptr < (stack + sizeof(stack) / sizeof(stack[0]) - 1))
      {
        stackptr[1].parent = mmd_add(stackptr->parent, MMD_TYPE_CODE_BLOCK, 0, NULL, NULL);
        stackptr[1].indent = stackptr->indent + 4;
        stackptr ++;
      }

      while (blank_code > 0)
      {
        mmd_add(stackptr->parent, MMD_TYPE_CODE_TEXT, 0, "\n", NULL);
        blank_code --;
      }

      mmd_add(stackptr->parent, MMD_TYPE_CODE_TEXT, 0, line + stackptr->indent, NULL);
      continue;
    }

    if (!block || block->type != type)
    {
      if (stackptr->parent->type == MMD_TYPE_CODE_BLOCK)
        stackptr --;

      block = mmd_add(stackptr->parent, type, 0, NULL, NULL);
    }

   /*
    * Read continuation lines before parsing this...
    */

    while (mmd_has_continuation(line, &file, stackptr->indent))
    {
      char *ptr = line + strlen(line);

      if (!mmd_read_line(&file, ptr, sizeof(line) - (size_t)(ptr - line)))
        break;
    }

    mmd_parse_inline(&doc, block, lineptr);
  }

 /*
  * Free any references...
  */

  for (i = doc.num_references, reference = doc.references; i > 0; i --, reference ++)
  {
    if (reference->pending)
    {
      size_t	j;			/* Looping var */

      for (j = 0; j < reference->num_pending; j ++)
        reference->pending[j]->url = strdup(reference->name);

      free(reference->pending);
    }

    free(reference->name);
    free(reference->url);
  }

  free(doc.references);

 /*
  * Return the root node...
  */

  return (doc.root);
}


/*
 * 'mmdSetOptions()' - Set (enable/disable) support for various markdown options.
 */

void
mmdSetOptions(mmd_option_t options)	/* I - Options */
{
  mmd_options = options;
}


/*
 * 'mmd_add()' - Add a new markdown node.
 */

static mmd_t *                          /* O - New node */
mmd_add(mmd_t      *parent,             /* I - Parent node */
        mmd_type_t type,                /* I - Node type */
        int        whitespace,          /* I - 1 if whitespace precedes this node */
        char       *text,               /* I - Text, if any */
        char       *url)                /* I - URL, if any */
{
  mmd_t         *temp;                  /* New node */


  DEBUG2_printf("Adding %s to %p(%s), whitespace=%d, text=\"%s\", url=\"%s\"\n", mmd_type_string(type), parent, parent ? mmd_type_string(parent->type) : "", whitespace, text ? text : "(null)", url ? url : "(null)");

  if ((temp = calloc(1, sizeof(mmd_t))) != NULL)
  {
    if (parent)
    {
     /*
      * Add node to the parent...
      */

      temp->parent = parent;

      if (parent->last_child)
      {
        parent->last_child->next_sibling = temp;
        temp->prev_sibling               = parent->last_child;
        parent->last_child               = temp;
      }
      else
      {
        parent->first_child = parent->last_child = temp;
      }
    }

   /*
    * Copy the node values...
    */

    temp->type       = type;
    temp->whitespace = whitespace;

    if (text)
      temp->text = strdup(text);

    if (url)
      temp->url = strdup(url);
  }

  return (temp);
}


/*
 * 'mmd_free()' - Free memory used by a node.
 */

static void
mmd_free(mmd_t *node)                   /* I - Node */
{
  free(node->text);
  free(node->url);
  free(node->extra);
  free(node);
}


/*
 * 'mmd_has_continuation()' - Determine whether the next line is a continuation
 *                            of the current one.
 */

static int				/* O - 1 if the next line continues, 0 otherwise */
mmd_has_continuation(
    const char     *line,		/* I - Current line */
    _mmd_filebuf_t *file,		/* I - File buffer */
    int            indent)		/* I - Indentation for current block */
{
  const char	*lineptr = line;	/* Pointer into current line */
  const char	*fileptr = file->bufptr;/* Pointer into next line */


  do
  {
    while (isspace(*lineptr & 255))
      lineptr ++;

    while (isspace(*fileptr & 255))
      fileptr ++;

    if (*lineptr == '>' && *fileptr == '>')
    {
      lineptr ++;
      fileptr ++;
    }
    else if (*fileptr == '>')
      return (0);
  }
  while (isspace(*lineptr & 255) || isspace(*fileptr & 255));

  if (*lineptr == '#')
    return (0);

  if (strchr("-+*", *fileptr) && isspace(fileptr[1] & 255))
  {
   /*
    * Bullet list item...
    */

    return (0);
  }

  if (isdigit(*fileptr & 255))
  {
   /*
    * Ordered list item...
    */

    while (*fileptr && isdigit(*fileptr & 255))
      fileptr ++;

    if (*fileptr == '.' || *fileptr == '(')
      return (0);
  }

  if (mmd_is_codefence((char *)fileptr, '\0', 0, NULL))
    return (0);

  if (mmd_is_chars(fileptr, "- \t", 3) || mmd_is_chars(fileptr, "_ \t", 3) || mmd_is_chars(fileptr, "* \t", 3))
  {
   /*
    * Thematic break...
    */

    return (0);
  }

  if (mmd_is_chars(fileptr, "-", 1) || mmd_is_chars(fileptr, "=", 1))
  {
   /*
    * Heading...
    */

    return (0);
  }

  if (*fileptr == '#')
  {
   /*
    * Possible heading...
    */

    int count = 0;

    while (*fileptr == '#')
    {
      fileptr ++;
      count ++;
    }

    if (count <= 6)
      return (0);
  }

  return ((fileptr - file->bufptr) <= indent);
}


/*
 * 'mmd_is_chars()' - Determine whether a line consists solely of whitespace
 *                    and the specified character.
 */

static size_t				/* O - 1 if as specified, 0 otherwise */
mmd_is_chars(const char *lineptr,	/* I - Current line */
             const char *chars,		/* I - Non-space character */
             size_t     minchars)	/* I - Minimum number of non-space characters */
{
  size_t	found_ch = 0;		/* Did we find the specified characters? */

  while (*lineptr == *chars)
  {
    found_ch ++;
    lineptr ++;
  }

  if (minchars > 1)
  {
    while (*lineptr && strchr(chars, *lineptr))
    {
      if (*lineptr == *chars)
	found_ch ++;

      lineptr ++;
    }
  }

  while (*lineptr && isspace(*lineptr & 255) && *lineptr != '\n')
    lineptr ++;

  if ((*lineptr && *lineptr != '\n') || found_ch < minchars)
    return (0);
  else
    return (found_ch);
}


/*
 * 'mmd_is_codefence()' - Determine whether the line contains a code fence.
 */

static size_t				/* O - Length of fence or 0 otherwise */
mmd_is_codefence(char   *lineptr,	/* I - Line */
                 char   fence,		/* I - Current fence character, if any */
                 size_t fencelen,	/* I - Current fence length */
                 char   **language)	/* O - Language name, if any */
{
  char		match = fence;		/* Character to match */
  size_t	len = 0;		/* Length of fence chars */


  if (language)
    *language = NULL;

  if (!match)
  {
    if (*lineptr == '~' || *lineptr == '`')
      match = *lineptr;
    else
      return (0);
  }

  while (*lineptr == match)
  {
    lineptr ++;
    len ++;
  }

  if (len < 3 || (fencelen && len < fencelen))
    return (0);

  if (*lineptr && *lineptr != '\n' && fence)
    return (0);
  else if (*lineptr && *lineptr != '\n' && !fence)
  {
    if (strchr(lineptr, match))
      return (0);

    while (isspace(*lineptr & 255))
      lineptr ++;

    if (*lineptr && language)
    {
      *language = lineptr;

      while (*lineptr && !isspace(*lineptr & 255))
        lineptr ++;
      *lineptr = '\0';
    }
  }

  return (len);
}


/*
 * 'mmd_is_table()' - Look ahead to see if the next line contains a heading
 *                    divider for a table.
 */

static int				/* O - 1 if this is a table, 0 otherwise */
mmd_is_table(_mmd_filebuf_t *file)	/* I - File to read from */
{
  const char	*ptr;			/* Pointer into buffer */


  for (ptr = file->bufptr; *ptr; ptr ++)
  {
    if (*ptr == '>' && ptr == file->bufptr)
      continue;
    else if (!strchr(" \t\n\r:-|", *ptr))
      break;
  }

  return (!*ptr);
}


/*
 * 'mmd_parse_inline()' - Parse inline formatting.
 */

static void
mmd_parse_inline(_mmd_doc_t *doc,	/* I - Document */
		 mmd_t      *parent,	/* I - Parent node */
		 char       *lineptr)	/* I - Pointer into line */
{
  mmd_t		*node;			/* New node */
  mmd_type_t    type;                   /* Current node type */
  int           whitespace;             /* Whitespace precedes? */
  char          *text,                  /* Text fragment in line */
                *url,                   /* URL in link */
                *refname;		/* Reference name */
  const char	*delim = NULL;		/* Delimiter */
  size_t	delimlen = 0;		/* Length of delimiter */


  whitespace = parent->last_child != NULL;

  for (text = NULL, type = MMD_TYPE_NORMAL_TEXT; *lineptr; lineptr ++)
  {
    DEBUG2_printf("mmd_parse_inline: lineptr=\"%s\", type=%d, text=%p, whitespace=%d\n", lineptr, type, text, whitespace);

    if (isspace(*lineptr & 255) && type != MMD_TYPE_CODE_TEXT)
    {
      if (text)
      {
        *lineptr = '\0';
        mmd_add(parent, type, whitespace, text, NULL);
        text = NULL;
      }

      whitespace = 1;

      if (!strcmp(lineptr + 1, " \n"))
      {
        DEBUG2_printf("mmd_parse_inline: Adding hard break to %p(%d)\n", parent, parent->type);
        mmd_add(parent, MMD_TYPE_HARD_BREAK, 0, NULL, NULL);
      }
    }
    else if (*lineptr == '!' && lineptr[1] == '[' && type != MMD_TYPE_CODE_TEXT)
    {
     /*
      * Image...
      */

      if (text)
      {
        mmd_add(parent, type, whitespace, text, NULL);

        text       = NULL;
        whitespace = 0;
      }

      lineptr = mmd_parse_link(doc, lineptr + 1, &text, &url, &refname);

      if (url || refname)
      {
        node = mmd_add(parent, MMD_TYPE_IMAGE, whitespace, text, url);

        if (refname)
          mmd_ref_add(doc, node, refname, NULL);
      }

      if (!*lineptr)
        return;

      text = url = NULL;
      whitespace = 0;
      lineptr --;
    }
    else if (*lineptr == '[' && type != MMD_TYPE_CODE_TEXT)
    {
     /*
      * Link...
      */

      if (text)
      {
        mmd_add(parent, type, whitespace, text, NULL);

        text       = NULL;
        whitespace = 0;
      }

      lineptr = mmd_parse_link(doc, lineptr, &text, &url, &refname);

      if (text && *text == '`')
      {
        char *end = text + strlen(text) - 1;

        text ++;
        if (end > text && *end == '`')
          *end = '\0';

        node = mmd_add(parent, MMD_TYPE_CODE_TEXT, whitespace, text, url);
      }
      else if (text)
        node = mmd_add(parent, MMD_TYPE_LINKED_TEXT, whitespace, text, url);
      else
        node = NULL;

      if (refname && node)
        mmd_ref_add(doc, node, refname, NULL);

      if (!*lineptr)
        return;

      text = url = NULL;
      whitespace = 0;
      lineptr --;
    }
    else if (*lineptr == '<' && type != MMD_TYPE_CODE_TEXT && strchr(lineptr + 1, '>'))
    {
     /*
      * Autolink...
      */

      *lineptr++ = '\0';

      if (text)
      {
        mmd_add(parent, type, whitespace, text, NULL);

        text       = NULL;
        whitespace = 0;
      }

      url      = lineptr;
      lineptr  = strchr(lineptr, '>');
      *lineptr = '\0';

      mmd_add(parent, MMD_TYPE_LINKED_TEXT, whitespace, url, url);

      text = url = NULL;
      whitespace = 0;
    }
    else if ((*lineptr == '*' || *lineptr == '_') && (!text || ispunct(lineptr[-1] & 255) || type != MMD_TYPE_NORMAL_TEXT) && type != MMD_TYPE_CODE_TEXT)
    {
      if (type != MMD_TYPE_NORMAL_TEXT || !delim)
      {
	if (!strncmp(lineptr, "**", 2))
	  delim = "**";
	else if (!strncmp(lineptr, "__", 2))
	  delim = "__";
	else if (*lineptr == '*')
	  delim = "*";
	else
	  delim = "_";

	delimlen = strlen(delim);
      }

      if (type == MMD_TYPE_NORMAL_TEXT && delim && !strstr(lineptr + delimlen, delim))
      {
        if (!text)
          text = lineptr;

        delim    = NULL;
        delimlen = 0;
        continue;
      }

      if (text)
      {
        char save = *lineptr;

        *lineptr = '\0';

        mmd_add(parent, type, whitespace, text, NULL);

        *lineptr   = save;
        text       = NULL;
        whitespace = 0;
      }

      if (type == MMD_TYPE_NORMAL_TEXT)
      {
        if (!strncmp(lineptr, delim, delimlen) && !isspace(lineptr[delimlen] & 255))
        {
          type = delimlen == 2 ? MMD_TYPE_STRONG_TEXT : MMD_TYPE_EMPHASIZED_TEXT;
	  text = lineptr + delimlen;
          lineptr += delimlen - 1;
        }
        else
        {
	  text = lineptr;
        }
      }
      else if (!strncmp(lineptr, delim, delimlen))
      {
        lineptr += delimlen - 1;
        type = MMD_TYPE_NORMAL_TEXT;

        delim    = NULL;
        delimlen = 0;
      }
    }
    else if (lineptr[0] == '~' && lineptr[1] == '~' && type != MMD_TYPE_CODE_TEXT)
    {
      if (text)
      {
        *lineptr = '\0';

        mmd_add(parent, type, whitespace, text, NULL);

        *lineptr   = '~';
        text       = NULL;
        whitespace = 0;
      }

      if (!isspace(lineptr[2] & 255) && type == MMD_TYPE_NORMAL_TEXT)
      {
	type = MMD_TYPE_STRUCK_TEXT;
        text = lineptr + 2;
      }
      else
      {
	lineptr ++;
        type = MMD_TYPE_NORMAL_TEXT;
      }
    }
    else if (*lineptr == '`')
    {
      if (type != MMD_TYPE_NORMAL_TEXT || !delim)
      {
        if (lineptr[1] == '`')
        {
          if (lineptr[2] == '`')
	  {
	    delim    = "```";
	    delimlen = 3;
	  }
	  else
	  {
	    delim    = "``";
	    delimlen = 2;
	  }
	}
        else
        {
          delim    = "`";
          delimlen = 1;
        }
      }

      if (type != MMD_TYPE_CODE_TEXT && delim && !strstr(lineptr + delimlen, delim))
      {
        if (!text)
          text = lineptr;

        delim    = NULL;
        delimlen = 0;
        continue;
      }

      if (text)
      {
        if (!strncmp(lineptr, delim, delimlen))
        {
          char	*textptr = lineptr;

          while (textptr > text && isspace(textptr[-1] & 255))
            textptr --;

	  *textptr = '\0';
	}

        if (type == MMD_TYPE_CODE_TEXT)
        {
          if (whitespace && !*text)
            mmd_add(parent, type, 0, " ", NULL);

          whitespace = 0;
        }

        mmd_add(parent, type, whitespace, text, NULL);

        text       = NULL;
        whitespace = 0;
      }

      if (type == MMD_TYPE_CODE_TEXT)
      {
        if (!strncmp(lineptr, delim, delimlen))
        {
          type     = MMD_TYPE_NORMAL_TEXT;
          lineptr += delimlen - 1;
          delim    = NULL;
          delimlen = 0;
        }
      }
      else
      {
        type    = MMD_TYPE_CODE_TEXT;
        lineptr += delimlen - 1;

        if (isspace(lineptr[1] & 255))
        {
          whitespace = 1;

          while (isspace(lineptr[1] & 255))
            lineptr ++;
        }

        text = lineptr + 1;
      }
    }
    else if (!text)
    {
      if (*lineptr == '\\' && lineptr[1] && lineptr[1] != '\n')
      {
       /*
        * Escaped character...
        */

        lineptr ++;
      }

      text = lineptr;
    }
    else if (*lineptr == '\\' && lineptr[1] && lineptr[1] != '\n')
    {
     /*
      * Escaped character...
      */

      memmove(lineptr, lineptr + 1, strlen(lineptr));
    }
  }

  if (text)
    mmd_add(parent, type, whitespace, text, NULL);
}


/*
 * 'mmd_parse_link()' - Parse a link.
 */

static char *				/* O - End of link text */
mmd_parse_link(_mmd_doc_t *doc,		/* I - Document */
               char       *lineptr,	/* I - Pointer into line */
               char       **text,	/* O - Text */
               char       **url,	/* O - URL */
               char       **refname)	/* O - Reference name */
{
  lineptr ++; /* skip "[" */

  *text    = lineptr;
  *url     = NULL;
  *refname = NULL;

  while (*lineptr && *lineptr != ']')
  {
    if (*lineptr == '\"')
    {
      lineptr ++;
      while (*lineptr && *lineptr != '\"')
        lineptr ++;

      if (!*lineptr)
        return (lineptr);
    }

    lineptr ++;
  }

  if (!*lineptr)
  {
    return (lineptr);
  }

  *lineptr++ = '\0';

  if (isspace(*lineptr & 255))
  {
   /*
    * Shortcut reference...
    */

    *refname = *text;
    return (lineptr);
  }
  else if (*lineptr == '(')
  {
   /*
    * Get URL...
    */

    lineptr ++;
    *url = lineptr;

    while (*lineptr && *lineptr != ')')
    {
      if (isspace(*lineptr & 255))
        *lineptr = '\0';
      else if (*lineptr == '\"')
      {
        lineptr ++;
        while (*lineptr && *lineptr != '\"')
          lineptr ++;

        if (!*lineptr)
          return (lineptr);
      }

      lineptr ++;
    }

    *lineptr++ = '\0';
  }
  else if (*lineptr == '[')
  {
   /*
    * Get reference...
    */

    lineptr ++;
    *refname = lineptr;

    while (*lineptr && *lineptr != ']')
    {
      if (isspace(*lineptr & 255))
        *lineptr = '\0';
      else if (*lineptr == '\"')
      {
        lineptr ++;
        while (*lineptr && *lineptr != '\"')
          lineptr ++;

        if (!*lineptr)
          return (lineptr);
      }

      lineptr ++;
    }

    *lineptr++ = '\0';
    if (!**refname)
      *refname = *text;
  }
  else if (*lineptr == ':')
  {
   /*
    * Get reference definition...
    */

    lineptr ++;
    while (*lineptr && isspace(*lineptr & 255))
      lineptr ++;

    *url = lineptr;

    while (*lineptr && !isspace(*lineptr & 255))
      lineptr ++;

    *lineptr = '\0';

    mmd_ref_add(doc, NULL, *text, *url);

    *text = NULL;
    *url  = NULL;
  }

  return (lineptr);
}


/*
 * 'mmd_read_buffer()' - Fill the file buffer with more data from a file.
 */

static void
mmd_read_buffer(_mmd_filebuf_t *file)	/* I - File buffer */
{
  size_t	bytes;			/* Bytes read */


  if (file->bufptr && file->bufptr > file->buffer)
  {
   /*
    * Discard previous characters in the buffer.
    */

    memmove(file->buffer, file->bufptr, file->bufend - file->bufptr);
    file->bufend -= (file->bufptr - file->buffer);
  }
  else
  {
   /*
    * Otherwise just clear the buffer...
    */

    file->bufend = file->buffer;
  }

  if ((bytes = fread(file->bufend, 1, sizeof(file->buffer) - (size_t)(file->bufend - file->buffer - 1), file->fp)) > 0)
    file->bufend += bytes;

  *(file->bufend) = '\0';
  file->bufptr = file->buffer;
}


/*
 * 'mmd_read_line()' - Read a line from a file in a Markdown-aware way.
 */

static char *				/* O - Pointer to line or `NULL` on EOF */
mmd_read_line(_mmd_filebuf_t *file,	/* I - File buffer */
              char           *line,	/* I - Line buffer */
              size_t         linesize)	/* I - Size of line buffer */
{
  int	ch,				/* Current character */
	column = 0;			/* Current column */
  char	*lineptr = line,		/* Pointer into line */
	*lineend = line + linesize - 1;	/* Pointer to end of buffer */


 /*
  * Fill the buffer as needed...
  */

  if (!file->bufptr || (file->bufptr >= file->bufend) || !strchr(file->bufptr, '\n'))
    mmd_read_buffer(file);

 /*
  * Copy a line out of the file buffer...
  */

  while (file->bufptr < file->bufend)
  {
    ch = *(file->bufptr);
    file->bufptr ++;

    if (ch == '\t')
    {
     /*
      * Expand tabs since nobody uses the same tab width and Markdown says
      * 4 columns per tab...
      */

      do
      {
        column ++;
        if (lineptr < lineend)
          *lineptr++ = ' ';
      }
      while (column  & 3);
    }
    else if (ch != '\r' && lineptr < lineend)
    {
      column ++;
      *lineptr++ = ch;
    }

    if (ch == '\n')
      break;
  }

  *lineptr = '\0';

  if (file->bufptr == file->bufend && lineptr == line)
    return (NULL);
  else if (!strchr(file->bufptr, '\n'))
    mmd_read_buffer(file);

  return (line);
}


/*
 * 'mmd_ref_add()' - Add or update a reference...
 */

static void
mmd_ref_add(_mmd_doc_t *doc,		/* I - Document */
            mmd_t      *node,		/* I - Link node, if any */
            const char *name,		/* I - Reference name */
            const char *url)		/* I - Reference URL */
{
  size_t	i;			/* Looping var */
  _mmd_ref_t	*ref = mmd_ref_find(doc, name);
					/* Reference */


  if (ref)
  {
    if (!ref->url && url)
    {
      if (node)
        node->url = strdup(url);

      ref->url = strdup(url);

      for (i = 0; i < ref->num_pending; i ++)
        ref->pending[i]->url = strdup(url);

      free(ref->pending);

      ref->num_pending = 0;
      ref->pending     = NULL;
      return;
    }
  }
  else if ((ref = realloc(doc->references, (doc->num_references + 1) * sizeof(_mmd_ref_t))) != NULL)
  {
    doc->references = ref;
    ref += doc->num_references;
    doc->num_references ++;

    ref->name        = strdup(name);
    ref->url         = url ? strdup(url) : NULL;
    ref->num_pending = 0;
    ref->pending     = NULL;
  }
  else
    return;

  if (node)
  {
    if (ref->url)
      node->url = strdup(ref->url);
    else if ((ref->pending = realloc(ref->pending, (ref->num_pending + 1) * sizeof(mmd_t *))) != NULL)
      ref->pending[ref->num_pending ++] = node;
  }
}


/*
 * 'mmd_ref_find()' - Find a reference...
 */

static _mmd_ref_t *			/* O - Reference or NULL */
mmd_ref_find(_mmd_doc_t *doc,		/* I - Document */
             const char *name)		/* I - Reference name */
{
  size_t	i;			/* Looping var */


  for (i = 0; i < doc->num_references; i ++)
    if (!strcasecmp(name, doc->references[i].name))
      return (doc->references + i);

  return (NULL);
}


/*
 * 'mmd_remove()' - Remove a node from its parent.
 */

static void
mmd_remove(mmd_t *node)                 /* I - Node */
{
  if (node->parent)
  {
    if (node->prev_sibling)
      node->prev_sibling->next_sibling = node->next_sibling;
    else
      node->parent->first_child = node->next_sibling;

    if (node->next_sibling)
      node->next_sibling->prev_sibling = node->prev_sibling;
    else
      node->parent->last_child = node->prev_sibling;

    node->parent       = NULL;
    node->prev_sibling = NULL;
    node->next_sibling = NULL;
  }
}


#if DEBUG
/*
 * 'mmd_type_string()' - Return a string for the specified type enumeration.
 */

static const char *			/* O - String representing the type */
mmd_type_string(mmd_type_t type)	/* I - Type value */
{
  static char	unknown[64];		/* Unknown type buffer */


  switch (type)
  {
    case MMD_TYPE_NONE :
        return ("MMD_TYPE_NONE");
    case MMD_TYPE_DOCUMENT :
        return "MMD_TYPE_DOCUMENT";
    case MMD_TYPE_METADATA :
        return "MMD_TYPE_METADATA";
    case MMD_TYPE_BLOCK_QUOTE :
        return "MMD_TYPE_BLOCK_QUOTE";
    case MMD_TYPE_ORDERED_LIST :
        return "MMD_TYPE_ORDERED_LIST";
    case MMD_TYPE_UNORDERED_LIST :
        return "MMD_TYPE_UNORDERED_LIST";
    case MMD_TYPE_LIST_ITEM :
        return "MMD_TYPE_LIST_ITEM";
    case MMD_TYPE_TABLE :
        return "MMD_TYPE_TABLE";
    case MMD_TYPE_TABLE_HEADER :
        return "MMD_TYPE_TABLE_HEADER";
    case MMD_TYPE_TABLE_BODY :
        return "MMD_TYPE_TABLE_BODY";
    case MMD_TYPE_TABLE_ROW :
        return "MMD_TYPE_TABLE_ROW";
    case MMD_TYPE_HEADING_1 :
        return "MMD_TYPE_HEADING_1";
    case MMD_TYPE_HEADING_2 :
        return "MMD_TYPE_HEADING_2";
    case MMD_TYPE_HEADING_3 :
        return "MMD_TYPE_HEADING_3";
    case MMD_TYPE_HEADING_4 :
        return "MMD_TYPE_HEADING_4";
    case MMD_TYPE_HEADING_5 :
        return "MMD_TYPE_HEADING_5";
    case MMD_TYPE_HEADING_6 :
        return "MMD_TYPE_HEADING_6";
    case MMD_TYPE_PARAGRAPH :
        return "MMD_TYPE_PARAGRAPH";
    case MMD_TYPE_CODE_BLOCK :
        return "MMD_TYPE_CODE_BLOCK";
    case MMD_TYPE_THEMATIC_BREAK :
        return "MMD_TYPE_THEMATIC_BREAK";
    case MMD_TYPE_TABLE_HEADER_CELL :
        return "MMD_TYPE_TABLE_HEADER_CELL";
    case MMD_TYPE_TABLE_BODY_CELL_LEFT :
        return "MMD_TYPE_TABLE_BODY_CELL_LEFT";
    case MMD_TYPE_TABLE_BODY_CELL_CENTER :
        return "MMD_TYPE_TABLE_BODY_CELL_CENTER";
    case MMD_TYPE_TABLE_BODY_CELL_RIGHT :
        return "MMD_TYPE_TABLE_BODY_CELL_RIGHT";
    case MMD_TYPE_NORMAL_TEXT :
        return "MMD_TYPE_NORMAL_TEXT";
    case MMD_TYPE_EMPHASIZED_TEXT :
        return "MMD_TYPE_EMPHASIZED_TEXT";
    case MMD_TYPE_STRONG_TEXT :
        return "MMD_TYPE_STRONG_TEXT";
    case MMD_TYPE_STRUCK_TEXT :
        return "MMD_TYPE_STRUCK_TEXT";
    case MMD_TYPE_LINKED_TEXT :
        return "MMD_TYPE_LINKED_TEXT";
    case MMD_TYPE_CODE_TEXT :
        return "MMD_TYPE_CODE_TEXT";
    case MMD_TYPE_IMAGE :
        return "MMD_TYPE_IMAGE";
    case MMD_TYPE_HARD_BREAK :
        return "MMD_TYPE_HARD_BREAK";
    case MMD_TYPE_SOFT_BREAK :
        return "MMD_TYPE_SOFT_BREAK";
    case MMD_TYPE_METADATA_TEXT :
        return "MMD_TYPE_METADATA_TEXT";
    default :
        snprintf(unknown, sizeof(unknown), "?? %d ??", (int)type);
        return (unknown);
  }
}
#endif /* DEBUG */
