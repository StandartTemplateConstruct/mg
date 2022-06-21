/**************************************************************************
 *
 * mg_hilite_words -- display text and highlight particular words which 
 *                    it contains
 * Copyright (C) 1994 Tim Shimmin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: mg_hilite_words.c,v 1.3 1995/04/21 01:35:30 tes Exp $
 *
 **************************************************************************/

static char *RCSID = "$Id: mg_hilite_words.c,v 1.3 1995/04/21 01:35:30 tes Exp $";

#include "sysfuncs.h"

#include "getopt.h"
#include "messages.h"
#include "local_strings.h"
#include "stemmer.h"
#include "words.h"


/*
 * Description
 * -----------
 *   Hilite_words reads text from stdin and outputs it into a pager such as
 *   less. It's command arguments include a list of stemmed words 
 *   which if extracted from the text will be highlighted on the output.
 *
 * Implementation
 * --------------
 *   Extracting of words - using PARSE_STEM_WORD & stemmer
 *   
 *   Highlighting of words - for standard pager:
 *                             using back-space character code
 *                             e.g. bolded a = "a\ba", underlined a = "_\ba"
 *                         - for pager==html:
 *                             use some of the standard html character 
 *                             formatting tags
 *
 *   Storage of words - using hashtable (set) of size equalling a constant
 *                      times the number of words
 * 
 * Usage
 * -----
 *   mg_hilite_words --stem_method [0-3] 
 *                   --style [bold|underline|italic|emphasis|strong]
 *                   --pager [less|more|html|???]
 *                   --terminator [terminator-string]
 *                   list-of-words-to-highlight
 */

/*
 * Modifications:
 *
 * 21/Apr/95: To handle outputting html tags to stdout
 *
 */

/* --- constants --- */

/* highlighting styles */
#define HILITE_MAX      5	/* the number of styles */
#define BOLD 		0
#define UNDERLINE	1
#define ITALIC     	2
#define EMPHASIS     	3
#define STRONG     	4

/* maximum length of line buffer */
#define MAX_LINE_BUFFER 200

/* set pager to this to get html hilited text to stdout */
#define HTML_OUT "html"

/* output types */
#define PAGER 0
#define HTML  1

/* --- types --- */

typedef u_char *Word;

/* --- globals --- */

/* keep in synch with constants */
static char *hilite_names[] =
{"bold", "underline", "italic", "emphasis", "strong"};
static char *hilite_tags[] =
{"B", "", "I", "EM", "STRONG"};
static short hilite_style = BOLD;
static char *pager = "less";
static int stem_method = 3;	/* fold & stem */
static char **word_list;
static int num_of_words = 0;
static int output_type = PAGER;
static char *terminator = NULL;

/* --- prototypes --- */

static Word copy_c_word (char *w);
static void process_words (char **words, int num_of_words);
static int get_line (u_char * line, int n, FILE * stream);
static void process_text (FILE * input_file, FILE * output_file);
static void copy_word (u_char * w1, u_char * w2);
static void process_buffer (u_char * s_in, int len, FILE * output_file);
static void output_word (u_char * s_start, u_char * s_finish, FILE * output_file);
static void output_hilite_word (u_char * s_start, u_char * s_finish,
				FILE * output_file);
static void process_args (int argc, char *argv[]);
static void print_line (u_char * line, int n, FILE * stream);


/******************************** word set ************************************/

#include "hash.h"
#include "locallib.h"

#define SIZE_FACTOR 2

typedef Word *WordSet;
static WordSet set_of_words = NULL;
static int hash_size = 0;

/* prototypes - set routines */
static void set_create (int num_of_words);
static void set_add (Word word);
static int set_member (Word word);
static void set_print (void);

/* =========================================================================
 * Function: set_print
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

static void
set_print (void)
{
  int i = 0;

  for (i = 0; i < hash_size; i++)
    {
      Word word = set_of_words[i];

      if (word)
	{
	  int len = *word++;

	  fprintf (stderr, "[%d] = ", i);
	  while (len--)
	    fputc (*word++, stderr);
	  fputc ('\n', stderr);
	}
    }

}


/* =========================================================================
 * Function: set_create
 * Description: 
 *      Allocate memory for the set
 * Input: 
 * Output: 
 * ========================================================================= */

static void
set_create (int num_of_words)
{
  WordSet set;

  hash_size = prime (num_of_words * SIZE_FACTOR);
  set = (WordSet) malloc (hash_size * sizeof (Word));
  if (!set)
    FatalError (1, "Runout of memory for word hashtable");
  bzero ((char *) set, sizeof (Word) * hash_size);

  set_of_words = set;

}

/* =========================================================================
 * Function: set_add
 * Description: 
 *      Add a string element to the set.
 * Input: 
 * Output: 
 * ========================================================================= */

