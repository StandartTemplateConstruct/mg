/**************************************************************************
 *
 * filename -- description
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
 * $Id: term_lists.c,v 1.1 1994/10/20 03:57:07 tes Exp $
 *
 **************************************************************************/

/*
   $Log: term_lists.c,v $
   * Revision 1.1  1994/10/20  03:57:07  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
 */

static char *RCSID = "$Id: term_lists.c,v 1.1 1994/10/20 03:57:07 tes Exp $";

#include "config.h"
#include "sysfuncs.h"

#include "memlib.h"
#include "local_strings.h"
#include "term_lists.h"
#include "messages.h"

TermList *query_term_list = NULL;

/* =========================================================================
 * Function: MakeTermList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */
TermList *
MakeTermList (int n)
{
  TermList *t;
  int list_size = (n == 0 ? 1 : n);	/* always allocate at least one node */

  t = Xmalloc (sizeof (TermList) + (list_size - 1) * sizeof (TermEntry));
  if (!t)
    FatalError (1, "Unable to allocate term list");

  t->num = n;
  t->list_size = list_size;

  return t;
}

/* =========================================================================
 * Function: ResizeTermList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */

#define GROWTH_FACTOR 2
#define MIN_SIZE 2

static void
ResizeTermList (TermList ** term_list)
{
  TermList *tl = *term_list;

  if (tl->num > tl->list_size)
    {
      if (tl->list_size)
	tl->list_size *= GROWTH_FACTOR;
      else
	tl->list_size = MIN_SIZE;
    }
  tl = Xrealloc (tl, sizeof (TermList) + (tl->list_size - 1) * sizeof (TermEntry));

  if (!tl)
    FatalError (1, "Unable to resize term list");

  *term_list = tl;
}

/* =========================================================================
 * Function: ConvertTermsToString
 * Description:
 *      Convert term list into null-terminated string
 * Input:
 *      query_term_list = term list
 * Output:
 *      str = term string
 * ========================================================================= */

void
ConvertTermsToString (TermList * query_term_list, char *str)
{
  int i = 0;
  int total_len = 0;

  /* terms_str should be preallocated */
  if (!str)
    return;

  for (i = 0; i < query_term_list->num; i++)
    {
      unsigned char *word = query_term_list->TE[i].Word;
      int len = word[0];
      total_len += len + 1;	/* +1 for space */
      if (total_len > MAXTERMSTRLEN)
	break;
      strncpy (str, (char *) word + 1, len);
      str += len;
      if (i != (query_term_list->num) - 1)
	{
	  *str = ' ';
	  str++;		/* add space gap */
	}

    }
  *str = '\0';
}

/* =========================================================================
 * Function: ResetTermList
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

void
ResetTermList (TermList ** tl)
{
  if (*tl)
    FreeTermList (tl);
  *tl = MakeTermList (0);
}

/* =========================================================================
 * Function: AddTermEntry
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
AddTermEntry (TermList ** query_term_list, TermEntry * te)
{
  TermList *tl = *query_term_list;

  tl->num++;
  ResizeTermList (query_term_list);
  tl = *query_term_list;

  /* copy the structure contents */
  bcopy ((char *) te, (char *) &(tl->TE[tl->num - 1]), sizeof (TermEntry));

  return tl->num - 1;
}



/* =========================================================================
 * Function: AddTerm
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
AddTerm (TermList ** query_term_list, u_char * Word)
{
  int j;
  TermList *tl = *query_term_list;

  /* Look for the word in the already identified terms */
  for (j = 0; j < tl->num; j++)
    {
      TermEntry *te = &(tl->TE[j]);
      if (compare (te->Word, Word) == 0)
	{
	  te->Count++;
	  return j;
	}
    }


  {
    /* Create a new entry in the list for the new word */
    TermEntry te;

    te.WE.word_num = 0;
    te.WE.count = 0;
    te.WE.doc_count = 0;
    te.WE.invf_ptr = 0;
    te.WE.invf_len = 0;
    te.Count = 1;
    te.Word = copy_string (Word);
    if (!te.Word)
      FatalError (1, "Could NOT create memory to add term");

    return AddTermEntry (query_term_list, &te);
  }

}

/* =========================================================================
 * Function: FreeTermList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */

void
FreeTermList (TermList ** the_tl)
{
  int j;
  TermList *tl = *the_tl;

  for (j = 0; j < tl->num; j++)
    if (tl->TE[j].Word)
      Xfree (tl->TE[j].Word);
  Xfree (tl);

  *the_tl = NULL;
}

/* =========================================================================
 * Function: PrintWordEntry
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

void
PrintWordEntry (WordEntry * we, FILE * file)
{
  fprintf (file, "we->word_num = %d\n", we->word_num);
  fprintf (file, "we->count = %ld\n", we->count);
  fprintf (file, "we->doc_count = %ld\n", we->doc_count);
  fprintf (file, "we->invf_ptr = %ld\n", we->invf_ptr);
  fprintf (file, "we->invf_len = %ld\n", we->invf_len);
}

/* =========================================================================
 * Function: PrintTermEntry
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

void
PrintTermEntry (TermEntry * te, FILE * file)
{

  fprintf (file, "Term Entry\n");
  fprintf (file, "te->Count = %d\n", te->Count);
  fprintf (file, "te->Word = %s\n", str255_to_string (te->Word, NULL));
  PrintWordEntry (&(te->WE), file);

}

/* =========================================================================
 * Function: PrintTermList
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

void
PrintTermList (TermList * tl, FILE * file)
{
  int i;

  fprintf (file, "Term List\n");
  fprintf (file, "tl->list_size = %d\n", tl->list_size);
  fprintf (file, "tl->num = %d\n", tl->num);

  for (i = 0; i < tl->num; i++)
    {
      fprintf (file, "[%d]\n", i);
      PrintTermEntry (&(tl->TE[i]), file);
    }
}
