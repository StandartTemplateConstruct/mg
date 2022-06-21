/**************************************************************************
 *
 * marklist.h -- Functions managing the list of marks
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
 * $Id: marklist.h,v 1.2 1994/09/20 04:41:41 tes Exp $
 *
 **************************************************************************/

#ifndef __MARKLIST_H
#define __MARKLIST_H

#include "sysfuncs.h"
#include "basictypes.h"

typedef struct
  {
    int symnum;
    int xpos, ypos;
    int w, h;
    int xcen, ycen;
    int xoffset, yoffset;
    char name[10];
    Pixel **bitmap;

    int xcen1, ycen1;
    int xcen2, ycen2;
    int xcen3, ycen3;
    int xcen4, ycen4;
    int set;
    int h_runs;
    int v_runs;
  }
marktype;


typedef struct marklisttype *marklistptr;
typedef struct marklisttype
  {
    marktype data;
    marklistptr next;
  }
marklisttype;


void marktype_dump (marktype d);
marktype marktype_copy (marktype d);

float marktype_density (marktype d);
void marktype_calc_centroid (marktype * b);
void marktype_calc_features (marktype * d, char str[]);
void marktype_adj_bound (marktype * m);

void marklist_average (marklistptr list, marktype * avgmark);
void marklist_setnull (marklistptr * list);
marklistptr marklist_add (marklistptr * list, marktype m);
marklistptr marklist_addcopy (marklistptr * list, marktype m);
int marklist_length (marklistptr list);
void marklist_dump (marklistptr list);
marklistptr marklist_getat (marklistptr list, int pos, marktype * d);
void marklist_free (marklistptr * list);
int marklist_removeat (marklistptr * list, int pos);
marklistptr marklist_next (marklistptr list);

void write_library_mark (FILE * fp, marktype d);
int read_library_mark (FILE * fp, marktype * d);
int read_library (char libraryname[], marklistptr * library);






/* 
   The 'elem' structure is an ordered linked list, with fields corresponding
   directly to the arithmetic coding routines.  
 */
typedef struct elem *elemptr;
typedef struct elem
  {
    int num;
    int pos;
    int freq;
    elemptr next;
  }
elem;

typedef struct elemlist
  {
    elemptr head;
    int tot;
  }
elemlist;

void elem_add (elemlist * list, int num);
void elem_free (elemlist * list);
int elem_find (elemlist * list, int n, int *p, int *f, int *t);
int elem_getrange (elemlist * list, int range, int *num);

#endif
