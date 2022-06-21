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
 * $Id: term_lists.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:24  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:52  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:43  rjmcnab
   *** empty log message ***

   * Revision 1.1  1994/10/20  03:57:07  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
 */

static char *RCSID = "$Id: term_lists.c 16583 2008-07-29 10:20:36Z davidb $";

#include "sysfuncs.h"

#include "memlib.h"
#include "local_strings.h"
#include "term_lists.h"
#include "messages.h"
#include "stemmer.h"

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
 * Description: Used in boolean parser - see bool_tree   [RPAP - Feb 97: Term Frequency] 
 * Input: 
 * Output: 
 * ========================================================================= */

int
AddTerm (TermList ** query_term_list, u_char * Word, int Count, int word_num,
	 u_long count, u_long doc_count, u_long invf_ptr, u_long invf_len, int stemmer_num)  /* [RPAP - Feb 97: Term Frequency] */
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

    /* [RPAP - Feb 97: Term Frequency] */
    te.WE.word_num = word_num;
    te.WE.count = count;
    te.WE.doc_count = doc_count;
    te.WE.max_doc_count = doc_count;   /* [RPAP - Jan 97: Stem Index Change] */
    te.WE.invf_ptr = invf_ptr;
    te.WE.invf_len = invf_len;
    te.Count = Count;
    te.Word = copy_string (Word);
    if (!te.Word)
      FatalError (1, "Could NOT create memory to add term");

    /* [RPAP - Jan 97: Stem Index Change] */
    te.Stem = copy_string (Word);
    if (!te.Stem)
      FatalError (1, "Could NOT create memory to add term");
    stemmer (2, stemmer_num, te.Stem);

    te.require_match = 0;  /* [RJM - 07/97: Ranked Required Terms] */

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
    {
      if (tl->TE[j].Word)
	Xfree (tl->TE[j].Word);
      /* [RPAP - Jan 97: Stem Index Change] */
      if (tl->TE[j].Stem)
	Xfree (tl->TE[j].Stem);
    }
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
  fprintf (file, "we->max_doc_count = %ld\n", we->max_doc_count);
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
  if (te->Stem != NULL)
    fprintf (file, "te->Stem = %s\n", str255_to_string (te->Stem, NULL));  /* [RPAP - Jan 97: Stem Index Change] */
  fprintf (file, "te->require_match = %i\n", te->require_match);  /* [RJM 07/97: Ranked Required Terms] */
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
