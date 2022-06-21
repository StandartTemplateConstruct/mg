/**************************************************************************
 *
 * bool_optimiser -- optimise boolean parse tree
 * Copyright (C) 1994,95  Tim Shimmin
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
 * $Id: bool_optimiser.c,v 1.4 1995/07/27 04:54:55 tes Exp $
 *
 **************************************************************************/

/*
   $Log: bool_optimiser.c,v $
   * Revision 1.4  1995/07/27  04:54:55  tes
   * Committing optimiser before I add the code
   * to find diff-nodes in any kind of tree.
   *
   * Revision 1.3  1994/10/20  00:02:55  tes
   * Fixed bug with bool_optimiser:
   *   - it couldn't handle "false | false"
   *   - it couldn't handle "true & true"
   * These would collapse into an empty list and cause an error.
   *
   * Revision 1.2  1994/10/18  06:11:02  tes
   * The boolean optimiser seems to be modifying the parse tree
   * like it is supposed to.
   * Paragraph ranking now works without any text files if required to.
   *
   * Revision 1.1  1994/10/12  01:15:28  tes
   * Found bugs in the existing boolean query optimiser.
   * So decided to rewrite it.
   * I accidentally deleted query.bool.y, but I have replaced it
   * with bool_parser.y (which I have forgotten to add here ! ;-(
   *
 */

static char *RCSID = "$Id: bool_optimiser.c,v 1.4 1995/07/27 04:54:55 tes Exp $";

#include "sysfuncs.h"

#include "memlib.h"
#include "bool_tree.h"
#include "term_lists.h"
#include "messages.h"

typedef int (*CompFunc) (const void *, const void *);
typedef void (*TreeFunc) (bool_tree_node *);

/* --- routines --- */
static void PermeateNots (bool_tree_node * tree);
static void DoubleNeg (bool_tree_node * not_tree);
static void AndDeMorgan (bool_tree_node * not_tree);
static void OrDeMorgan (bool_tree_node * not_tree);
static int AndDistribute (bool_tree_node * tree);
static void DNF_Distribute (bool_tree_node * tree);
static void ReplaceWithChildren (bool_tree_node * tree);
static void CollapseTree (bool_tree_node * tree);
static void AndTrueFalse (bool_tree_node * and_tree);
static void OrTrueFalse (bool_tree_node * or_tree);
static void TF_Idempotent (bool_tree_node * tree);
static void AndNotToDiff (bool_tree_node * and_tree, TreeFunc recurs_func);
static void DiffDNF (bool_tree_node * or_node);
static void DiffTree (bool_tree_node * tree);
static void AndSort (bool_tree_node * and_tree, TermList * tl, CompFunc comp);
static void AndOrSort (bool_tree_node * tree, TermList * tl);
static void AndSortDNF (bool_tree_node * or_node, TermList * tl);
static void MarkOrTerms (bool_tree_node * tree, TermList * tl);
static int calc_sum_ft (bool_tree_node * term_tree, TermList * tl);
static int AndOrSort_comp (const void *A, const void *B);
static int AndSort_comp (const void *A, const void *B);

/* =========================================================================
 * Function: OptimiseBoolTree
 * Description: 
 *      For case 2:
 *        Do three major steps:
 *        (i) put into standard form
 *            -> put into DNF (disjunctive normal form - or of ands)
 *            Use DeMorgan's, Double-negative, Distributive rules
 *        (ii) simplify
 *             apply idempotency rules
 *        (iii) ameliorate
 *              convert &! to diff nodes, order terms by frequency,...
 *     Could also do the matching idempotency laws i.e. ...
 *     (A | A), (A | !A), (A & !A), (A & A), (A & (A | B)), (A | (A & B)) 
 *     Job for future.... ;-) 
 * Input: 
 * Output: 
 * ========================================================================= */

