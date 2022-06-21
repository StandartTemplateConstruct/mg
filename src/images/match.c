/**************************************************************************
 *
 * match.c -- Functions related to matching marks
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
 * $Id: match.c,v 1.1.1.1 1994/08/11 03:26:11 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "pbmtools.h"
#include "utils.h"


/* error mask goes from 0..8, single pixel errors are worth nothing */
int 
WXOR_match (marktype b1, marktype b2)
{
  int r, c, i, j;
  int minr, minc;
  int maxr, maxc;
  int offx, offy;
  int p1, p2;
  int count = 0;
  marktype d;

  offx = b1.xcen - b2.xcen;
  offy = b1.ycen - b2.ycen;

  minc = min (0, offx);
  minr = min (0, offy);

  maxc = max (b1.w, b2.w + offx);
  maxr = max (b1.h, b2.h + offy);

  d.w = maxc - minc + 1;
  d.h = maxr - minr + 1;
  d.bitmap = pbm_allocarray (d.w, d.h);


  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      {
	if ((r >= 0) && (c >= 0) && (r < b1.h) && (c < b1.w))
	  p1 = pbm_getpixel (b1.bitmap, c, r);
	else
	  p1 = 0;		/* 0 = back ground colour */
	if ((r - offy >= 0) && (c - offx >= 0) && (r - offy < b2.h) && (c - offx < b2.w))
	  p2 = pbm_getpixel (b2.bitmap, c - offx, r - offy);
	else
	  p2 = 0;		/* background colour */
	pbm_putpixel (d.bitmap, c - minc, r - minr, p1 != p2);
      }
  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      if (pbm_getpixel (d.bitmap, c - minc, r - minr))
	for (i = -1; i <= 1; i++)
	  for (j = -1; j <= 1; j++)
	    if ((r + j >= 0) && (c + i >= 0) && (r + j < d.h) && (c + i < d.w) && ((i != 0) || (j != 0)) && (pbm_getpixel (d.bitmap, c + i - minc, r + j - minr)))
	      count++;
  pbm_freearray (&d.bitmap, d.h);
  return count;
}



/* define an array for most matches, if it is a massive mark, then WXOR is called */
#define S_SIZE 500

static unsigned char scratch[S_SIZE][S_SIZE], sc2[S_SIZE][S_SIZE];




/* Weighted AND-NOT */

int 
WAN_match (marktype b1, marktype b2)
{
  int r, c, i, j;
  int minr, minc;
  int maxr, maxc;
  int offx, offy;
  int p1, p2;
  int count = 0;

  offx = b1.xcen - b2.xcen;
  offy = b1.ycen - b2.ycen;

  minc = min (0, offx);
  minr = min (0, offy);

  maxc = max (b1.w, b2.w + offx);
  maxr = max (b1.h, b2.h + offy);

  if (((maxc - minc) >= S_SIZE) || ((maxr - minr) >= S_SIZE))
    return WXOR_match (b1, b2);

  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      {
	scratch[r - minr][c - minc] = sc2[r - minr][c - minc] = 0;
	if ((r >= 0) && (c >= 0) && (r < b1.h) && (c < b1.w))
	  p1 = pbm_getpixel (b1.bitmap, c, r);
	else
	  p1 = 0;		/* 0 == background colour */
	if ((r - offy >= 0) && (c - offx >= 0) && (r - offy < b2.h) && (c - offx < b2.w))
	  p2 = pbm_getpixel (b2.bitmap, c - offx, r - offy);
	else
	  p2 = 0;

	if ((p1 == 0) && (p2 == 1))	/* p1 not set, p2 = set */
	  scratch[r - minr][c - minc] = 1;
	if ((p1 == 1) && (p2 == 0))	/* p1 set, p2 not set */
	  sc2[r - minr][c - minc] = 1;
      }

  count = 0;
  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      if (scratch[r - minr][c - minc])
	for (i = -1; i <= 1; i++)
	  for (j = -1; j <= 1; j++)
	    if ((r + j >= minr) && (c + i >= minc) && (r + j < maxr) && (c + i < maxc) && ((i != 0) || (j != 0)) && scratch[r - minr + j][c - minc + i])
	      count++;

  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      if (sc2[r - minr][c - minc])
	for (i = -1; i <= 1; i++)
	  for (j = -1; j <= 1; j++)
	    if ((r + j >= minr) && (c + i >= minc) && (r + j < maxr) && (c + i < maxc) && ((i != 0) || (j != 0)) && sc2[r - minr + j][c - minc + i])
	      count++;

  return count;
}

