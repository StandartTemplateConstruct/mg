/**************************************************************************
 *
 * marklist.c -- Functions managing the list of marks
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
 * $Id: marklist.c,v 1.1.1.1 1994/08/11 03:26:11 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "pbmtools.h"
#include "utils.h"

void 
marktype_dump (marktype d)
{
  fprintf (stderr, "symnum: %d\n", d.symnum);
  fprintf (stderr, "xpos: %d, ypos: %d\n", d.xpos, d.ypos);
  fprintf (stderr, "w: %d, h: %d\n", d.w, d.h);
  fprintf (stderr, "xcen: %d, ycen: %d\n", d.xcen, d.ycen);
  write_library_mark (stderr, d);
}


/* copy, including make a copy of the bitmap */
marktype 
marktype_copy (marktype d)
{
  marktype d2;
  int i, j;

  d2 = d;
  d2.bitmap = pbm_allocarray (d2.w, d2.h);
  for (i = 0; i < d2.w; i++)
    for (j = 0; j < d2.h; j++)
      pbm_putpixel (d2.bitmap, i, j, pbm_getpixel (d.bitmap, i, j));
  return d2;
}


float 
marktype_density (marktype d)
{
  return d.set / (float) (d.w * d.h);
}



void 
marktype_calc_centroid (marktype * b)
{
  int r, c;
  int count = 0;
  int xtot = 0, ytot = 0;

  for (r = 0; r < b->h; r++)
    for (c = 0; c < b->w; c++)
      if (pbm_getpixel (b->bitmap, c, r))
	{
	  count++;
	  xtot += c;
	  ytot += r;
	}
  if (count)
    {
      xtot /= count;
      ytot /= count;
    }
  b->xcen = xtot;
  b->ycen = ytot;
}



static void 
marktype_area (marktype * d)
{
  int x, y;
  int set = 0;

  for (y = 0; y < d->h; y++)
    for (x = 0; x < d->w; x++)
      if (pbm_getpixel (d->bitmap, x, y))
	set++;
  d->set = set;
}


static void 
marktype_h_runs (marktype * d)
{
  int x, y;
  int inrun = 0, num = 0;
  int foundblack = 0;

  for (y = 0; y < d->h; y++)
    {
      inrun = 0;
      foundblack = 0;
      for (x = 0; x < d->w; x++)
	{
	  if ((!foundblack) && (pbm_getpixel (d->bitmap, x, y)))
	    foundblack = 1;
	  if ((foundblack) && (pbm_getpixel (d->bitmap, x, y) == 0))
	    inrun = 1;
	  if ((inrun) && (pbm_getpixel (d->bitmap, x, y)))
	    {
	      num++;
	      inrun = 0;
	      foundblack = 0;
	    }
	}
    }
  d->h_runs = num;
}

static void 
marktype_v_runs (marktype * d)
{
  int x, y;
  int inrun = 0, num = 0;
  int foundblack = 0;


  for (x = 0; x < d->w; x++)
    {
      inrun = 0;
      foundblack = 0;
      for (y = 0; y < d->h; y++)
	{
	  if ((!foundblack) && (pbm_getpixel (d->bitmap, x, y)))
	    foundblack = 1;
	  if ((foundblack) && (pbm_getpixel (d->bitmap, x, y) == 0))
	    inrun = 1;
	  if ((inrun) && (pbm_getpixel (d->bitmap, x, y)))
	    {
	      num++;
	      inrun = 0;
	      foundblack = 0;
	    }
	}
    }
  d->v_runs = num;
}