void
OptimiseBoolTree (bool_tree_node * parse_tree, TermList * tl, int type)
{
  switch (type)
    {
    case 1:
      /* --- sorts for and of or-terms --- */
      PermeateNots (parse_tree);
      TF_Idempotent (parse_tree);
      CollapseTree (parse_tree);
      DiffTree (parse_tree);
      MarkOrTerms (parse_tree, tl);
      AndOrSort (parse_tree, tl);
      break;
    case 2:
      /* --- put into DNF --- */
      PermeateNots (parse_tree);
      TF_Idempotent (parse_tree);
      DNF_Distribute (parse_tree);
      CollapseTree (parse_tree);
      AndSortDNF (parse_tree, tl);
      DiffDNF (parse_tree);
      break;
    default:
      /* --- do nothing --- */
      break;
    }				/*switch */
}				/*OptimiseBoolTree */

/* =========================================================================
 * Function: DoubleNeg
 * Description: 
 *      !(!(a) = a
 *      Assumes binary tree.
 * Input: 
 * Output: 
 * ========================================================================= */
static void
DoubleNeg (bool_tree_node * not_tree)
{
  bool_tree_node *child = BOOL_CHILD (not_tree);
  bool_tree_node *a = BOOL_CHILD (child);
  bool_tree_node *sibling = BOOL_SIBLING (not_tree);

  /* copy a into not tree */
  bcopy (a, not_tree, sizeof (bool_tree_node));

  BOOL_SIBLING (not_tree) = sibling;

  Xfree (child);
  Xfree (a);

}

/* =========================================================================
 * Function: AndDeMorgan
 * Description: 
 *      DeMorgan's rule for 'not' of an 'and'  i.e. !(a & b) <=> (!a) | (!b)
 *      Assumes Binary Tree
 * Input: 
 *      not of and tree
 * Output: 
 *      or of not trees
 * ========================================================================= */

static void
AndDeMorgan (bool_tree_node * not_tree)
{
  bool_tree_node *child = BOOL_CHILD (not_tree);
  bool_tree_node *a = BOOL_CHILD (child);
  bool_tree_node *b = BOOL_SIBLING (a);
  bool_tree_node *new_not = NULL;

  /* change 'not' to 'or' */
  BOOL_TAG (not_tree) = N_or;

  /* change 'and' to 'not' */
  BOOL_TAG (child) = N_not;

  /* 'a' no longer has a sibling */
  BOOL_SIBLING (a) = NULL;

  /* 'a's old sibling is now a child of a new not */
  new_not = CreateBoolTreeNode (N_not, b, NULL);
  BOOL_SIBLING (child) = new_not;
}

/* =========================================================================
 * Function: OrDeMorgan
 * Description: 
 *      DeMorgan's rule for 'not' of an 'or' i.e. !(a | b) <=> (!a) & (!b)
 *      Assumes Binary Tree
 * Input: 
 *      not of and tree
 * Output: 
 *      or of not trees
 * ========================================================================= */

static void
OrDeMorgan (bool_tree_node * not_tree)
{
  bool_tree_node *child = BOOL_CHILD (not_tree);
  bool_tree_node *a = BOOL_CHILD (child);
  bool_tree_node *b = BOOL_SIBLING (a);
  bool_tree_node *new_not = NULL;

  /* change 'not' to 'and' */
  BOOL_TAG (not_tree) = N_and;

  /* change 'or' to 'not' */
  BOOL_TAG (child) = N_not;

  /* 'a' no longer has a sibling */
  BOOL_SIBLING (a) = NULL;

  /* 'a's old sibling is now a child of a new not */
  new_not = CreateBoolTreeNode (N_not, b, NULL);
  BOOL_SIBLING (child) = new_not;
}

/* =========================================================================
 * Function: PermeateNots
 * Description: 
 *      Use DeMorgan's and Double-negative
 *      Assumes tree in binary form (i.e. No ands/ors collapsed)
 * Input: 
 * Output: 
 * ========================================================================= */

