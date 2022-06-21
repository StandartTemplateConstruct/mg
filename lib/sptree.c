/**************************************************************************
 *
 * sptree.c -- Splay tree code
 * Copyright (C) 1994  Gary Eddy, Alistair Moffat and Neil Sharman
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
 * $Id: sptree.c,v 1.1 1994/08/22 00:24:50 tes Exp $
 *
 **************************************************************************/


#include	"sysfuncs.h"
#include	"sptree.h"

/*
   $Log: sptree.c,v $
   * Revision 1.1  1994/08/22  00:24:50  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: sptree.c,v 1.1 1994/08/22 00:24:50 tes Exp $";

#define TRACE(w)

#define POOL_SIZE 1024


typedef struct node_rec node;

struct node_rec
  {
    node *left, *rght;
    node *par;
    char *datsun;
  };

struct pool
  {
    int length, pos;
    struct pool *next;
    node nodes[POOL_SIZE];
  };

static node *SP_splay ();

#define	CNULL	((char *) NULL)
#define	NNULL	((node *) NULL)
#define	SNULL	((Splay_Tree *) NULL)


/*
 *    createset():
 *              allocates memory for and initialises a set.
 *
 *      Accesses outside variables:     none
 *
 *      Return value...:                none
 */

Splay_Tree *
SP_createset (int (*cmp) (void *, void *))
{
  Splay_Tree *S;

  if (!(S = (Splay_Tree *) malloc (sizeof (Splay_Tree))))
    return (SNULL);
  S->cmp = cmp;
  S->root = CNULL;
  S->no_of_items = 0;
  S->mem_in_use = sizeof (Splay_Tree);
  S->pool = NULL;
  S->pool_chunks = 1024;
  return S;
}				/* createset() */



static node *
SP_get_node (Splay_Tree * S)
{
  struct pool *p = S->pool;
  if (!p || p->pos == p->length)
    {
      p = malloc (sizeof (struct pool));
      if (!p)
	return NULL;
      S->mem_in_use += sizeof (struct pool);
      p->length = POOL_SIZE;
      p->pos = 0;
      p->next = S->pool;
      S->pool = p;
    }
  return &(p->nodes[p->pos++]);
}



/*
 *    SP_freeset():
 *              free the memory allocated to a splay tree. Note this Does not
 *              free the data in the tree you must do this yourself.
 *
 *      Accesses outside variables:     none
 *
 *      Return value...:                none
 */

void 
SP_freeset (Splay_Tree * S)
{
  while (S->pool)
    {
      struct pool *p = S->pool;
      S->pool = p->next;
      free (p);
    }
  free (S);
}


/*
 *    insert():
 *              inserts the item given into the set given in the order
 *              specified by the comparison function in the set 
 *              definition.
 *
 *      Accesses outside variables:     none
 *
 *      Return value...:                none
 */


void *
SP_insert (void *item, Splay_Tree * S)
{
  int dir = 0;
  register node *curr, *prev;
  node *tmp;

  prev = NNULL;
  curr = (node *) S->root;
  for (; curr;)
    {
      prev = curr;
      dir = S->cmp (item, curr->datsun);
      if (dir < 0)
	curr = curr->left;
      else
	curr = curr->rght;
    }
  if (!(tmp = SP_get_node (S)))
    return NULL;
  tmp->datsun = item;

  if (prev == NNULL)
    S->root = (char *) tmp;
  else if (dir < 0)
    prev->left = tmp;
  else
    prev->rght = tmp;

  tmp->par = prev;
  tmp->left = NNULL;
  tmp->rght = NNULL;
  S->no_of_items++;
  S->root = (char *) SP_splay (tmp);
  return (tmp->datsun);
}				/* insert() */

/*
 *    get_first():
 *              returns a pointer to the first item in the set. If the set 
 *              is empty a NULL pointer is returned.
 *
 *      Accesses outside variables:     none
 */

void *
SP_get_first (Splay_Tree * S)
{
  node *curr, *prev;

  for (curr = (node *) S->root, prev = NNULL; curr;
       prev = curr, curr = curr->left)
    ;				/* NULLBODY */

  if (prev)
    S->root = (char *) SP_splay (prev);
  return (prev ? prev->datsun : CNULL);
}				/* get_first() */

