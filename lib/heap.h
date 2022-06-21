/**************************************************************************
 *
 * heap.h -- Heap routines
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
 * $Id: heap.h,v 1.2 1994/09/20 04:20:01 tes Exp $
 *
 **************************************************************************/

/* Care should be taken when using these routines as NO error checking is
 * done.
 */



/* The heap comparison routine should return 
 * <0 : If a should be before b
 *  0 : If a and b are equal
 * >0 : If a should be after b
 */
typedef int (*heap_comp) (void *a, void *b);


/************************************************************************
 *
 * NOTE: If you choose to change the comparison function the first thing 
 *       you do after changing the function is call heap_build.
 *
 */
void heap_build (void *heap, int size, int num, heap_comp hc);



/************************************************************************
 *
 * NOTE : The heap must be built before heap_sort is called.
 *        This has the effect of reversing the order of the array.
 *		e.g. if your comparison function is designed to pull the
 *                   biggest thing off the heap first then the result of
 *		     sorting with this function will be to put the bigest
 *                   thing at the end of the array.
 *
 */
void heap_sort (void *heap, int size, int num, heap_comp hc);

/***********************************************************************
 *
 * NOTE: After deleting the head the heap will be one item shorter.
 *       The deleted item will be placed at the end of the heap.
 *
 */
void heap_deletehead (void *heap, int size, int *num, heap_comp hc);


/***********************************************************************
 *
 * If you change the "value" of the root node of the heap then you
 * should call this to re-pritorize the heap.
 *
 */
void heap_changedhead (void *heap, int size, int num, heap_comp hc);


/***********************************************************************
 *
 * This assumes that the item has been added to the end of the
 * array that is the heap. But that num has not been changed. 
 *
 */
void heap_additem (void *heap, int size, int *num, heap_comp hc);