static void
PermeateNots (bool_tree_node * tree)
{
  if (!tree)
    return;

  if (BOOL_TAG (tree) == N_not)
    {
      switch (BOOL_TAG (BOOL_CHILD (tree)))
	{
	case N_not:
	  DoubleNeg (tree);
/** NOTE: I do not go on to do the children **/
	  /* I permeate the nots on the tree here & leave it at that */
	  /* In this case I do not know what type 'tree' will end up */
	  PermeateNots (tree);
	  return;
	case N_and:
	  AndDeMorgan (tree);
	  break;
	case N_or:
	  OrDeMorgan (tree);
	  break;
	default:
	  /* leave alone */
	  break;
	}
    }

  /* Permeate Nots for children */
  if (BOOL_HAS_CHILD (tree))
    {
      PermeateNots (BOOL_CHILD (tree));
      PermeateNots (BOOL_SIBLING (BOOL_CHILD (tree)));
    }

}

/* =========================================================================
 * Function: DNF_Distribute
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */


#define ITER_LIMIT 500		/* if reach limit then probably in trouble */

static void
DNF_Distribute (bool_tree_node * tree)
{
  int i = 0;

  /* multiple passes until no change */
  /* not very smart :-( */
  while (1)
    {
      if (AndDistribute (tree) == 0)
	return;
      if (i == ITER_LIMIT)
	FatalError (1, "DNF infinite loop");
      i++;
    }				/*while */

}

/* =========================================================================
 * Function: AndDistribute
 * Description: 
 *      (a | b) & A <=> (a & A) | (b & A)
 * Input: 
 *      binary tree of "AND" , "OR"s.
 * Output: 
 *      return 1 if changed the tree
 *      return 0 if there was NO change (no distributive rule to apply)
 * ========================================================================= */

static int
AndDistribute (bool_tree_node * tree)
{
  if (!tree)
    return 0;

  if (BOOL_TAG (tree) != N_and)
    {
      /* try the children if got some */
      if (BOOL_TAG (tree) == N_or)
	{
	  bool_tree_node *left_child = BOOL_CHILD (tree);
	  int left = AndDistribute (left_child);
	  int right = AndDistribute (BOOL_SIBLING (left_child));
	  return (left || right);
	}

      /* no children, then no change */
      return 0;
    }

  /* AND case */
  {
    /* we have to check the children to see if can distribute */
    bool_tree_node *left_child = BOOL_CHILD (tree);
    bool_tree_node *right_child = BOOL_SIBLING (left_child);
    int swap = 0;

    /* If "or" on the right side then do a swap */
    /* This is done for code simplicity */
    if (BOOL_TAG (right_child) == N_or)
      {
	/* do a swap */
	bool_tree_node *temp;
	temp = right_child;
	right_child = left_child;
	left_child = temp;

	BOOL_SIBLING (left_child) = right_child;
	BOOL_SIBLING (right_child) = NULL;
	BOOL_CHILD (tree) = left_child;

	swap = 1;
      }

    /* do the actual distribution */
    if (swap || BOOL_TAG (left_child) == N_or)
      {
	bool_tree_node *A = right_child;
	bool_tree_node *A_copy = CopyBoolTree (A);
	bool_tree_node *a = BOOL_CHILD (left_child);
	bool_tree_node *b = BOOL_SIBLING (a);

	/* then can distribute away ... */
	BOOL_TAG (tree) = N_or;
	BOOL_TAG (left_child) = N_and;
	BOOL_SIBLING (left_child) = CreateBoolTreeNode (N_and, b, A);
	BOOL_SIBLING (a) = A_copy;

	AndDistribute (tree);
	return 1;
      }

    /* if "and" of something other than "or" s then try the children */
    {
      int left = AndDistribute (left_child);
      int right = AndDistribute (right_child);
      return (left || right);
    }
  }				/* AND case */

}				/*AndDistribute */

/* =========================================================================
 * Function: CollapseTree
 * Description: 
 *      Collapse "and" and "or"s.
 * Input: 
 * Output: 
 * ========================================================================= */

