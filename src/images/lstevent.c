/**************************************************************************
 *
 * lstevent.c -- Routines to implement "events" using MTF lists.
 * Copyright (C) 1987  Alistair Moffat
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
 * $Id: lstevent.c,v 1.2 1994/09/20 04:41:40 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "arithcode.h"
#include "model.h"

/*
   $Log: lstevent.c,v $
   * Revision 1.2  1994/09/20  04:41:40  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: lstevent.c,v 1.2 1994/09/20 04:41:40 tes Exp $";

void
write_method ()
{
  fprintf (stderr, "C");
}


/*===========================================*/

/*
 * [Rodney Brown - Sep/95]
 * On IBM RS/6000 AIX version 3.2 the compiler defines NULL as (void *)0
 * when in ANSI mode. Hence the (event) and (point) casts
 */


#define GETNEWNODE(e,node)		\
do 					\
{	node = p32(numnodes) ;		\
	node->eventnum = (event)e ;	\
	node->count = increment ;	\
	node->foll.totalcnt = 0 ;	\
	node->foll.notfound = 0 ;	\
	node->foll.list = (point)NULL ;	\
	numnodes += 1 ;			\
	nfreenodes -= 1 ;		\
}					\
while (false)

/*===========================================*/

#define INSERTNODE(U,node)		\
do					\
{	node->next = U->list ;		\
	U->list = p16(node) ;		\
	U->totalcnt += increment;	\
}					\
while (false)

/*===========================================*/

int
halvelist (p)
     register eventnode *p;
{
  register int sum = 0;
  while (p != ENULL)
    {
      sum += p->count = (p->count + 1) / 2;
      p = p32 (p->next);
    }
  return (sum);
}

/*===========================================*/

void
halveallcounts (U)
     register eventset *U;
{
  U->notfound = (U->notfound + 1) / 2;
  U->totalcnt = U->notfound;
  U->totalcnt += halvelist (p32 (U->list));
}

/*===========================================*/

eventnode *
encode_event_noex (U, e, success)
     register eventset *U;
     event e;
     boolean *success;
{
  register eventnode *pnode, *node;
  register int lbnd, totl;

  if (U->list == (point)NULL)
    {
      *success = false;
      U->notfound += increment;
      U->totalcnt += increment;
      GETNEWNODE (e, node);
      INSERTNODE (U, node);
      return (node);
    }

  lbnd = 0;
  pnode = ENULL;
  node = p32 (U->list);
  while (node != ENULL)
    {
      if (e == node->eventnum)
	break;
      lbnd += node->count;
      pnode = node;
      node = p32 (node->next);
    }

  totl = U->totalcnt;

  if (node == ENULL)
    {				/* event not found in list */
      if ((totl > U->notfound))
	arithmetic_encode (totl - U->notfound, totl, totl);
      U->notfound += increment;
      U->totalcnt += increment;
      *success = false;
      GETNEWNODE (e, node);
      INSERTNODE (U, node);
    }
  else
    {				/* found in the list, so go ahead and code */
      arithmetic_encode (lbnd, lbnd + node->count, totl);
      node->count += increment;
      U->totalcnt += increment;
      *success = true;
      /* and move to front */
      if (pnode != ENULL)
	{
	  pnode->next = node->next;
	  node->next = U->list;
	  U->list = p16 (node);
	}
    }
  if (U->totalcnt > maxtotalcnt)
    halveallcounts (U);
  return (node);
}

/*===========================================*/

eventnode *
decode_event_noex (U, success, e)
     register eventset *U;
     boolean *success;
     event *e;
{
  register eventnode *node, *pnode;
  register int tget, lbnd, totl;

  if (U->list == (point)NULL)
    {
      *success = false;
      U->totalcnt += increment;
      U->notfound += increment;
      GETNEWNODE (NULL, node);
      INSERTNODE (U, node);
      return (node);
    }
  totl = U->totalcnt;
  tget = arithmetic_decode_target (totl);

  if (tget >= totl - U->notfound)
    {
      if (totl > U->notfound)
	arithmetic_decode (totl - U->notfound, totl, totl);
      U->totalcnt += increment;
      U->notfound += increment;
      *success = false;
      GETNEWNODE (NULL, node);
      INSERTNODE (U, node);
    }
  else
    {
      pnode = ENULL;
      node = p32 (U->list);
      lbnd = 0;
      for (;;)
	{
	  if (tget < (lbnd += node->count))
	    break;
	  pnode = node;
	  node = p32 (node->next);
	}

      arithmetic_decode (lbnd - node->count, lbnd, totl);
      node->count += increment;
      U->totalcnt += increment;
      *e = node->eventnum;
      *success = true;
      /* and move to front */
      if (pnode != ENULL)
	{
	  pnode->next = node->next;
	  node->next = U->list;
	  U->list = p16 (node);
	}
    }
  if (U->totalcnt > maxtotalcnt)
    halveallcounts (U);
  return (node);
}

