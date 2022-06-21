/**************************************************************************
 *
 * bool_tree.c -- Boolean parse tree ADT
 * Copyright (C) 1994  Tim Shimmin
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
 *       @(#)query.bool.y	1.9 16 Mar 1994
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "bool_tree.h"

static char *RCSID = "$Id: bool_tree.c,v 1.2 1995/03/14 05:15:24 tes Exp $";


/* =========================================================================
 * Function: CreateBoolNode
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */
bool_tree_node *
CreateBoolNode (N_Tag tag)
{
  bool_tree_node *n;
  if (!(n = Xmalloc (sizeof (bool_tree_node))))
    FatalError (1, "Can not create bool_tree_node for boolean query");
  n->tag = tag;
  BOOL_SIBLING (n) = NULL;
  return n;
}

/* =========================================================================
 * Function: 
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */
bool_tree_node *
CreateBoolTermNode (TermList ** tl, char *text)
{
  bool_tree_node *n = NULL;

  /* allocate bool_tree_node */
  n = CreateBoolNode (N_term);

  BOOL_TERM (n) = AddTerm (tl, (u_char *) text);

  return n;
}


/* =========================================================================
 * Function: CreateTreeNode
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

bool_tree_node *
CreateBoolTreeNode (N_Tag tag, bool_tree_node * left, bool_tree_node * right)
{
  bool_tree_node *n = CreateBoolNode (tag);

  BOOL_CHILD (n) = left;
  if (left)
    BOOL_SIBLING (left) = right;
  if (right)
    BOOL_SIBLING (right) = NULL;

  return (n);
}

/* =========================================================================
 * Function: 
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
EqualBoolNodes (bool_tree_node * a, bool_tree_node * b)
{
  if (BOOL_TAG (a) == N_term && BOOL_TAG (a) == N_term)
    return BOOL_TERM (a) == BOOL_TERM (b);
  return 0;
}

/* =========================================================================
 * Function: FreeBoolTree
 * Description: 
 *      Notes: does not fix up any parent or sibling links around given tree
 * Input: 
 * Output: 
 * ========================================================================= */

static void 
FreeTree (bool_tree_node * tree)
{
  bool_tree_node *sibling;

  while (tree)
    {
      if (BOOL_HAS_CHILD (tree))
	FreeTree (BOOL_CHILD (tree));
      sibling = BOOL_SIBLING (tree);
      Xfree (tree);
      tree = sibling;
    }
}

void 
FreeBoolTree (bool_tree_node ** the_tree)
{
  bool_tree_node *tree = *the_tree;

  if (!tree)
    return;

  FreeTree (tree);
  *the_tree = NULL;
/*********************************/
  /* THIS IS POTENTIALLY DANGEROUS */
  /* FIX UP LATER */
  /* Need to correct the links for parent/siblings */
/*********************************/

}

/* =========================================================================
 * Function: PrintBoolTree
 * Description: 
 *      Print out a text version of the tree to the file
 * Input: 
 * Output: 
 * ========================================================================= */

static void
PrintBinaryOp (bool_tree_node * tree, FILE * file, char op)
{
  bool_tree_node *child = NULL;
  assert (BOOL_HAS_CHILD (tree));

  child = BOOL_CHILD (tree);
  while (child)
    {
      PrintBoolTree (child, file);
      child = BOOL_SIBLING (child);
      if (child)
	fprintf (file, " %c ", op);
    }
}

static void
PrintUnaryOp (bool_tree_node * tree, FILE * file, char op)
{
  bool_tree_node *child = NULL;

  fprintf (file, " %c ", op);
  assert (BOOL_HAS_CHILD (tree));
  child = BOOL_CHILD (tree);
  PrintBoolTree (child, file);
}

void
PrintBoolTree (bool_tree_node * tree, FILE * file)
{
  if (!tree)
    return;

  fprintf (file, "(");
  switch (BOOL_TAG (tree))
    {
    case N_term:
      fprintf (file, "term %d", BOOL_TERM (tree));
      break;
    case N_and:
      PrintBinaryOp (tree, file, '&');
      break;
    case N_or:
      PrintBinaryOp (tree, file, '|');
      break;
    case N_or_terms:
      PrintBinaryOp (tree, file, '+');
      break;
    case N_diff:
      PrintBinaryOp (tree, file, '-');
      break;
    case N_not:
      PrintUnaryOp (tree, file, '!');
      break;
    case N_all:
      fprintf (file, " TRUE ");
      break;
    case N_none:
      fprintf (file, " FALSE ");
      break;
    default:
      break;
    }

  fprintf (file, ")");
}


/* =========================================================================
 * Function: CopyBoolTree
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

bool_tree_node *
CopyBoolTree (bool_tree_node * tree)
{
  bool_tree_node *sibling = NULL;
  bool_tree_node *tree_copy = NULL;
  bool_tree_node *prev = NULL;

  if (!tree)
    return NULL;

  sibling = tree;
  prev = NULL;
  while (sibling)
    {
      /* copy sibling */
      bool_tree_node *sibling_copy = (bool_tree_node *) malloc (sizeof (bool_tree_node));
      if (!sibling_copy)
	FatalError (1, "No memory for bool tree copy");
      bcopy ((char *) sibling, (char *) sibling_copy, sizeof (bool_tree_node));

      if (BOOL_HAS_CHILD (sibling))
	{
	  BOOL_CHILD (sibling_copy) = CopyBoolTree (BOOL_CHILD (sibling));
	}

      /* do the link up of copies */
      if (prev)
	BOOL_SIBLING (prev) = sibling_copy;
      else
	tree_copy = sibling_copy;
      prev = sibling_copy;

      sibling = BOOL_SIBLING (sibling);
    }
  return tree_copy;
}


/* =========================================================================
 * Function: LastBoolSibling
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

bool_tree_node *
LastBoolSibling (bool_tree_node * tree)
{

  if (!tree)
    return NULL;

  do
    {
      tree = BOOL_SIBLING (tree);
    }
  while (BOOL_SIBLING (tree));

  return tree;
}