static void
CollapseTree (bool_tree_node * tree)
{
  if (!tree)
    return;

  if ((BOOL_TAG (tree) == N_and) || (BOOL_TAG (tree) == N_or))
    {
      bool_tree_node *child = BOOL_CHILD (tree);
      while (child)
	{
	  while (BOOL_TAG (child) == BOOL_TAG (tree))
	    {
	      ReplaceWithChildren (child);
	    }
	  CollapseTree (child);
	  child = BOOL_SIBLING (child);
	}
    }

  /* assume "not"s permeated */
  return;
}

/* =========================================================================
 * Function: ReplaceWithChildren
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

static void
ReplaceWithChildren (bool_tree_node * tree)
{
  bool_tree_node *next = BOOL_SIBLING (tree);
  bool_tree_node *child = BOOL_CHILD (tree);
  bool_tree_node *last = LastBoolSibling (child);

  bcopy ((char *) child, (char *) tree, sizeof (bool_tree_node));
  Xfree (child);
  BOOL_SIBLING (last) = next;

}

/* =========================================================================
 * Function: TF_Idempotent
 * Description: 
 *      Applies Idempotency laws which refer to True or False (hence "TF")
 *      uses the true or false idempotent rules
 * Input: 
 *      operates on n-ary trees (not restricted to binary)
 * Output: 
 * ========================================================================= */

static void
TF_Idempotent (bool_tree_node * tree)
{
  bool_tree_node *child = NULL;

  while (tree)
    {
      if (BOOL_HAS_CHILD (tree))
	{
	  child = BOOL_CHILD (tree);
	  TF_Idempotent (child);
	}

      /* --- process node --- */
      switch (BOOL_TAG (tree))
	{
	case N_not:
	  child = BOOL_CHILD (tree);

	  /* if true then falsify */
	  if (BOOL_TAG (child) == N_all)
	    {
	      BOOL_TAG (tree) = N_none;
	      BOOL_CHILD (tree) = NULL;
	      Xfree (child);
	      break;
	    }

	  /* if false then trueify */
	  if (BOOL_TAG (child) == N_none)
	    {
	      BOOL_TAG (tree) = N_all;
	      BOOL_CHILD (tree) = NULL;
	      Xfree (child);
	      break;
	    }

	  break;
	case N_and:
	  AndTrueFalse (tree);
	  break;
	case N_or:
	  OrTrueFalse (tree);
	  break;

	default:
	  /* do nothing */
	  break;
	}			/*switch */

      tree = BOOL_SIBLING (tree);
    }				/*while */
}				/*TF_Idempotent */

/* =========================================================================
 * Function: AndTrueFalse
 * Description: 
 *      If any "false" then falsify whole tree 
 *      If any "true" then delete them
 * Input: 
 * Output: 
 * ========================================================================= */

static void
AndTrueFalse (bool_tree_node * and_tree)
{
  bool_tree_node *child = BOOL_CHILD (and_tree);

  bool_tree_node *prev = NULL;

  while (child)
    {
      if (BOOL_TAG (child) == N_none)
	{
	  FreeBoolTree (&(BOOL_CHILD (and_tree)));
	  BOOL_TAG (and_tree) = N_none;
	  return;
	}

      if (BOOL_TAG (child) == N_all)
	{
	  bool_tree_node *sibling = BOOL_SIBLING (child);

	  /* --- link over child --- */
	  if (!prev)
	    {
	      BOOL_CHILD (and_tree) = sibling;
	    }
	  else
	    {
	      BOOL_SIBLING (prev) = sibling;
	    }

	  Xfree (child);
	  child = sibling;
	}
      else
	{
	  prev = child;
	  child = BOOL_SIBLING (child);
	}
    }				/*while */

  child = BOOL_CHILD (and_tree);

  /* if no children ... */
  /* => all have been true */
  if (!child)
    {
      BOOL_TAG (and_tree) = N_all;
      return;
    }

  /* If only one child ... */
  if (!BOOL_SIBLING (child))
    {
      /* replace the parent with child - move it up */
      bool_tree_node *save_sibling = BOOL_SIBLING (and_tree);
      bcopy ((char *) child, (char *) and_tree, sizeof (bool_tree_node));
      BOOL_SIBLING (and_tree) = save_sibling;
      Xfree (child);
    }

}				/*AndTrueFalse */