static void
set_add (Word word)
{
  int hash_val;
  int hash_step;

  HASH (hash_val, hash_step, word, hash_size);

  /* loop around in case of collisions and need to step */
  while (1)
    {

      Word entry = set_of_words[hash_val];

      /* if doesn't exist then */
      if (!entry)
	{
	  set_of_words[hash_val] = word;
	  break;
	}

      /* if we have a matching word */
      if (compare (entry, word) == 0)
	break;

      /* if collides with a different word */
      hash_val = (hash_val + hash_step) % hash_size;

    }
}


/* =========================================================================
 * Function: set_member
 * Description: 
 *      Tests whether a string is a member of the set
 * Input: 
 * Output: 
 * ========================================================================= */

static int
set_member (Word word)
{
  int hash_val;
  int hash_step;

  HASH (hash_val, hash_step, word, hash_size);

  /* loop around in case of collisions and need to step */
  while (1)
    {

      Word entry = set_of_words[hash_val];

      /* if doesn't exist then */
      if (!entry)
	return 0;

      /* if we have a matching word */
      if (compare (entry, word) == 0)
	return 1;

      /* if collides with a different word */
      hash_val = (hash_val + hash_step) % hash_size;

    }
}

/******************************** end of word set ******************************/



/* =========================================================================
 * Function: copy_c_word
 * Description: 
 *      Allocate enough memory and copy word over
 * Input: 
 *      w = null terminated string (c_word)
 * Output: 
 *      word with length in 1st byte
 * ========================================================================= */

static Word
copy_c_word (char *w)
{
  int len = strlen (w);
  Word w_copy = (Word) malloc (len + 1);
  Word w_ptr = NULL;
  int j = 0;

  if (!w_copy)
    FatalError (1, "Not enough memory to copy a word");

  w_copy[0] = len;
  w_ptr = w_copy + 1;
  for (j = 0; j < len; j++)
    *w_ptr++ = *w++;

  return w_copy;
}

/* =========================================================================
 * Function: process_words
 * Description: 
 *      Go through the stemmed words and add to word set
 * Input: 
 * Output: 
 * ========================================================================= */

static void
process_words (char **words, int num_of_words)
{
  int i = 0;

  set_create (num_of_words);

  for (i = 0; i < num_of_words; i++)
    {
      Word word = copy_c_word (words[i]);
      set_add (word);
    }

}

/* =========================================================================
 * Function: get_line
 * Description: 
 *      Equivalent of fgets for u_char*.
 *      But returns length of read-in line.
 *      Expects to see a '\n' before an EOF
 * Input: 
 * Output: 
 * ========================================================================= */

static int
get_line (u_char * line, int n, FILE * stream)
{
  int i = 0;
  int ch = '\0';

  while (1)
    {
      if (i == n)
	return i;

      ch = fgetc (stream);

      if (ch == EOF)
	{
	  if (!feof (stream))
	    FatalError (1, "Error on reading a line from stdin");
	  return EOF;
	}

      if (ch == '\n')
	return i;

      *line++ = ch;
      i++;
    }

}

/* =========================================================================
 * Function: print_line
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

static void
print_line (u_char * line, int n, FILE * stream)
{

  while (n--)
    {
      fputc (*line++, stream);
    }
  fputc ('\n', stream);

}

/* =========================================================================
 * Function: process_text
 * Description: 
 *      Go through the text from input_file and highlight to output_file 
 * Input: 
 * Output: 
 * ========================================================================= */

static void
process_text (FILE * input_file, FILE * output_file)
{
  static u_char line_buffer[MAX_LINE_BUFFER];


  while (1)
    {
      int len = get_line (line_buffer, MAX_LINE_BUFFER, input_file);

      if (len == EOF)
	break;
      process_buffer (line_buffer, len, output_file);

    }
}

/* =========================================================================
 * Function: copy_word
 * Description: 
 *      Copies w2 into w1. Assumes both have storage allocated.
 * Input: 
 * Output: 
 * ========================================================================= */

static void
copy_word (u_char * w1, u_char * w2)
{
  int i;
  int len = w2[0];

  for (i = 0; i <= len; i++)
    *w1++ = *w2++;

}

/* =========================================================================
 * Function: process_buffer
 * Description: 
 *      Parse & stem words of line buffer
 *      Based on the usage of PARSEing in other mg files.
 * Input: 
 * Output: 
 * ========================================================================= */

static void
process_buffer (u_char * s_in, int len, FILE * output_file)
{
  u_char *end = s_in + len - 1;
  u_char *s_start = NULL;

  if (!INAWORD (*s_in))
    {
      s_start = s_in;
      PARSE_NON_STEM_WORD (s_in, end);
      output_word (s_start, s_in - 1, output_file);
    }

  while (s_in <= end)
    {
      u_char word[MAXSTEMLEN + 1];

      s_start = s_in;
      PARSE_STEM_WORD (word, s_in, end);

      stemmer (stem_method, word);

      if (set_member (word))	/* output with highlighting */
	{
	  output_hilite_word (s_start, s_start + word[0] - 1, output_file);
	  s_start += word[0];	/* step over hilited output */
	}
      output_word (s_start, s_in - 1, output_file);

      s_start = s_in;
      PARSE_NON_STEM_WORD (s_in, end);
      output_word (s_start, s_in - 1, output_file);

    }				/*while */

  fputc ('\n', output_file);
  fflush (output_file);

}				/*process_buffer */

