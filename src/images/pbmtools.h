/**************************************************************************
 *
 * pbmtools.h -- Routines for dealing with portable bitmap files (PBM)
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
 * $Id: pbmtools.h,v 1.2 1994/09/20 04:42:01 tes Exp $
 *
 **************************************************************************/

#ifndef __PBMTOOLS_H
#define __PBMTOOLS_H

#include "sysfuncs.h"
#include "basictypes.h"

#include "utils.h"
#include "marklist.h"

#define PBITS 32
#define PSH 5


/* The next two are ordered pixels col0=left, cols31=right */

/*
   #define pbm_getpixel(bitmap,c,r)  \
   ((bitmap[r][(c)>>PSH] >> ((PBITS-1)-((c)&(PBITS-1))))&1)
   #define pbm_putpixel(bitmap,c,r,val) \
   if((val))  \
   bitmap[r][(c)>>PSH] |=  (((unsigned)1<<(PBITS-1)) >> ((c)&(PBITS-1))); \
   else \
   bitmap[r][(c)>>PSH] &= ~(((unsigned)1<<(PBITS-1)) >> ((c)&(PBITS-1))) 
 */




/* The next two defines are ordered pixel cols0=right, cols31=left */

#define pbm_getpixel(bitmap,c,r)                            \
         ((bitmap[r][(c)>>PSH] >> ((c)&(PBITS-1)))&1)
#define pbm_putpixel(bitmap,c,r,val)                        \
         if((val))                                          \
              bitmap[r][(c)>>PSH] |= (1<<((c)&(PBITS-1)));  \
         else                                               \
              bitmap[r][(c)>>PSH] &= ~(1<<((c)&(PBITS-1)))


#define pbm_putpixel_trunc(bitmap,c,r,val,cols,rows) \
       if(((c)>=0)&&((c)<cols)&&((r)>=0)&&((r)<rows)) \
             pbm_putpixel(bitmap,c,r,val)

#define pbm_getpixel_trunc(bitmap,c,r,cols,rows)       \
       ((((c)>=0)&&((c)<cols)&&((r)>=0)&&((r)<rows)) ?  \
             pbm_getpixel(bitmap,c,r) : 0)




int pbm_isapbmfile (FILE * fp);
void pbm_freearray (Pixel *** bitmap, int rows);
Pixel **pbm_copy (Pixel ** bitmap, int cols, int rows);
Pixel **pbm_allocarray (int cols, int rows);
Pixel **pbm_readfile (FILE * fp, int *cols, int *rows);
Pixel **pbm_readnamedfile (char fn[], int *cols, int *rows);
void pbm_writefile (FILE * fp, Pixel ** bitmap, int cols, int rows);
void pbm_writenamedfile (char fn[], Pixel ** bitmap, int cols, int rows);

void pbm_putpixel_range (Pixel ** bitmap, int c, int r, int val, int cols, int rows);
int pbm_getpixel_range (Pixel ** bitmap, int c, int r, int cols, int rows);

#endif