/* =========================================================================
 * Function: OrTrueFalse
 * Description: 
 *      If any "true" then trueify whole tree 
 *      If any "false" then delete them 
 *      ***** NOTE: This is a reflection of AndTrueFalse *****
 * Input: 
 * Output: 
 * ========================================================================= */

static void
OrTrueFalse (bool_tree_node * or_tree)
{
  bool_tree_node *child = BOOL_CHILD (or_tree);

  bool_tree_node *prev = NULL;

  while (child)
    {
      if (BOOL_TAG (child) == N_all)
	{
	  FreeBoolTree (&(BOOL_CHILD (or_tree)));
	  BOOL_TAG (or_tree) = N_all;
	  return;
	}

      if (BOOL_TAG (child) == N_none)
	{
	  bool_tree_node *sibling = BOOL_SIBLING (child);
	  if (!prev)
	    {
	      BOOL_CHILD (or_tree) = sibling;
	    }
	  else
	    {
	      BOOL_SIBLING (prev) = sibling;
	    }
	  Xfree (child);
	  child = sibling;

	}
      else
	{
	  prev = child;
	  child = BOOL_SIBLING (child);
	}
    }				/*while */

  child = BOOL_CHILD (or_tree);

  /* if no children ... */
  /* => all have been false */
  if (!child)
    {
      BOOL_TAG (or_tree) = N_none;
      return;
    }

  /* If only one child ... */
  if (!BOOL_SIBLING (child))
    {
      /* replace the parent with child - move it up */
      bool_tree_node *save_sibling = BOOL_SIBLING (or_tree);
      bcopy ((char *) child, (char *) or_tree, sizeof (bool_tree_node));
      BOOL_SIBLING (or_tree) = save_sibling;
      Xfree (child);
    }

}				/*AndTrueFalse */

/* =========================================================================
 * Function: AndNotToDiff
 * Description: 
 *      Convert an "And" tree with possibly some negated terms into
 *      a "And - neg-terms". 
 * Input: 
 *      Expects "And" of only literals - as tree should be in DNF form.
 *      This means that no recursion is necessary.
 * Output: 
 * ========================================================================= */

