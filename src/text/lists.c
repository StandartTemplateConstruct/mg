/**************************************************************************
 *
 * lists.c -- List processing functions for boolean queries
 * Copyright (C) 1994  Neil Sharman, Alistair Moffat and Lachlan Andrew
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
 * $Id: lists.c,v 1.3 1994/10/20 03:56:50 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"

#include "lists.h"
#include "local_strings.h"
#include "locallib.h"

/*
   $Log: lists.c,v $
   * Revision 1.3  1994/10/20  03:56:50  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:37  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: lists.c,v 1.3 1994/10/20 03:56:50 tes Exp $";





/*
 * Make a DocList large enough to hold num entries 
 */
DocList *
MakeDocList (int num)
{
  DocList *d;
  if (!(d = Xmalloc (sizeof (DocList) + sizeof (DocEntry) * (num - 1))))
    return (NULL);
  d->num = 0;
  return (d);
}


/*
 * Change the size of a DocList so that it can hold num entries 
 */
DocList *
ResizeDocList (DocList * d, int num)
{
  if (!(d = Xrealloc (d, sizeof (DocList) + sizeof (DocEntry) * (num - 1))))
    return (NULL);
  return (d);
}


/* IntersectLists () */
/* Returns the intersection of List1 and List2 by deleting from List1 all */
/* entries not in List2.  All of List2 is freed, and the nodes of List1 not */
/* included in the resultant list are freed. */
/* On exit the intersection of the lists is returned, or NULL on empty */
/* intersection. */
/* Assumes that both lists are ordered on increasing Value. */
DocList *
IntersectLists (DocList * List1, DocList * List2)
{
  DocEntry *s1, *d1, *s2;
  if (!List2 || !List1)
    {
      if (List1)
	Xfree (List1);
      if (List2)
	Xfree (List2);
      return (MakeDocList (0));
    }
  d1 = s1 = List1->DE;
  s2 = List2->DE;
  while (s1 - List1->DE < List1->num && s2 - List2->DE < List2->num)
    {
      if (s1->DocNum == s2->DocNum)
	*d1++ = *s1++;
      if (s1 - List1->DE < List1->num)
	while (s2 - List2->DE < List2->num && s2->DocNum < s1->DocNum)
	  s2++;
      if (s2 - List2->DE < List2->num)
	while (s1 - List1->DE < List1->num && s1->DocNum < s2->DocNum)
	  s1++;
    }
  List1->num = d1 - List1->DE;
  Xfree (List2);
  return (List1);
}


/* DiffLists () */
/* Returns the difference of List1 and List2 by deleting from List1 all */
/* entries in List2.  All of List2 is freed, and the nodes of List1 not */
/* included in the resultant list are freed. */
/* On exit the differrence of the lists is returned, or NULL on empty */
/* intersection. */
/* Assumes that both lists are ordered on increasing Value. */
DocList *
DiffLists (DocList * List1, DocList * List2)
{
  DocEntry *s1, *d1, *s2;
  if (!List1)
    {
      if (List2)
	Xfree (List2);
      return (MakeDocList (0));
    }
  if (!List2)
    return (List1);
  d1 = s1 = List1->DE;
  s2 = List2->DE;
  while (s1 - List1->DE < List1->num)
    {
      if (s2 - List2->DE == List2->num)
	*d1++ = *s1++;
      else
	{
	  if (s1->DocNum == s2->DocNum)
	    {
	      s1++;
	      s2++;
	    }
	  else if (s1->DocNum < s2->DocNum)
	    *d1++ = *s1++;
	  else
	    s2++;
	}
    }
  List1->num = d1 - List1->DE;
  Xfree (List2);
  return (List1);
}

/* MergeLists () */
/* Finds the union of two lists and returns the head of the new list formed. */
/* Assumes that both lists are ordered on increasing Value. */
DocList *
MergeLists (DocList * List1, DocList * List2)
{
  DocList *NewList;
  DocEntry *d, *s1, *s2;
  if (!List1)
    {
      if (List2)
	return (List2);
      else
	return (MakeDocList (0));
    }
  if (!List2)
    return (List1);
  if (!(NewList = MakeDocList (List1->num + List2->num)))
    {
      Xfree (List1);
      Xfree (List2);
      return (NULL);
    }

  s1 = List1->DE;
  s2 = List2->DE;
  d = NewList->DE;
  while (s1 - List1->DE < List1->num || s2 - List2->DE < List2->num)
    {
      if (s1 - List1->DE >= List1->num)
	*d++ = *s2++;
      else if (s2 - List2->DE >= List2->num)
	*d++ = *s1++;
      else if (s1->DocNum == s2->DocNum)
	{
	  *d = *s1++;
	  (d++)->Weight += (s2++)->Weight;
	}
      else if (s1->DocNum < s2->DocNum)
	*d++ = *s1++;
      else
	*d++ = *s2++;
    }
  NewList->num = d - NewList->DE;
  Xfree (List1);
  Xfree (List2);
  return (NewList);
}
