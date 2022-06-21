/**************************************************************************
 *
 * heap.c -- Heap routines
 * Copyright (C) 1994  Neil Sharman
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
 * $Id: heap.c,v 1.1 1994/08/22 00:24:43 tes Exp $
 *
 **************************************************************************/

/*
   $Log: heap.c,v $
   * Revision 1.1  1994/08/22  00:24:43  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: heap.c,v 1.1 1994/08/22 00:24:43 tes Exp $";

#include "heap.h"

#define SWAP(s, a, b) 						\
do {								\
     register int __i;						\
     register char *__a = (a), *__b = (b);			\
     for (__i = (s); __i; __i--)				\
       {							\
         register char __t = *__a;				\
	 *__a++ = *__b; *__b++ = __t;				\
       }							\
} while(0)



#define COPY(s, dst, src) 					\
do {								\
     register int __i;						\
     register char *__dst = (dst), *__src = (src);		\
     for (__i = (s); __i; __i--)				\
       *__dst++ = *__src++;					\
} while(0)



static void 
heap_heapify (void *heap, int size, int num, int curr, heap_comp hc)
{
  register int child = curr * 2;
  while (child <= num)
    {
      if (child < num && hc ((char *) heap + child * size,
			     (char *) heap + child * size - size) < 0)
	child++;
      if (hc ((char *) heap + (curr - 1) * size, (char *) heap + (child - 1) * size) > 0)
	{
	  SWAP (size, (char *) heap + (curr - 1) * size,
		(char *) heap + (child - 1) * size);
	  curr = child;
	  child = child * 2;
	}
      else
	break;
    }
}

static void 
heap_pullup (void *heap, int size, int num, heap_comp hc)
{
  register int curr = num;
  register int parent = curr >> 1;
  while (parent &&
	 hc ((char *) heap + (curr - 1) * size, (char *) heap + (parent - 1) * size) < 0)
    {
      SWAP (size, (char *) heap + (curr - 1) * size,
	    (char *) heap + (parent - 1) * size);
      curr = parent;
      parent = curr >> 1;
    }
}

/************************************************************************
 *
 * NOTE: If you choose to change the comparison function the first thing 
 *       you do after changing the function is call heap_build.
 *
 */
void 
heap_build (void *heap, int size, int num, heap_comp hc)
{
  register int i;
  for (i = num / 2; i > 0; i--)
    heap_heapify (heap, size, num, i, hc);
}



/****************************************************
 *
 * NOTE : The heap must be built before heap_sort is called. 
 *        This has the effect of reversing the order of the array.
 *		e.g. if your comparison function is designed to pull the
 *                   biggest thing off the heap first then the result of
 *		     sorting with this function will be to put the bigest
 *                   thing at the end of the array.
 *
 */
void 
heap_sort (void *heap, int size, int num, heap_comp hc)
{
  register int i;
  for (i = num; i > 1; i--)
    {
      SWAP (size, heap, (char *) heap + (i - 1) * size);
      heap_heapify (heap, size, i - 1, 1, hc);
    }
}


void 
heap_deletehead (void *heap, int size, int *num, heap_comp hc)
{
  (*num)--;
  SWAP (size, heap, (char *) heap + *num * size);
  heap_heapify (heap, size, *num, 1, hc);
}


void 
heap_changedhead (void *heap, int size, int num, heap_comp hc)
{
  heap_heapify (heap, size, num, 1, hc);
}


/***********************************************************************
 *
 * This assumes that the item has been added to the end of the
 * array that is the heap. But that num has not been changed. 
 *
 */
void 
heap_additem (void *heap, int size, int *num, heap_comp hc)
{
  (*num)++;
  heap_pullup (heap, size, *num, hc);
}
