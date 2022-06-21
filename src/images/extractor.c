/**************************************************************************
 *
 * extractor.c -- Functions related to extracting marks
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
 * $Id: extractor.c,v 1.1.1.1 1994/08/11 03:26:10 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "extractor.h"
#include "pbmtools.h"
#include "marklist.h"
#include "utils.h"


/* 
   'splitjoined' defines whether the extractor modules tries to break
   "seemingly" merged/joined marks.
 */
int splitjoined = 1;

#define MAX_FILL_STACK_SIZE 10000

/* RunRegionFill uses line conherence to implement a better fill
   routine.  The stack has currently got 10000 elements, although the
   biggest I have encountered used a maximum stack size of 1294.  usage:
   RunRegionFill(bitmap,mark,i,j,xpos,ypos); xpos and ypos are the top
   left corners of the mark.  */

int maxst = 0, st = 0;
struct
  {
    int x, y;
  }
a[MAX_FILL_STACK_SIZE];

#define push(i,j) a[st].x=i;a[st].y=j;st++;if(st>MAX_FILL_STACK_SIZE){error_msg ("extractor/flood_fill","woooo! Fill stack overflow!","");}
#define pop(i,j) st--;i=a[st].x;j=a[st].y

void 
RunRegionFill (Pixel ** bitmap, marktype * mark, int i, int j, int xl, int yt, int cols, int rows)
{
  int l, r, t, start_up, start_down;

  push (i, j);
  while (st > 0)
    {
      if (st > maxst)
	maxst = st;

      pop (l, j);
      r = l;

      while (pbm_getpixel (bitmap, l - 1, j))
	l--;
      while (pbm_getpixel (bitmap, r + 1, j))
	r++;
      start_up = start_down = 1;
      for (t = min (r + 1, cols - 1); t >= max (l - 1, 0); t--)
	{
	  if ((t >= l) && (t <= r))
	    {
	      pbm_putpixel (bitmap, t, j, 0);

	      if (mark)
		pbm_putpixel (mark->bitmap, t - xl, j - yt, 1);
	    }

	  if ((j - 1 >= 0) && (pbm_getpixel (bitmap, t, j - 1)))
	    {
	      if (start_up)
		{
		  push (t, j - 1);
		  start_up = 0;
		}
	    }
	  else
	    start_up = 1;
	  if ((j + 1 < rows) && (pbm_getpixel (bitmap, t, j + 1)))
	    {
	      if (start_down)
		{
		  push (t, j + 1);
		  start_down = 0;
		}
	    }
	  else
	    start_down = 1;
	}
    }
}




/*

   ExtractNextMark starts at (tx,ty) and looks for a set (==1) pixel.
   The mark is then boundary traced to give an outline {The process is a
   4-pixel exterior check -- giving 8 way connectivity}, and then
   RunRegionFill is used to remove the mark and set the pixels in the
   mark.

   usage: ExtractNextMark(bitmap,&marklist,&startx,&starty,int cols,int rows);
   Defines:
   gp - gets a pixel, but returns 0 for outside regions.
   boundary_{x,y}comp - returns the {x,y} component of the direction.
   dir 0=right, 1=down, 2=left, 3=up.
 */

#define gp(i,j) ( (((i)<0)||((j)<0)||((i)>=cols)||((j)>=rows))? 0 : pbm_getpixel(bitmap,(i),(j)))

#define boundary_xcomp(x) (x==0?1:(x==2?-1 : 0))
#define boundary_ycomp(x) (x==1?1:(x==3?-1 : 0))

int 
ExtractNextMark (Pixel ** bitmap, marklistptr * marklist, int *tx, int *ty, int cols, int rows)
{
  int i, j, xl, xr, yt, yb;
  register int dir, tempdir;
  marktype d;

  xl = INT_MAX;
  xr = (-1);
  yt = INT_MAX;
  yb = (-1);

  i = *tx;
  j = *ty;
  if ((i < 0) || (j < 0))
    error_msg ("ExtractNextMark()", "Stupid values for x,y in findnextmark()", "");
  while (!pbm_getpixel (bitmap, i, j))
    {
      i++;
      if (i >= cols)
	{
	  i = 0;
	  j++;
	  if (j >= rows)
	    return 0;
	}
    }

  *tx = i;
  *ty = j;
  dir = 0;
  j = j - 1;			/* start above */
  do
    {
      if (i > xr)
	xr = i - 1;
      if (i < xl)
	xl = i + 1;
      if (j < yt)
	yt = j + 1;
      if (j > yb)
	yb = j - 1;
      tempdir = (dir + 1) % 4;
      if (gp (i + boundary_xcomp (tempdir), j + boundary_ycomp (tempdir)) == 0)
	dir = tempdir;
      else
	{
	  tempdir = dir;
	  if (gp (i + boundary_xcomp (tempdir), j + boundary_ycomp (tempdir)) == 0)
	    dir = tempdir;
	  else
	    {
	      tempdir = (dir + 3) % 4;
	      if (gp (i + boundary_xcomp (tempdir), j + boundary_ycomp (tempdir)) == 0)
		dir = tempdir;
	      else
		dir = (dir + 2) % 4;
	    }
	}
      i += boundary_xcomp (dir);
      j += boundary_ycomp (dir);
    }
  while ((i != (*tx)) || (j != (*ty - 1)));

  d.w = (xr - xl + 1);
  d.h = (yb - yt + 1);
  d.xpos = xl;
  d.ypos = yt;
  d.bitmap = pbm_allocarray (d.w, d.h);

  RunRegionFill (bitmap, &d, *tx, *ty, xl, yt, cols, rows);

  marktype_calc_features (&d, "?");
  marklist_add (marklist, d);

  *tx = *tx + 1;
  return 1;
}




