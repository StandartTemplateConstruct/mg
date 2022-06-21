/**************************************************************************
 *
 * bool_tester -- used to test bool_tree, bool_parser, bool_optimiser
 * Copyright (C) 1994  Authors
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
 * $Id: bool_tester.c,v 1.2 1995/03/14 05:15:23 tes Exp $
 *
 **************************************************************************/

/*
   $Log: bool_tester.c,v $
   * Revision 1.2  1995/03/14  05:15:23  tes
   * Updated the boolean query optimiser to do different types of optimisation.
   * A query environment variable "optimise_type" specifies which one is to be
   * used. Type 1 is the new one which is faster than 2.
   *
   * Revision 1.2  1994/10/18  06:11:05  tes
   * The boolean optimiser seems to be modifying the parse tree
   * like it is supposed to.
   * Paragraph ranking now works without any text files if required to.
   *
   * Revision 1.1  1994/10/12  01:15:33  tes
   * Found bugs in the existing boolean query optimiser.
   * So decided to rewrite it.
   * I accidentally deleted query.bool.y, but I have replaced it
   * with bool_parser.y (which I have forgotten to add here ! ;-(
   *
 */

static char *RCSID = "$Id: bool_tester.c,v 1.2 1995/03/14 05:15:23 tes Exp $";

#include "sysfuncs.h"

#include "local_strings.h"
#include "term_lists.h"
#include "bool_tree.h"
#include "bool_parser.h"
#include "bool_optimiser.h"

#define MAX_LINE_LEN  255
#define STEM_METHOD 3

static FILE *file_in;
static FILE *file_out;
static char line[MAX_LINE_LEN + 1];

/* --- prototypes --- */
static char *prompt (char *str);


/* =========================================================================
 * Function: main
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
main (int argc, char *argv[])
{
  bool_tree_node *tree = NULL;
  TermList *term_list = NULL;
  int opt_type = 0;

  file_in = stdin;
  file_out = stdout;
  while (1)
    {
      int res = 0;
      int len = 0;

      /* get a line */
      if (!prompt ("Please enter boolean expression..."))
	break;

      len = strlen (line) - 1;	/* -1 => ignore the \n */

      tree = ParseBool (line, len, &term_list, STEM_METHOD, &res);

      {
	int done = 0;
	while (!done)
	  {
	    if (!prompt ("Which type of optimisation 1 or 2 ?"))
	      break;
	    opt_type = atoi (line);
	    done = (opt_type == 1 || opt_type == 2);
	  }
      }

      if (!prompt ("Do you want to assign doc_counts to terms ?"))
	break;

      if (toupper (line[0]) == 'Y')
	{
	  /* cycle thru terms asking for doc_counts */
	  int i;
	  for (i = 0; i < term_list->num; i++)
	    {
	      char str[80];
	      TermEntry *te = &(term_list->TE[i]);

	      sprintf (str, "Please enter doc count for term '%s'",
		       str255_to_string (te->Word, NULL));
	      prompt (str);
	      te->WE.doc_count = atoi (line);
	      printf ("doc_count = %ld\n", te->WE.doc_count);
	    }			/*for */
	}			/*if */


      if (res == 0)
	{
	  fprintf (file_out, "\n***Parsed Expression***\n");
	  PrintBoolTree (tree, file_out);
	  fputc ('\n', file_out);
	  OptimiseBoolTree (tree, term_list, opt_type);
	  fprintf (file_out, "\n***Optimised Expression ***\n");
	  PrintBoolTree (tree, file_out);
	  fputc ('\n', file_out);
	}

    }
  exit(0);
}

/* =========================================================================
 * Function: 
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

static char *
prompt (char *str)
{
  fprintf (file_out, "\n%s\n", str);
  return fgets (line, MAX_LINE_LEN, file_in);
}