/* assume we already have the centroid! */
static void 
marktype_prog_cent (marktype * d)
{
  int x, y;
  int xtot = 0, ytot = 0;
  int num = 0;

  xtot = ytot = num = 0;
  for (x = 0; x < d->xcen; x++)
    for (y = 0; y < d->ycen; y++)
      if (pbm_getpixel (d->bitmap, x, y))
	{
	  xtot += x;
	  ytot += y;
	  num++;
	}
  if (num == 0)
    {
      d->xcen1 = -1;
      d->ycen1 = -1;
    }
  else
    {
      d->xcen1 = xtot / num;
      d->ycen1 = ytot / num;
    }


  xtot = ytot = num = 0;
  for (x = d->xcen; x < d->w; x++)
    for (y = 0; y < d->ycen; y++)
      if (pbm_getpixel (d->bitmap, x, y))
	{
	  xtot += x;
	  ytot += y;
	  num++;
	}
  if (num == 0)
    {
      d->xcen2 = -1;
      d->ycen2 = -1;
    }
  else
    {
      d->xcen2 = xtot / num;
      d->ycen2 = ytot / num;
    }



  xtot = ytot = num = 0;
  for (x = 0; x < d->xcen; x++)
    for (y = d->ycen; y < d->h; y++)
      if (pbm_getpixel (d->bitmap, x, y))
	{
	  xtot += x;
	  ytot += y;
	  num++;
	}


  if (num == 0)
    {
      d->xcen3 = -1;
      d->ycen3 = -1;
    }
  else
    {
      d->xcen3 = xtot / num;
      d->ycen3 = ytot / num;
    }

  xtot = ytot = num = 0;
  for (x = d->xcen; x < d->w; x++)
    for (y = d->ycen; y < d->h; y++)
      if (pbm_getpixel (d->bitmap, x, y))
	{
	  xtot += x;
	  ytot += y;
	  num++;
	}
  if (num == 0)
    {
      d->xcen4 = -1;
      d->ycen4 = -1;
    }
  else
    {
      d->xcen4 = xtot / num;
      d->ycen4 = ytot / num;
    }
}



/* call this to get all features */
void 
marktype_calc_features (marktype * d, char str[])
{
  if (strlen (str))
    strcpy (d->name, str);
  marktype_calc_centroid (d);
  marktype_area (d);
  marktype_prog_cent (d);
  marktype_h_runs (d);
  marktype_v_runs (d);
}





void 
marktype_adj_bound (marktype * m)
{
  int x, y, w, h;
  int xpos, ypos;
  int top = 0, left = 0, right = 0, bot = 0;
  marktype temp;

  w = m->w;
  h = m->h;
  xpos = m->xpos;
  ypos = m->ypos;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      if (pbm_getpixel (m->bitmap, x, y))
	{
	  top = y;
	  x = w;
	  y = h;
	}

  for (y = h - 1; y >= 0; y--)
    for (x = 0; x < w; x++)
      if (pbm_getpixel (m->bitmap, x, y))
	{
	  bot = y;
	  x = w;
	  y = -1;
	}

  for (x = 0; x < w; x++)
    for (y = 0; y < h; y++)
      if (pbm_getpixel (m->bitmap, x, y))
	{
	  left = x;
	  x = w;
	  y = h;
	}

  for (x = w - 1; x >= 0; x--)
    for (y = 0; y < h; y++)
      if (pbm_getpixel (m->bitmap, x, y))
	{
	  right = x;
	  x = -1;
	  y = h;
	}

  temp.w = right - left + 1;
  temp.h = bot - top + 1;
  temp.xpos = xpos + left;
  temp.ypos = ypos + top;
  temp.bitmap = pbm_allocarray (temp.w, temp.h);

  for (x = 0; x < temp.w; x++)
    for (y = 0; y < temp.h; y++)
      pbm_putpixel (temp.bitmap, x, y, pbm_getpixel (m->bitmap, x + left, y + top));

  pbm_freearray (&(m->bitmap), h);

  marktype_calc_features (&temp, "?");
  *m = temp;
}






/* returns a newly created mark reprenting the average of the marklist */
void 
marklist_average (marklistptr list, marktype * avgmark)
{
  int w = 0, h = 0;
  int t = INT_MAX, b = 0, l = INT_MAX, r = 0, matched = 0;
  marktype d, temp;
  marklistptr step;
  static int a[100][100];
  int x, y;

  if (list == NULL)
    error_msg ("marklist_average", "Can't average 'nothing'!", "");

  for (step = list; step; step = step->next)
    {
      matched++;
      temp = step->data;
      w = max (w, temp.w);
      h = max (h, temp.h);
      r = max (r, temp.xcen);
      l = min (l, temp.xcen);
      b = max (b, temp.ycen);
      t = min (t, temp.ycen);
    }
  w = w + (r - l);
  h = h + (b - t);
  if ((w >= 100) || (h >= 100))
    matched = 0;

  if (matched > 1)
    {
      int x_off, y_off;
      for (x = 0; x < w; x++)
	for (y = 0; y < h; y++)
	  a[x][y] = 0;

      for (step = list; step; step = step->next)
	{
	  x_off = r - step->data.xcen;
	  y_off = b - step->data.ycen;
	  for (x = 0; x < step->data.w; x++)
	    for (y = 0; y < step->data.h; y++)
	      if (pbm_getpixel (step->data.bitmap, x, y))
		a[x + x_off][y + y_off]++;
	}

      d.w = w;
      d.h = h;
      d.bitmap = pbm_allocarray (w, h);
      for (x = 0; x < w; x++)
	for (y = 0; y < h; y++)
	  if (a[x][y] > max (1, (matched / 2)))		/* > gives better CR than >= */
	    pbm_putpixel (d.bitmap, x, y, 1);
      marktype_adj_bound (&d);
      strcpy (d.name, "?");
      marktype_calc_centroid (&d);
    }
  else
    d = marktype_copy (list->data);

  *avgmark = d;
}