/* =========================================================================
 * Function: output_word
 * Description: 
 *      Output a word which lies from s_start to s_finish in buffer
 * Input: 
 *      s_start = ptr to 1st char
 *      s_finish = ptr to last char
 * Output: 
 * ========================================================================= */

static void
output_word (u_char * s_start, u_char * s_finish, FILE * output_file)
{
  while (s_start <= s_finish)
    {
      fputc (*s_start++, output_file);
    }
}


/* =========================================================================
 * Function: output_hilite_word
 * Description: 
 *      Highlight a word (with length in 1st byte)
 *      Pager highlighting:
 *        Highlighting is either by bolding or underlining using
 *        the method used by UNIX utilities More(1) and Less(1)
 *      HTML highlighting:
 *        use the appropriate start and end tags around the word
 * ========================================================================= */

static void
output_hilite_word (u_char * s_start, u_char * s_finish, FILE * output_file)
{

  if (output_type == HTML)
    {
      char *hilite_tag = hilite_tags[hilite_style];

      /* print start tag */
      fprintf (output_file, "<%s>", hilite_tag);

      output_word (s_start, s_finish, output_file);

      /* print end tag */
      fprintf (output_file, "</%s>", hilite_tag);
    }

  else
    /* PAGER */
    {
      /* use backspaces around each letter */
      while (s_start <= s_finish)
	{
	  switch (hilite_style)
	    {
	    case BOLD:
	      fputc (*s_start, output_file);
	      fputc ('\b', output_file);
	      fputc (*s_start, output_file);
	      break;
	    case UNDERLINE:
	      fputc ('_', output_file);
	      fputc ('\b', output_file);
	      fputc (*s_start, output_file);
	      break;
	    default:
	      fputc (*s_start, output_file);
	    }
	  s_start++;
	}			/*while */
    }
}

/* =========================================================================
 * Function: process_args
 * Description: 
 *      sets the global variables:
 *              hilite_style, pager, num_of_words, word_list
 * Input: 
 * Output: 
 * ========================================================================= */

struct option long_opts[] =
{
  {"style", required_argument, 0, 's'},
  {"terminator", required_argument, 0, 't'},
  {"pager", required_argument, 0, 'p'},
  {"stem_method", required_argument, 0, 'm'},
  {0, 0, 0, 0}
};

static void
process_args (int argc, char *argv[])
{
  int ch;


  opterr = 0;
  while ((ch = getopt_long (argc, argv, "s:p:t:m:", long_opts, (int *) 0)) != -1)
    {
      switch (ch)
	{
	case 's':
	  {
	    int i;
	    for (i = 0; i < HILITE_MAX; i++)
	      if (strcmp (optarg, hilite_names[i]) == 0)
		break;

	    if (i < HILITE_MAX)
	      hilite_style = i;
	  }
	  break;
	case 't':
	  terminator = optarg;
	  break;
	case 'm':
	  stem_method = atoi (optarg);
	  break;
	case 'p':
	  pager = optarg;
	  break;
	default:
	  FatalError (1, "Usage: \n"
		      "mg_hilite_words --stem_method [0-3]\n"
	  "                --style [bold|underline|italic|emphasis|strong]\n"
		      "                --pager [less|more|html|???]\n");
	}
    }

  num_of_words = argc - optind;

  word_list = &argv[optind];

  /* fix up output type */
  if (strcmp (pager, HTML_OUT) == 0)
    output_type = HTML;
  else
    output_type = PAGER;

}

/* =========================================================================
 * Function: main
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
main (int argc, char *argv[])
{
  FILE *output = NULL;

  process_args (argc, argv);

  /* set output file */
  if (output_type == PAGER)
    output = popen (pager, "w");
  else
    output = stdout;

  if (!output)
    FatalError (1, "Unable to run \"%s\"\n", pager);

  if (num_of_words < 1)
    {
      int ch;

      /* just echo the input */
      /* better not to call this program at all ;-) */
      while ((ch = fgetc (stdin)) != EOF)
	{
	  fputc (ch, output);
	}
    }
  else
    {
      /* set up hash table for words */
      process_words (word_list, num_of_words);


      /* Go thru lines of text from stdin and 
       * output words with hilite info if
       * words parse into existence in the hash table 
       */
      process_text (stdin, output);
    }

  if (terminator)
    fprintf (output, "%s\n", terminator);

  if (output != stdout)
    pclose (output);

  exit(0);
}