static void
AndNotToDiff (bool_tree_node * and_tree, TreeFunc recurs_func)
{

  /* -------------------------------------------------- */
  /* --- Divide the children up into positive and negative literals --- */
  /* Preserves ordering of children */
  bool_tree_node *pos = NULL;
  bool_tree_node *neg = NULL;
  bool_tree_node *last_pos = NULL;
  bool_tree_node *last_neg = NULL;

  bool_tree_node *child = BOOL_CHILD (and_tree);
  while (child)
    {
      bool_tree_node *next = BOOL_SIBLING (child);
      if (BOOL_TAG (child) == N_not)
	{
	  /* add to end of neg list */
	  if (!neg)
	    neg = child;
	  else
	    BOOL_SIBLING (last_neg) = child;
	  last_neg = child;
	}
      else
	{
	  /* add to end of pos list */
	  if (!pos)
	    pos = child;
	  else
	    BOOL_SIBLING (last_pos) = child;
	  last_pos = child;
	}
      child = next;
    }				/*while */
  if (last_pos)
    BOOL_SIBLING (last_pos) = NULL;
  if (last_neg)
    BOOL_SIBLING (last_neg) = NULL;
  /* -------------------------------------------------- */

  if (!neg)			/* no negtives => no subtraction */
    {
      /* fix up parent's child link */
      BOOL_CHILD (and_tree) = pos;
      return;
    }

  if (!pos)			/* no positives = all negatives */
    {
      /* Choose one of the negs for the left-hand-side of the subtraction */
      /* In particular, choose the first */
      pos = neg;
      neg = BOOL_SIBLING (neg);
      BOOL_SIBLING (pos) = NULL;
    }

  /* Get rid of all '!' from not nodes */
  /* Make a list out of the children */
  {
    bool_tree_node *n = neg;
    bool_tree_node *next = NULL;

    neg = BOOL_CHILD (neg);
    while (n)
      {
	bool_tree_node *c = BOOL_CHILD (n);
	next = BOOL_SIBLING (n);
	Xfree (n);
	if (next)
	  BOOL_SIBLING (c) = BOOL_CHILD (next);
	n = next;
      }
  }

  /* --- restructure the tree with "and" and "diff" --- */
  BOOL_TAG (and_tree) = N_diff;
  if (!BOOL_SIBLING (pos))	/* i.e. only one pos */
    {
      /* (A & !B & !C & !D & ...) ==> A - (B,C,D,...) */

      BOOL_CHILD (and_tree) = pos;
      BOOL_SIBLING (pos) = neg;
    }
  else
    {
      /* (A & !C & B & !D & ...) ==> (A & B & ....) - (C, D, ...) */

      bool_tree_node *n = CreateBoolNode (N_and);
      BOOL_CHILD (and_tree) = n;
      BOOL_CHILD (n) = pos;
      BOOL_SIBLING (n) = neg;
    }				/*if */

  if (recurs_func)
    {
      recurs_func (BOOL_CHILD (and_tree));
    }

}				/*AndNotToDiff */

/* =========================================================================
 * Function: DiffTree
 * Description: 
 *      Applies "AndNotToNeg" to each conjunct.
 *      Unlike DiffDNF, this routine will search throughout the tree for
 *      "and"-nodes. It will call AndNotToNeg with this as its recursive 
 *      routine.
 * Date: 27/July/95
 * Input: 
 * Output: 
 * ========================================================================= */

static void
DiffTree (bool_tree_node * tree)
{
  bool_tree_node *t = tree;

  while (t)
    {
      if (BOOL_TAG (t) == N_and)
	{
	  AndNotToDiff (t, DiffTree);
	}
      t = BOOL_SIBLING (t);
    }				/*while */

}				/*DiffTree */

/* =========================================================================
 * Function: DiffDNF
 * Description: 
 *      Applies "AndNotToNeg" to each conjunct.
 * Input: 
 *      A DNF tree
 * Output: 
 *      A tree possibly with some diffs in it.
 * ========================================================================= */

static void
DiffDNF (bool_tree_node * DNF_tree)
{

  if (BOOL_TAG (DNF_tree) == N_or)
    {
      bool_tree_node *child = BOOL_CHILD (DNF_tree);

      while (child)
	{
	  if (BOOL_TAG (child) == N_and)
	    {
	      AndNotToDiff (child, NULL);
	    }
	  child = BOOL_SIBLING (child);
	}
    }
  else if (BOOL_TAG (DNF_tree) == N_and)
    {
      AndNotToDiff (DNF_tree, NULL);
    }
}

/* =========================================================================
 * Function: AndSort_comp 
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

/* Module variable, "term_list",  used as parameter for comparison function */
/* The comparison function only allows certain fixed parameters */
static TermList *term_list;

static int
AndSort_comp (const void *A, const void *B)
{
  const bool_tree_node *const *a = A;
  const bool_tree_node *const *b = B;

  if (BOOL_TAG (*a) == N_term && BOOL_TAG (*b) == N_term)
    {
      /* look up in term list */
      int a_doc_count = GetNthWE (term_list, BOOL_TERM (*a))->doc_count;
      int b_doc_count = GetNthWE (term_list, BOOL_TERM (*b))->doc_count;

      return a_doc_count - b_doc_count;
    }

  if (BOOL_TAG (*a) == N_term && BOOL_TAG (*b) != N_term)
    return -1;

  if (BOOL_TAG (*a) != N_term && BOOL_TAG (*b) == N_term)
    return 1;

  return 0;
}				/*AndSort_comp */