void 
marklist_setnull (marklistptr * list)
{
  *list = NULL;
}


marklistptr 
marklist_add (marklistptr * list, marktype m)
{
  marklistptr p;

  if (*list == NULL)
    {
      *list = (marklistptr) malloc (sizeof (marklisttype));
      if ((*list) == NULL)
	error_msg ("marklist_add", "OUT of Memory in marklist_add", "");
      p = *list;
    }
  else
    {
      p = *list;
      while (p->next != NULL)
	p = p->next;
      p->next = (marklistptr) malloc (sizeof (marklisttype));
      if (p->next == NULL)
	error_msg ("marklist_add", "OUT of Memory in marklist_add", "");
      p = p->next;
    }
  p->data = m;
  p->next = NULL;
  return p;

}



marklistptr 
marklist_addcopy (marklistptr * list, marktype m)
{
  marktype d;

  d = marktype_copy (m);
  return marklist_add (list, d);
}



int 
marklist_length (marklistptr list)
{
  int count = 0;

  while (list)
    {
      count++;
      list = list->next;
    }
  return count;
}





void 
marklist_dump (marklistptr list)
{
  int count = 0;
  marktype d;

  while (list)
    {
      d = list->data;
      marktype_dump (d);

      count++;
      list = list->next;
    }
}




/* pos starts from 0, to up marklist_length-1 */
marklistptr 
marklist_getat (marklistptr list, int pos, marktype * d)
{
  int count = 0;

  while (list)
    {
      if (count == pos)
	{
	  *d = list->data;
	  return list;
	}
      count++;
      list = list->next;
    }
  fprintf (stderr, "marklist_getat(): access off ends of list: pos %d\n", pos);
  return NULL;
}



void 
marklist_free (marklistptr * list)
{
  marklistptr n;

  while ((*list) != NULL)
    {
      n = (*list)->next;
      pbm_freearray (&((*list)->data.bitmap), (*list)->data.h);
      free ((*list));
      (*list) = n;
    }
}



int 
marklist_removeat (marklistptr * list, int pos)
{
  marklistptr c, n, p = NULL;
  int count = 0;

  if ((pos == 0) && (*list))
    {				/* if removing the 1st element */
      *list = (*list)->next;
      return 1;
    }
  else
    {
      c = *list;
      while (c)
	{
	  if (count == pos)
	    {			/* two cases, no previous, >=1 previous */
	      if (p == NULL)	/* no previous */
		{
		  n = c->next;
		  pbm_freearray (&c->data.bitmap, c->data.h);
		  free (c);
		  c = n;
		  return 1;
		}
	      else
		/* >=1 previous */
		{
		  n = c->next;
		  pbm_freearray (&c->data.bitmap, c->data.h);
		  free (c);
		  p->next = n;
		  return 1;
		}
	    }
	  count++;
	  p = c;
	  c = c->next;
	}
    }
  fprintf (stderr, "marklist_removeat(): access off ends of list: pos %d\n", pos);
  return 0;			/* nothing was removed */
}



marklistptr 
marklist_next (marklistptr list)
{
  return list->next;
}


/* only writes out ascii form */
void 
write_library_mark (FILE * fp, marktype d)
{
  int r, c, rows, cols;
  int xtot = d.xcen, ytot = d.ycen;

  rows = d.h;
  cols = d.w;

  fprintf (fp, "Mark: %d\n", d.symnum);
  fprintf (fp, "Char: %s\n", d.name);
  fprintf (fp, "Cols: %d Rows: %d\n", d.w, d.h);
  for (r = 0; r < d.h; r++)
    {
      for (c = 0; c < d.w; c++)
	if ((r == ytot) && (c == xtot))
	  putc (pbm_getpixel (d.bitmap, c, r) ? 'C' : '!', fp);
	else
	  putc (pbm_getpixel (d.bitmap, c, r) ? 'X' : '.', fp);
      putc ('\n', fp);
    }
}