/* oc 8 must be in a clock wise order, with oc[0] a corner */
struct
{
  int x, y;
}
oc[8] =
{
  {
    -1, -1
  }
  ,
  {
    0, -1
  }
  ,
  {
    1, -1
  }
  ,
  {
    1, 0
  }
  ,
  {
    1, 1
  }
  ,
  {
    0, 1
  }
  ,
  {
    -1, 1
  }
  ,
  {
    -1, 0
  }
};



static void 
xormatchscratch (marktype * b1, marktype * b2, int minr, int maxr, int minc, int maxc, int offx, int offy)
{
  int r, c, rmm, roo, coo;
  int p1, p2;
  Pixel **bm1 = b1->bitmap, **bm2 = b2->bitmap;
  int w1 = b1->w, h1 = b1->h, w2 = b2->w, h2 = b2->h;

  for (r = minr; r < maxr; r++)
    {
      rmm = r - minr;
      roo = r - offy;
      p1 = 0;
      for (c = minc; c < 0; c++)
	{
	  coo = c - offx;
	  scratch[rmm][c - minc] = sc2[rmm][c - minc] = 0;
	  if ((coo >= 0) && (roo >= 0) && (coo < w2) && (roo < h2))
	    p2 = pbm_getpixel (bm2, coo, roo);
	  else
	    p2 = 0;		/* background */
	  scratch[rmm][c - minc] = p1 != p2;
	}
      for (c = 0; c < maxc; c++)
	{
	  coo = c - offx;
	  scratch[rmm][c - minc] = sc2[rmm][c - minc] = 0;
	  if ((r >= 0) && (c < w1) && (r < h1))
	    p1 = pbm_getpixel (bm1, c, r);
	  else
	    p1 = 0;		/* background */
	  if ((coo >= 0) && (roo >= 0) && (coo < w2) && (roo < h2))
	    p2 = pbm_getpixel (bm2, coo, roo);
	  else
	    p2 = 0;		/* background */
	  scratch[rmm][c - minc] = p1 != p2;
	}
    }
}