/* =========================================================================
 * Function: AndSort
 * Description: 
 *      Sort the list of nodes by increasing doc_count 
 *      Using some Neil Sharman code - pretty straight forward.
 *      Note: not-terms are sent to the end of the list
 * Input: 
 * Output: 
 * ========================================================================= */

static void
AndSort (bool_tree_node * and_tree, TermList * tl, CompFunc comp)
{
  bool_tree_node **list = NULL;
  bool_tree_node *child = NULL;
  int count = 0;
  int i = 0;

  if (BOOL_TAG (and_tree) != N_and)
    return;

  /* --- Convert linked list into an array for sorting --- */
  child = BOOL_CHILD (and_tree);
  count = 0;
  while (child)
    {
      count++;
      child = BOOL_SIBLING (child);
    }

  if (!(list = Xmalloc (sizeof (bool_tree_node *) * count)))
    return;

  child = BOOL_CHILD (and_tree);
  for (i = 0; i < count; i++)
    {
      list[i] = child;
      child = BOOL_SIBLING (child);
    }
  /* ----------------------------------------------------- */

  term_list = tl;
  qsort (list, count, sizeof (bool_tree_node *), comp);

  /* --- Convert array back into linked list --- */
  for (i = 0; i < count - 1; i++)
    BOOL_SIBLING (list[i]) = list[i + 1];
  BOOL_SIBLING (list[count - 1]) = NULL;
  BOOL_CHILD (and_tree) = list[0];
  Xfree (list);
  /* ----------------------------------------------------- */

}				/*AndSort */

/* =========================================================================
 * Function: AndSortDNF
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */


static void
AndSortDNF (bool_tree_node * DNF_tree, TermList * tl)
{

  if (BOOL_TAG (DNF_tree) == N_or)
    {
      bool_tree_node *child = BOOL_CHILD (DNF_tree);

      while (child)
	{
	  if (BOOL_TAG (child) == N_and)
	    {
	      AndSort (child, tl, AndSort_comp);
	    }
	  child = BOOL_SIBLING (child);
	}
    }
  else if (BOOL_TAG (DNF_tree) == N_and)
    {
      AndSort (DNF_tree, tl, AndSort_comp);
    }
}

/* =========================================================================
 * Function: MarkOrTerms
 * Description: 
 *      Mark or-nodes whose children are all terms.
 *      Also store the sum of the ft's of these nodes.
 * Input: 
 * Output: 
 * ========================================================================= */

static void
MarkOrTerms (bool_tree_node * tree, TermList * tl)
{
  if (BOOL_TAG (tree) == N_or)
    {
      int found_non_term = 0;
      bool_tree_node *child = BOOL_CHILD (tree);

      /* --- check to see if got all terms --- */
      while (child)
	{
	  if (BOOL_TAG (child) != N_term)
	    {
	      found_non_term = 1;
	    }
	  if (BOOL_HAS_CHILD (tree))
	    {
	      MarkOrTerms (child, tl);
	    }
	  child = BOOL_SIBLING (child);
	}			/*while */

      if (!found_non_term)
	{
	  /* Must have all terms for the OR node */
	  /* So change its tag and calculate its sum */
	  BOOL_TAG (tree) = N_or_terms;
	  BOOL_SUM_FT (tree) = calc_sum_ft (tree, tl);
	}			/*if */
      /* ------------------------------------- */

    }				/*if OR node */

  else if (BOOL_HAS_CHILD (tree))
    {
      bool_tree_node *child = BOOL_CHILD (tree);
      while (child)
	{
	  MarkOrTerms (child, tl);
	  child = BOOL_SIBLING (child);
	}
    }

}				/*MarkOrTerms */

