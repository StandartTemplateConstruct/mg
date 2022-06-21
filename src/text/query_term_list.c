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
 * $Id: query_term_list.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:21  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.1  1998/11/17 09:35:36  rjmcnab
   *** empty log message ***

   * Revision 1.1  1994/10/20  03:57:07  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
 */

#include "sysfuncs.h"

#include "memlib.h"
#include "local_strings.h"
#include "query_term_list.h"
#include "term_lists.h"  /* for MAXTERMSTRLEN */
#include "messages.h"


/* =========================================================================
 * Function: MakeQueryTermList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */
QueryTermList *
MakeQueryTermList (int n)
{
  QueryTermList *t;
  int list_size = (n == 0 ? 1 : n);	/* always allocate at least one node */

  t = Xmalloc (sizeof (QueryTermList) + (list_size - 1) * sizeof (QueryTermEntry));
  if (!t)
    FatalError (1, "Unable to allocate query term list");

  t->num = n;
  t->list_size = list_size;

  return t;
}

/* =========================================================================
 * Function: ResizeQueryTermList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */

#define GROWTH_FACTOR 2
#define MIN_SIZE 2

static void
ResizeQueryTermList (QueryTermList ** query_list)
{
  QueryTermList *qtl = *query_list;

  if (qtl->num > qtl->list_size)
    {
      if (qtl->list_size)
	qtl->list_size *= GROWTH_FACTOR;
      else
	qtl->list_size = MIN_SIZE;
    }
  qtl = Xrealloc (qtl, sizeof (QueryTermList) + (qtl->list_size - 1) * sizeof (QueryTermEntry));

  if (!qtl)
    FatalError (1, "Unable to resize query term list");

  *query_list = qtl;
}

/* =========================================================================
 * Function: ConvertQueryTermsToString
 * Description:
 *      Convert term list into null-terminated string
 * Input:
 *      query_term_list = query list
 * Output:
 *      str = term string
 * ========================================================================= */

void
ConvertQueryTermsToString (QueryTermList * query_term_list, char *str)
{
  int i = 0;
  int total_len = 0;

  /* terms_str should be preallocated */
  if (!str)
    return;

  for (i = 0; i < query_term_list->num; i++)
    {
      unsigned char *word = query_term_list->QTE[i].Term;
      int len = word[0];
      total_len += len + 1;	/* +1 for space */
      if (query_term_list->QTE[i].stem_method >= 0)
	total_len += 2;         /* '#' and the stem number */
      if (total_len > MAXTERMSTRLEN)
	break;
      strncpy (str, (char *) word + 1, len);
      str += len;
      if (query_term_list->QTE[i].stem_method >= 0)
	{
	  *str++ = '#';
	  *str++ = '0' + query_term_list->QTE[i].stem_method;
	}
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
ResetQueryTermList (QueryTermList ** qtl)
{
  if (*qtl)
    FreeQueryTermList (qtl);
  *qtl = MakeQueryTermList (0);
}

/* =========================================================================
 * Function: AddQueryTermEntry
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
AddQueryTermEntry (QueryTermList ** query_term_list, QueryTermEntry * qte)
{
  QueryTermList *qtl = *query_term_list;

  qtl->num++;
  ResizeQueryTermList (query_term_list);
  qtl = *query_term_list;

  /* copy the structure contents */
  bcopy ((char *) qte, (char *) &(qtl->QTE[qtl->num - 1]), sizeof (QueryTermEntry));

  return qtl->num - 1;
}



/* =========================================================================
 * Function: AddQueryTerm
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
AddQueryTerm (QueryTermList ** query_term_list, u_char * Term, int Count, int stem_method)
{
  /* Create a new entry in the list for the new word */
  QueryTermEntry qte;

  qte.Count = Count;
  qte.stem_method = stem_method;
  qte.Term = copy_string (Term);
  if (!qte.Term)
    FatalError (1, "Could NOT create memory to add term");

  return AddQueryTermEntry (query_term_list, &qte);
}

/* =========================================================================
 * Function: FreeQueryTermList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */

void
FreeQueryTermList (QueryTermList ** the_qtl)
{
  int j;
  QueryTermList *qtl = *the_qtl;

  for (j = 0; j < qtl->num; j++)
    if (qtl->QTE[j].Term)
      Xfree (qtl->QTE[j].Term);
  Xfree (qtl);

  *the_qtl = NULL;
}
