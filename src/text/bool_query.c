/**************************************************************************
 *
 * bool_query.c -- Boolean query evaluation
 * Copyright (C) 1994  Neil Sharman & Tim Shimmin
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
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "filestats.h"
#include "timing.h"
#include "sptree.h"


#include "mg.h"
#include "text.h"
#include "invf.h"
#include "lists.h"
#include "term_lists.h"
#include "backend.h"
#include "words.h"
#include "stemmer.h"
#include "stem_search.h"
#include "invf_get.h"
#include "text_get.h"
#include "read_line.h"
#include "bool_tree.h"
#include "bool_parser.h"
#include "bool_optimiser.h"
#include "environment.h"

/* =========================================================================
 * Function: ReadInTermInfo
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

static void 
ReadInTermInfo (query_data * qd)
{
  int i;
  TermList *tl = qd->TL;

  for (i = 0; i < tl->num; i++)
    {
      TermEntry *te = &(tl->TE[i]);
      WordEntry *we = &(te->WE);

      we->word_num = FindWord (qd->sd, te->Word,
			       &we->count, &we->doc_count,
			       &we->invf_ptr, &we->invf_len);
      if (we->word_num == -1)
	we->count = we->doc_count = 0;
    }
}

/* =========================================================================
 * Function: FindNoneTerms 
 * Description: 
 *      Set any terms which are not in dictionary to N_none (i.e. false)
 *      Assumes a prior call to ReadInTermInfo
 * Input: 
 * Output: 
 * ========================================================================= */

static void 
FindNoneTerms (query_data * qd, bool_tree_node * tree)
{
  if (BOOL_HAS_CHILD (tree))
    {
      bool_tree_node *child = BOOL_CHILD (tree);
      while (child)
	{
	  FindNoneTerms (qd, child);
	  child = BOOL_SIBLING (child);
	}
    }
  else if (BOOL_TAG (tree) == N_term)
    {
      int doc_count = qd->TL->TE[BOOL_TERM (tree)].WE.doc_count;
      if (!doc_count)
	BOOL_TAG (tree) = N_none;
    }

}				/*FindNoneTerms */

/* =========================================================================
 * Function: ReduceList
 * Description: 
 *      Go thru doc-set and get rid of all docs which are unmarked
 *      The motivation for this routine is in the case of CNF queries.
 * Input: 
 * Output: 
 * ========================================================================= */

void
ReduceList (DocList * cset)
{
  DocEntry *src;
  DocEntry *dest;

  src = dest = cset->DE;
  for (src = dest = cset->DE; src < cset->DE + cset->num; src++)
    {
      if (src->or_included)
	{
	  src->or_included = 0;
	  *dest++ = *src;
	}
    }
  cset->num = dest - cset->DE;

}				/*ReduceList */

/* =========================================================================
 * Function: BooleanGet
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */
static DocList *
BooleanGet (query_data * qd, bool_tree_node * n,
	    BooleanQueryInfo * bqi)
{
  if (n)
    {
      DocList *res = NULL;
      switch (BOOL_TAG (n))
	{
	case N_all:
	  Message ("Unexpected \"all\" node in the parse tree.");
	  res = MakeDocList (0);
	  break;
	case N_not:
	  Message ("Unexpected \"not\" node in the parse tree.");
	  res = MakeDocList (0);
	  break;
	case N_none:
	  {
	    res = MakeDocList (0);
	    break;
	  }
	case N_diff:
	  {
	    res = BooleanGet (qd, BOOL_CHILD (n), bqi);
	    n = BOOL_SIBLING (BOOL_CHILD (n));
	    while (n)
	      {
		bool_tree_node *next = BOOL_SIBLING (n);
		if (BOOL_TAG (n) == N_term)
		  {
		    WordEntry *we = GetNthWE (qd->TL, BOOL_TERM (n));
		    res = GetDocsOp (qd, we, op_diff_terms, res);
		  }
		else
		  res = DiffLists (res, BooleanGet (qd, n, bqi));
		n = next;
	      }
	    break;
	  }
	case N_and:
	  {
	    res = BooleanGet (qd, BOOL_CHILD (n), bqi);		/* initial fill of c-set */
	    n = BOOL_SIBLING (BOOL_CHILD (n));
	    while (n)
	      {
		bool_tree_node *next = BOOL_SIBLING (n);
		switch (BOOL_TAG (n))
		  {
		  case N_term:
		    {
		      WordEntry *we = GetNthWE (qd->TL, BOOL_TERM (n));
		      res = GetDocsOp (qd, we, op_and_terms, res);
		    }
		    break;
		  case N_or_terms:
		    {
		      bool_tree_node *child = BOOL_CHILD (n);
		      /* note: all children are terms - predetermined */
		      while (child)
			{
			  WordEntry *we = GetNthWE (qd->TL, BOOL_TERM (child));
			  res = GetDocsOp (qd, we, op_and_or_terms, res);
			  child = BOOL_SIBLING (child);
			}
		      ReduceList (res);
		    }
		    break;
		  default:
		    res = IntersectLists (res, BooleanGet (qd, n, bqi));
		  }		/*switch */
		n = next;
	      }
	    break;
	  }
	case N_or:
	case N_or_terms:
	  {
	    res = BooleanGet (qd, BOOL_CHILD (n), bqi);
	    n = BOOL_SIBLING (BOOL_CHILD (n));
	    while (n)
	      {
		bool_tree_node *next = BOOL_SIBLING (n);
		res = MergeLists (res, BooleanGet (qd, n, bqi));
		n = next;
	      }
	    break;
	  }
	case N_term:
	  {
	    res = GetDocsOp (qd, GetNthWE (qd->TL, BOOL_TERM (n)), op_term, NULL);
	    break;
	  }
	}
      return (res ? res : MakeDocList (0));
    }
  else
    return MakeDocList (0);
}

