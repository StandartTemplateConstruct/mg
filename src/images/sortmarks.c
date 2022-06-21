/**************************************************************************
 *
 * sortmarks.c -- Functions which sort the marks into a "readable sequence"
 * Copyright (C) 1994  Stuart Inglis
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
 * $Id: sortmarks.c,v 1.1.1.1 1994/08/11 03:26:13 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "sortmarks.h"

#define GAP 4



static int 
CmpOnY (const void *e1, const void *e2)
{
  return (((marktype *) e1)->ycen * 0 + ((marktype *) e1)->ypos) -
    (((marktype *) e2)->ycen * 0 + ((marktype *) e2)->ypos);
}

static int 
CmpOnX (const void *e1, const void *e2)
{
  return (((marktype *) e1)->xcen * 0 + ((marktype *) e1)->xpos) -
    (((marktype *) e2)->xcen * 0 + ((marktype *) e2)->xpos);
}








marklistptr 
sortmarks (marklistptr listofmarks)
{
  marktype *table;
  int count = 0;
  int i, s, e;


  table = calloc ((unsigned int) marklist_length (listofmarks), sizeof (marklisttype));

  while (listofmarks)
    {
      table[count] = listofmarks->data;
      marklist_removeat (&listofmarks, 0);
      count++;
    }

  /* sort first on the y position of their centroids */
  qsort ((void *) table, (unsigned int) (count), sizeof (*table), CmpOnY);

  for (s = 0; s < count;)
    {
      int c;

      for (e = s + 1, c = 0; (e < count) &&
	   ((table[e].ypos + table[e].ycen * 0) - (table[e - 1].ypos + table[e - 1].ycen * 0) <= GAP); c++, e++);

      if (e < count && c >= 1)
	qsort (&table[s], (unsigned int) (e - s), sizeof (*table), CmpOnX);
      s = e;
    }

  for (i = 0; i < count; i++)
    {
      table[i].symnum = i;
      marklist_add (&listofmarks, table[i]);
    }

  return listofmarks;
}