int 
CSIS_match (marktype * b1, marktype * b2)
{
  int r, c, i;
  int minr, minc;
  int maxr, maxc;
  int offx, offy, fepcount;
  int totscratch = 0;


  offx = b1->xcen - b2->xcen;
  offy = b1->ycen - b2->ycen;

  minc = min (0, offx);
  minr = min (0, offy);

  maxc = max (b1->w, b2->w + offx);
  maxr = max (b1->h, b2->h + offy);

  if (((maxc - minc) >= S_SIZE) || ((maxr - minr) >= S_SIZE))
    return WXOR_match (*b1, *b2);

  /* set the scratch array to 1 if there is an XOR match */

  xormatchscratch (b1, b2, minr, maxr, minc, maxc, offx, offy);
  /* We how have the XOR map. Scratch contains the number of neighbours, 1=1 neighbour set...etc */

  /* CSIS rule 1, if FEP (four error pels) then exit */
  fepcount = 0;
  for (r = minr; r < maxr - 1; r++)
    for (c = minc; c < maxc - 1; c++)
      if ((scratch[r - minr][c - minc]) && (scratch[r - minr + 1][c - minc]) &&
	  (scratch[r - minr][c - minc + 1]) && (scratch[r - minr + 1][c - minc + 1]))
	{
	  fepcount++;
	  if (fepcount == 1)
	    return 10001;
	}

  /* foreach set pixel, update scratch with the number of pels around in five dir's. 
     FPN (five-pel neighbour hood) */
  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      if (scratch[r - minr][c - minc])
	{
	  sc2[r - minr][c - minc] = 1;	/* itself */
	  for (i = 1; i < 8; i += 3)
	    if (((r + oc[i].y) >= minr) && ((c + oc[i].x) >= minc) && ((r + oc[i].y) < maxr) && ((c + oc[i].x) < maxc))
	      if (scratch[r + oc[i].y - minr][c + oc[i].x - minc])
		sc2[r - minr][c - minc]++;
	}
  /* now scratch has the value of the FPN - 1=single error, 5=4 neighbouring errors etc... */

  /* set scratch and sc2 to be the same */
  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      totscratch += scratch[r - minr][c - minc] = sc2[r - minr][c - minc];

  for (r = minr; r < maxr; r++)
    for (c = minc; c < maxc; c++)
      {
	/* CSIS rule 2i: an error pel has 2 or more neighbours---this num is the same as PMS */
	if (scratch[r - minr][c - minc] >= 3)
	  if ((r - minr >= 1) && (c - minc >= 1) && (r - minr < (maxr - 1)) && (c - minc < (maxc - 1)))
	    {
	      int i, numsep = 0;
	      /* find how many neighbours a set pixel has, only check the 4 edges-FPN */
	      for (i = 1; i < 8; i += 2)
		if (scratch[r + oc[i].y - minr][c + oc[i].x - minc])	/* if a scratch pixel is set */
		  {
		    if ((scratch[r + oc[(i + 2) % 8].y - minr][c + oc[(i + 2) % 8].x - minc] == 0) &&
			(scratch[r + oc[(i + 6) % 8].y - minr][c + oc[(i + 6) % 8].x - minc] == 0)
		      )
		      numsep++;
		  }

	      if ((b1->w >= 12) || (b2->w >= 12) || (b1->h >= 12) || (b2->h >= 12))
		{

		  /* PMS rule 2ii: at least two of its neigh. error p's aren't connected */
		  if (numsep >= 2)
		    {
		      int i, j, p, p2, same;

		      /* PMS rule 2(iii), look for regions in the original images of 3x3 the same */
		      if ((c >= 0) && (r >= 0) && (c < b1->w) && (r < b1->h))
			{
			  p = pbm_getpixel (b1->bitmap, c, r);
			  same = 0;
			  for (i = -1; i <= 1; i++)
			    for (j = -1; j <= 1; j++)
			      {
				if ((c + i >= 0) && (r + j >= 0) && (c + i < b1->w) && (r + j < b1->h))
				  p2 = pbm_getpixel (b1->bitmap, c + i, r + j);
				else
				  p2 = 0;
				if (p2 == p)
				  same++;
			      }	/* i,j loop */
			  /* PMS rule 2(iii) */
			  if (same == 9)
			    {
			      return 10002;
			    }
			}	/* (*b1) in range */

		      if ((c - offx >= 0) && (r - offy >= 0) && (c - offx < b2->w) && (r - offy < b2->h))
			{
			  p = pbm_getpixel (b2->bitmap, c - offx, r - offy);
			  same = 0;
			  for (i = -1; i <= 1; i++)
			    for (j = -1; j <= 1; j++)
			      {
				if ((c + i - offx >= 0) && (r + j - offy >= 0) && (c + i - offx < b2->w) && (r + j - offy < b2->h))
				  p2 = pbm_getpixel (b2->bitmap, c + i - offx, r + j - offy);
				else
				  p2 = 0;
				if (p2 == p)
				  same++;
			      }	/* i,j loop */
			  /* PMS rule 2(iii) */
			  if (same == 9)
			    {
			      return 10003;
			    }
			}
		    }
		}
	      else
		{		/* do the CSIS rule */
		  /* CSIS rule 2ii: at least two of its neigh. error p's aren't connected */
		  if (numsep >= 2)
		    {
		      int i, p, p2, same;

		      /* PMS rule 2(iii), look for regions in the original images of 3x3 the same */
		      if ((c >= 0) && (r >= 0) && (c < b1->w) && (r < b1->h))
			{
			  p = pbm_getpixel (b1->bitmap, c, r);
			  same = 1;
			  for (i = 1; i < 8; i += 2)
			    if ((c + oc[i].x >= 0) && (r + oc[i].y >= 0) && (c + oc[i].x < b1->w) && (r + oc[i].y < b1->h))
			      p2 = pbm_getpixel (b1->bitmap, c + oc[i].x, r + oc[i].y);
			    else
			      p2 = 0;
			  if (p2 == p)
			    same++;

			  /* CSIS rule 2(iii) */
			  if (same == 5)
			    {
			      return 10002;
			    }
			}

		      if ((c - offx >= 0) && (r - offy >= 0) && (c - offx < b2->w) && (r - offy < b2->h))
			{
			  p = pbm_getpixel (b2->bitmap, c - offx, r - offy);
			  same = 1;
			  for (i = 1; i < 8; i += 2)
			    if ((c + oc[i].x - offx >= 0) && (r + oc[i].y - offy >= 0) && (c + oc[i].x - offx < b2->w) && (r + oc[i].y - offy < b2->h))
			      p2 = pbm_getpixel (b2->bitmap, c + oc[i].x - offx, r + oc[i].y - offy);
			    else
			      p2 = 0;
			  if (p2 == p)
			    same++;

			  /* CSIS rule 2(iii) */
			  if (same == 5)
			    {
			      return 10003;
			    }
			}
		    }
		}
	    }
      }

  return totscratch;
}