/*===========================================*/

eventnode *
encode_event (U, e, success)
     register eventset *U;
     event e;
     boolean *success;
{
  register eventnode *pnode, *node, *p;
  register int lbnd, totl;

  if (U->list == (point)NULL)
    {
      *success = false;
      U->notfound += increment;
      U->totalcnt += increment;
      GETNEWNODE (e, node);
      INSERTNODE (U, node);
      return (node);
    }

  lbnd = 0;
  pnode = ENULL;
  node = p32 (U->list);
  while (node != ENULL)
    {
      if (!excluded[node->eventnum])
	{
	  if (e == node->eventnum)
	    break;
	  lbnd += node->count;
	}
      pnode = node;
      node = p32 (node->next);
    }

  if (node == ENULL)
    {				/* event not found in list, and whole list has been scanned */
      totl = lbnd + U->notfound;
      if ((totl > U->notfound))
	arithmetic_encode (totl - U->notfound, totl, totl);
      U->notfound += increment;
      U->totalcnt += increment;
      *success = false;
      GETNEWNODE (e, node);
      INSERTNODE (U, node);
    }
  else
    {				/* found in the list, so go ahead and code */
      /* first, total up rest of list */
      totl = lbnd + node->count;
      p = p32 (node->next);
      while (p != ENULL)
	{
	  if (!excluded[p->eventnum])
	    totl += p->count;
	  p = p32 (p->next);
	}
      totl += U->notfound;

      arithmetic_encode (lbnd, lbnd + node->count, totl);
      node->count += increment;
      U->totalcnt += increment;
      *success = true;
      /* and move to front */
      if (pnode != ENULL)
	{
	  pnode->next = node->next;
	  node->next = U->list;
	  U->list = p16 (node);
	}
    }
  if (U->totalcnt > maxtotalcnt)
    halveallcounts (U);
  return (node);
}

/*===========================================*/

eventnode *
decode_event (U, success, e)
     register eventset *U;
     boolean *success;
     event *e;
{
  register eventnode *node, *pnode;
  register int tget, lbnd, totl;

  if (U->list == (point)NULL)
    {
      *success = false;
      U->totalcnt += increment;
      U->notfound += increment;
      GETNEWNODE (NULL, node);
      INSERTNODE (U, node);
      return (node);
    }
  totl = 0;
  node = p32 (U->list);
  while (node != ENULL)
    {
      if (!excluded[node->eventnum])
	totl += node->count;
      node = p32 (node->next);
    }
  totl += U->notfound;
  tget = arithmetic_decode_target (totl);

  if (tget >= totl - U->notfound)
    {
      if (totl > U->notfound)
	arithmetic_decode (totl - U->notfound, totl, totl);
      U->totalcnt += increment;
      U->notfound += increment;
      *success = false;
      GETNEWNODE (NULL, node);
      INSERTNODE (U, node);
    }
  else
    {
      pnode = ENULL;
      node = p32 (U->list);
      lbnd = 0;
      for (;;)
	{
	  if (!excluded[node->eventnum])
	    {
	      lbnd += node->count;
	      if (tget < lbnd)
		break;
	    }
	  pnode = node;
	  node = p32 (node->next);
	}

      arithmetic_decode (lbnd - node->count, lbnd, totl);
      node->count += increment;
      U->totalcnt += increment;
      *e = node->eventnum;
      *success = true;
      /* and move to front */
      if (pnode != ENULL)
	{
	  pnode->next = node->next;
	  node->next = U->list;
	  U->list = p16 (node);
	}
    }
  if (U->totalcnt > maxtotalcnt)
    halveallcounts (U);
  return (node);
}

/*===========================================*/

eventnode *
newnode (e)
     event e;
{
  register eventnode *node;
  GETNEWNODE (e, node);
  node->next = (point)NULL;
  node->prev = (point)NULL;
  return (node);
}

/*===========================================*/

/* not required in this version */

#if 0
eventnode *
addevent (U, e)
     register eventset *U;
     event e;
{
  register eventnode *node;
  /* get an event node */
  GETNEWNODE (e, node);
  INSERTNODE (U, node);
  if (U->totalcnt > maxtotalcnt)
    halveallcounts (U);
  return (node);
}
#endif