/*
 *    get_next():
 *              returns the in-order successor of the currently fingered
 *              item of the set. If no item is fingered or if there is 
 *              no in-order successor a NULL pointer is returned.
 *
 *      Accesses outside variables:     none
 */

void *
SP_get_next (Splay_Tree * S)
{
  node *curr, *prev;

  curr = (node *) S->root;
  if (!curr || !curr->rght)
    return (CNULL);

  for (prev = curr, curr = curr->rght; curr;
       prev = curr, curr = curr->left)
    ;				/* NULLBODY */

  if (prev)
    S->root = (char *) SP_splay (prev);
  return (prev ? prev->datsun : CNULL);
}				/* get_next() */

/*
 *    deletemin():
 *              removes the first element in the set and returns a
 *              pointer to it. If the set is empty a NULL pointer 
 *              is returned.
 *
 *      Accesses outside variables:     none
 */

void *
SP_deletemin (Splay_Tree * S)
{
  node *tmp;
  char *dat;

  dat = SP_get_first (S);
  if (dat)
    {
      tmp = (node *) S->root;
      S->root = (char *) tmp->rght;
      if (S->root)
	((node *) S->root)->par = NULL;
    }
  S->no_of_items--;
  return dat;
}				/* deletemin() */


/*
 *    member():
 *              searches the given set for an item which matches (has
 *              the same key) and returns a pointer to a copy of that item.
 *              If no matching item is found a NULL pointer is 
 *              returned.
 *
 *      Accesses outside variables:     none
 */

void *
SP_member (void *dat, Splay_Tree * S)
{
  register node *curr;
  register int dir;
  static int calls = 0;

  TRACE ("in member");
  for (curr = (node *) S->root; curr;)
    {
      TRACE ("call to cmp");
      dir = S->cmp (dat, curr->datsun);
      TRACE ("out of cmp");
      if (dir < 0)
	curr = curr->left;
      else if (dir > 0)
	curr = curr->rght;
      else
	{
	  /* splay occasionaly to speed up execution */
	  if ((calls++ % 4) == 0 && curr)
	    S->root = (char *) SP_splay (curr);
	  TRACE ("out of member");
	  return (curr->datsun);
	}
    }
  TRACE ("out of member");
  return (CNULL);
}				/* member() */

#define	ROTATEL(p,q) 					\
	do{						\
		p->par = q->par;			\
		q->par = p;				\
		q->rght = p->left;			\
		if(q->rght)				\
			q->rght->par = q;		\
		p->left = q;				\
		if(p->par)				\
			if(q == p->par->left)		\
				p->par->left = p;	\
			else				\
				p->par->rght = p;	\
	}while(0)

#define	ROTATER(p,q) 					\
	do{						\
		p->par = q->par;			\
		q->par = p;				\
		q->left = p->rght;			\
		if(q->left)				\
			q->left->par = q;		\
		p->rght = q;				\
		if(p->par)				\
			if(q == p->par->left)		\
				p->par->left = p;	\
			else				\
				p->par->rght = p;	\
	}while(0)


/*
 *    splay():
 *              i
 *
 *      Accesses outside variables:     i
 *
 *      Return value...:                i
 */

static node *
SP_splay (p)
     register node *p;
{
  register node *q, *g;

  q = p->par;
  /* splay the tree about p */
  while (q != NNULL)
    {
      g = q->par;
      if (p == q->left)
	{
	  if (g == NNULL)
	    ROTATER (p, q);
	  else if (q == g->left)
	    {
	      ROTATER (q, g);
	      ROTATER (p, q);
	    }
	  else
	    {
	      ROTATER (p, q);
	      ROTATEL (p, g);
	    }
	}
      else
	{
	  if (g == NNULL)
	    ROTATEL (p, q);
	  else if (q == g->rght)
	    {
	      ROTATEL (q, g);
	      ROTATEL (p, q);
	    }
	  else
	    {
	      ROTATEL (p, q);
	      ROTATER (p, g);
	    }
	}
      q = p->par;
    }
  return (p);
}