static void 
normalise (float *p, int w)
{
  int x;
  float mx;

  mx = p[0];
  for (x = 0; x < w; x++)
    if (p[x] > mx)
      mx = p[x];
  if (mx != 0)
    {
      for (x = 0; x < w; x++)
	p[x] = p[x] / mx;
    }
}



#define MAXPROFILE 1000
float profile[MAXPROFILE], p2[MAXPROFILE];


static int 
possibly_joined (marktype * d)
{
  float aspect;
  int i, n, x, y;
  int leftable, l2;


  aspect = d->w / (float) d->h;

  if ((aspect >= 1.1) || (d->w < MAXPROFILE))
    {
      /* examine it more closely */


      /* set up profile[] with a vertical profile */
      for (x = 0; x < d->w; x++)
	{
	  profile[x] = 0;
	  for (y = 0; y < d->h; y++)
	    if (pbm_getpixel (d->bitmap, x, y))
	      profile[x]++;
	}

      /* average the profile from (up to) 3 either way */
      for (x = 0; x < d->w; x++)
	{
	  n = 0;
	  p2[x] = 0;
	  for (i = x - 3; i <= x + 3; i++)
	    if ((i >= 0) && (i < d->w))
	      {
		n++;
		p2[x] += profile[x];
	      }
	  p2[x] = p2[x] / (float) n;
	}
      for (x = 0; x < d->w; x++)
	profile[x] = p2[x];

      /* normalise profile[] */
      normalise (profile, d->w);

      /* find a little local minimum, with profile <0.25, make p2[] either 0 or 1 */
      for (x = 0; x < d->w; x++)
	p2[x] = 0;
      for (x = 3; x < d->w - 3; x++)
	{
	  if (((profile[x - 2] - profile[x]) > 0.00) &&
	      ((profile[x + 2] - profile[x]) > 0.00) &&
	      (profile[x] < .25))
	    p2[x] = 1;
	}

      /* travel rightwards, checking aspect ratio, looking at p2 now */
      leftable = l2 = 0;
      for (x = 0; x < d->w; x++)
	if (p2[x] == 1)
	  {
	    if ((fabs ((abs (x - leftable) / (float) d->h) - 0.7) > 0.3) &&
		(fabs ((abs (x - l2) / (float) d->h) - 0.7) > 0.3))
	      p2[x] = 0;
	    if (p2[x] == 1)
	      {
		leftable = l2;
		l2 = x;
	      }
	  }

      /* travel inwards, checking widths of chars, looking at p2 */
      /* if a char has an aspect ratio of <= ? kill it */
      l2 = 0;
      leftable = d->w - 1;
      x = 0;
      y = d->w - 1;
      while (x <= y)
	{
	  if (p2[x])
	    {
	      if (abs (x - l2) < max (d->h * 0.6, 5))
		p2[x] = 0;
	      if (p2[x])
		l2 = x;
	    }
	  if (p2[y])
	    {
	      if (abs (y - leftable) < max (d->h * 0.6, 5))
		p2[y] = 0;
	      if (p2[y])
		leftable = y;
	    }
	  x++;
	  y--;
	}


      for (x = 0; x < d->w; x++)
	if (p2[x])
	  return 1;
    }
  return 0;
}


static void 
split_mark (marklistptr * list, marktype * joinedmark)
{
  marktype new, old;
  int x, found = 0, left = 0;
  int i, j;
  int oldw, oldh;
  int overwrite = 1;

  old = marktype_copy (*joinedmark);
  oldw = joinedmark->w;
  oldh = joinedmark->h;
  for (x = 0; x < old.w; x++)
    if (p2[x])
      {
	found = x;
	break;
      }
  do
    {
      /* split at (found) position */
      new.w = found + 1 - left;
      new.h = old.h;
      new.xpos = old.xpos + left;
      new.ypos = old.ypos;
      new.bitmap = pbm_allocarray (new.w, new.h);

      for (i = 0; i < new.w; i++)
	for (j = 0; j < new.h; j++)
	  pbm_putpixel (new.bitmap, i, j, pbm_getpixel (old.bitmap, left + i, j));

      marktype_adj_bound (&new);

      if (overwrite)
	{
	  overwrite = 0;
	  pbm_freearray (&(joinedmark->bitmap), oldh);
	  *joinedmark = new;
	}
      else
	marklist_add (list, new);


      p2[found] = 0;
      left = found;
      found = 0;
      for (x = left; x < old.w; x++)
	if (p2[x])
	  {
	    found = x;
	    break;
	  }
      if (left == old.w - 1)
	break;			/* if we've got to the right */
      if (found == 0)
	found = old.w - 1;
    }
  while (found);
  pbm_freearray (&(old.bitmap), oldh);
}





void 
ExtractAllMarks (Pixel ** bitmap, marklistptr * list, int cols, int rows)
{
  int startx, starty, count;
  marklistptr step;

  startx = starty = 0;
  while (ExtractNextMark (bitmap, list, &startx, &starty, cols, rows))
    ;

  if (splitjoined)
    for (step = (*list), count = 0; step; step = step->next, count++)
      if (possibly_joined (&(step->data)))
	split_mark (list, &(step->data));

  for (step = (*list), count = 0; step; step = step->next, count++)
    step->data.symnum = count;
}