/* only reads in ascii library */
int 
read_library_mark (FILE * fp, marktype * d)
{
  int r, c, rows, cols, numread, ch1 = 'C';
  char str1[100], str2[100];

  numread = fscanf (fp, "%s %d\n", str1, &(d->symnum));
  if ((numread != 2) || (strcmp (str1, "Mark:") != 0))
    return 0;
  fscanf (fp, "%s %s\n", str1, (d->name));
  fscanf (fp, "%s %d %s %d\n", str1, &(d->w), str2, &(d->h));
  cols = d->w;
  rows = d->h;
  d->bitmap = pbm_allocarray (cols, rows);
  for (r = 0; r < rows; r++)
    {
      for (c = 0; c < cols; c++)
	{
	  ch1 = getc (fp);
	  if ((ch1 == 'X') || (ch1 == 'C'))
	    pbm_putpixel (d->bitmap, c, r, 1);
	  else
	    pbm_putpixel (d->bitmap, c, r, 0);
	}
      ch1 = getc (fp);
    }
  marktype_calc_features (d, "?");

  return 1;
}







int 
read_library (char libraryname[], marklistptr * library)
{
  int err, count;
  FILE *lib;
  marktype d;
  marklistptr step;

  lib = fopen (libraryname, "rb");
  if (lib == NULL)
    error_msg ("read_library()", "Trouble opening library file", "");
  *library = NULL;
  count = 0;
  while (!isEOF (lib))
    {
      err = read_library_mark (lib, &d);
      if (!err)
	error_msg ("read_library()", "unknown format of the library file.", "");
      d.symnum = count;
      count++;
      if ((*library) == NULL)
	step = marklist_add (library, d);
      else
	step = marklist_add (&step, d);
    }
  count = marklist_length (*library);
  fclose (lib);

  return count;
}






/**********************************************************************

'elem' routines for coding offsets

the elem datatype contains the 3 params for arithmetic coding,
stored as a no-duplicates order linked list.

************************************************************************/




void 
elem_add (elemlist * list, int num)
{
  elemptr temp, nn;
  int p, f;

  list->tot++;
  if (list->head == NULL)
    {
      list->head = (elemptr) malloc (sizeof (elem));
      if ((list->head) == NULL)
	error_msg ("", "ekk", "");
      temp = list->head;
      temp->next = NULL;
      temp->num = num;
      temp->pos = 0;
      temp->freq = 1;
    }
  else
    {				/* add to list */
      temp = list->head;
      p = 0;
      f = 0;
      while ((num > temp->num) && (temp->next != NULL))
	{
	  p = temp->pos;
	  f = temp->freq;
	  temp = temp->next;
	}

      if (temp->num == num)
	temp->freq++;
      else
	{			/* insert it */
	  nn = (elemptr) malloc (sizeof (elem));
	  if (nn == NULL)
	    error_msg ("", "ekk", "");

	  if (num < temp->num)
	    {
	      nn->num = temp->num;
	      nn->pos = temp->pos;
	      nn->freq = temp->freq;
	      nn->next = temp->next;
	      temp->num = num;
	      temp->pos = p + f;
	      temp->freq = 1;
	      temp->next = nn;
	    }
	  else
	    {
	      nn->next = temp->next;
	      temp->next = nn;
	      nn->pos = temp->pos + temp->freq;
	      nn->num = num;
	      nn->freq = 1;
	      temp = nn;
	    }
	}
    }
  /* update frequencies */
  temp = temp->next;
  if (temp)
    do
      {
	temp->pos++;
	temp = temp->next;
      }
    while (temp);
}


void 
elem_free (elemlist * list)
{
  elemptr n, p;

  p = list->head;
  while (p != NULL)
    {
      n = p->next;
      free (p);
      p = n;
    }
  list->head = NULL;
}

int 
elem_find (elemlist * list, int n, int *p, int *f, int *t)
{
  elemptr temp;

  temp = list->head;
  while (temp)
    {
      if (temp->num == n)
	{
	  *p = temp->pos;
	  *f = temp->freq + temp->pos;
	  *t = list->tot;
	  return 1;
	}
      temp = temp->next;
    }
  return 0;
}


int 
elem_getrange (elemlist * list, int range, int *num)
{
  elemptr temp;

  temp = list->head;
  while (temp)
    {
      if ((range >= temp->pos) && (range < (temp->pos + temp->freq)))
	{
	  *num = temp->num;
	  return 1;
	}
      temp = temp->next;
    }
  return 0;
}