/* =========================================================================
 * Function: AdjustParaDocs
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */
static int 
DE_comp (void *a, void *b)
{
  return ((DocEntry *) a)->SeekPos - ((DocEntry *) b)->SeekPos;
}

static void
AdjustParaDocs (query_data * qd, DocList * Docs)
{
  DocEntry *DE = Docs->DE;
  int s, d;
  Splay_Tree *SP;


  /* Make the doc-list contain only references to unique
   * real documents. So can get rid of cases where there
   * are multiple paragraphs for a real document.
   */

  /* Note: [TS: 29/Sep/94] */
  /* reads in seek-pos for docnums so can compare docs */
  /* seek-pos used as real doc identifier */
  /* whereas doc-num entries refer to paragraphs -- I think */
  for (s = 0; s < Docs->num; s++)
    FetchDocStart (qd, DE[s].DocNum, &DE[s].SeekPos, &DE[s].Len);

  Message ("Original Number = %d\n", Docs->num);
  SP = SP_createset (DE_comp);
  d = 0;
  for (s = 0; s < Docs->num; s++)
    if (!SP_member (&DE[s], SP))
      {
	DE[d] = DE[s];
	SP_insert (&DE[d], SP);
	d++;
      }
  SP_freeset (SP);
  Docs->num = d;
  Message ("Final Number = %d\n", Docs->num);
}

/* =========================================================================
 * Function: BooleanQuery 
 * Description: 
 *      The main control function for the boolean query.
 * Input: 
 * Output: 
 *      qd->DL
 * ========================================================================= */

void 
BooleanQuery (query_data * qd, char *Query, BooleanQueryInfo * bqi)
{
  bool_tree_node *tree;
  DocList *Docs;
  int res = 0;

  tree = ParseBool (Query, MAXLINEBUFFERLEN,
		    &(qd->TL), qd->sd->sdh.stem_method, &res);

  /* [TS:Aug/94] no longer tries to continue after parse failure */
  if (res != 0)			/* failed the parse */
    {
      qd->DL = NULL;		/* have empty doc-list */
      return;
    }

  ReadInTermInfo (qd);
  FindNoneTerms (qd, tree);

  OptimiseBoolTree (tree, qd->TL, atoi (GetEnv ("optimise_type")));

  qd->hops_taken = qd->num_of_ptrs = 0;
  qd->num_of_terms = 0;

  Docs = BooleanGet (qd, tree, bqi);

  if (qd->id->ifh.InvfLevel == 3)
    {
      AdjustParaDocs (qd, Docs);
    }

  if (bqi->MaxDocsToRetrieve != -1 && Docs->num > bqi->MaxDocsToRetrieve)
    Docs->num = bqi->MaxDocsToRetrieve;

  FreeQueryDocs (qd);

  qd->DL = Docs;
}