/* =========================================================================
 * Function: calc_sum_ft
 * Description: 
 *      Sums up the ft's of all the terms 
 * Input: 
 *      Assumes that the tree only contains terms in it 
 * Output: 
 *      the ft sum
 * ========================================================================= */
static int
calc_sum_ft (bool_tree_node * term_tree, TermList * tl)
{
  int sum_ft = 0;
  bool_tree_node *child = BOOL_CHILD (term_tree);
  while (child)
    {
      sum_ft += GetNthWE (tl, BOOL_TERM (child))->doc_count;
      child = BOOL_SIBLING (child);
    }
  return sum_ft;
}

/* =========================================================================
 * Function: AndOrSort
 * Description: 
 *      This is like an AndSort of just terms. 
 *      The difference is that we are now also 
 *      considering sorting with <or of terms> by using the sum of f_t's.
 * Input: 
 * Output: 
 * ========================================================================= */
static void
AndOrSort (bool_tree_node * tree, TermList * tl)
{
  if (BOOL_HAS_CHILD (tree))
    {

      if (BOOL_TAG (tree) == N_and)
	{
	  AndSort (tree, tl, AndOrSort_comp);
	}

      /* Check out any children down the tree */
      /* Probably a waste of time */
      {
	bool_tree_node *child = BOOL_CHILD (tree);
	while (child)
	  {
	    AndOrSort (child, tl);
	    child = BOOL_SIBLING (child);
	  }
      }				/*do the tree */

    }				/*if has children */
}				/*AndOrSort */

/* =========================================================================
 * Function: AndOrSort_comp
 * Description: 
 *      Comparison function used to order "term"s and "or_terms"s.
 *      It's purpose is to be used by a sort routine.
 * Input: 
 * Output: 
 * ========================================================================= */
static int
AndOrSort_comp (const void *A, const void *B)
{
  const bool_tree_node *const *a = A;
  const bool_tree_node *const *b = B;

  if (BOOL_TAG (*a) == N_term && BOOL_TAG (*b) == N_term)
    {
      /* look up in term list */
      int a_doc_count = GetNthWE (term_list, BOOL_TERM (*a))->doc_count;
      int b_doc_count = GetNthWE (term_list, BOOL_TERM (*b))->doc_count;

      return a_doc_count - b_doc_count;
    }

  if (BOOL_TAG (*a) == N_term && BOOL_TAG (*b) == N_or_terms)
    {
      /* look up in term list */
      int a_doc_count = GetNthWE (term_list, BOOL_TERM (*a))->doc_count;
      int b_doc_count = BOOL_SUM_FT (*b);

      return a_doc_count - b_doc_count;
    }

  if (BOOL_TAG (*a) == N_or_terms && BOOL_TAG (*b) == N_term)
    {
      /* look up in term list */
      int a_doc_count = BOOL_SUM_FT (*a);
      int b_doc_count = GetNthWE (term_list, BOOL_TERM (*b))->doc_count;

      return a_doc_count - b_doc_count;
    }

  if (BOOL_TAG (*a) == N_or_terms && BOOL_TAG (*b) == N_or_terms)
    {
      /* look up in term list */
      int a_doc_count = BOOL_SUM_FT (*a);
      int b_doc_count = BOOL_SUM_FT (*b);

      return a_doc_count - b_doc_count;
    }

  if ((BOOL_TAG (*a) == N_term || BOOL_TAG (*a) == N_or_terms) &&
      (BOOL_TAG (*b) != N_term && BOOL_TAG (*b) != N_or_terms))
    return -1;

  if ((BOOL_TAG (*a) != N_term && BOOL_TAG (*a) != N_or_terms) &&
      (BOOL_TAG (*b) == N_term || BOOL_TAG (*b) == N_or_terms))
    return 1;

  return 0;
}				/*AndOrSort_comp */
