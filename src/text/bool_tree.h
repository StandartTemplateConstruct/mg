/**************************************************************************
 *
 * bool_tree.h -- description
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
 * $Id: bool_tree.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:23  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:17:46  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:42  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:34:31  rjmcnab
   *** empty log message ***

   * Revision 1.2  1995/03/14  05:15:25  tes
   * Updated the boolean query optimiser to do different types of optimisation.
   * A query environment variable "optimise_type" specifies which one is to be
   * used. Type 1 is the new one which is faster than 2.
   *
   * Revision 1.2  1994/10/18  06:11:07  tes
   * The boolean optimiser seems to be modifying the parse tree
   * like it is supposed to.
   * Paragraph ranking now works without any text files if required to.
   *
   * Revision 1.1  1994/10/12  01:15:35  tes
   * Found bugs in the existing boolean query optimiser.
   * So decided to rewrite it.
   * I accidentally deleted query.bool.y, but I have replaced it
   * with bool_parser.y (which I have forgotten to add here ! ;-(
   *
 */

#ifndef BOOL_TREE_H
#define BOOL_TREE_H

#include "sysfuncs.h"
#include "term_lists.h"

typedef enum
  {
    N_term, N_and, N_or, N_diff, N_none, N_all,
    N_not, N_or_terms
  }
N_Tag;

typedef struct bool_tree_node
  {
    N_Tag tag;
    struct bool_tree_node *sibling;
    union
      {
	struct bool_tree_node *child;
	int term_id;
      }
    fields;
    int sum_ft;			/* for or_terms nodes */
  }
bool_tree_node;


#define BOOL_HAS_CHILD(n) (\
			(n)->tag==N_and || \
			(n)->tag==N_or || (n)->tag==N_or_terms || \
			(n)->tag==N_not || (n)->tag==N_diff)
#define BOOL_TAG(n) ((n)->tag)
#define BOOL_SIBLING(n) ((n)->sibling)
#define BOOL_TERM(n) ((n)->fields.term_id)
#define BOOL_CHILD(n) ((n)->fields.child)
#define BOOL_SUM_FT(n) ((n)->sum_ft)

bool_tree_node *CreateBoolNode (N_Tag tag);
bool_tree_node *CopyBoolTree (bool_tree_node * tree);
bool_tree_node *CreateBoolTermNode (TermList ** tl, char *text, int Count, int word_num,
				    u_long count, u_long doc_count, u_long invf_ptr, 
				    u_long invf_len, int stemmer_num);
bool_tree_node *CreateBoolTreeNode (N_Tag tag,
			     bool_tree_node * left, bool_tree_node * right);
void FreeBoolTree (bool_tree_node ** tree);
void PrintBoolTree (bool_tree_node * tree, FILE * file);
bool_tree_node *LastBoolSibling (bool_tree_node * tree);


#endif
