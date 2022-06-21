/**************************************************************************
 *
 * pbmtools.c -- Routines for dealing with portable bitmap files (PBM)
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
 * $Id: pbmtools.c,v 1.1.1.1 1994/08/11 03:26:13 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "pbmtools.h"
#include "extractor.h"
#include "utils.h"



void 
pbm_putpixel_range (Pixel ** bitmap, int c, int r, int val, int cols, int rows)
{
  if (((c) >= 0) && ((c) < cols) && ((r) >= 0) && ((r) < rows))
    pbm_putpixel (bitmap, c, r, val);
  else
    fprintf (stderr, "putpitxel (%d %d) outside (%d %d)\n", c, r, cols, rows);
}

int 
pbm_getpixel_range (Pixel ** bitmap, int c, int r, int cols, int rows)
{
  if (((c) >= 0) && ((c) < cols) && ((r) >= 0) && ((r) < rows))
    return pbm_getpixel (bitmap, c, r);
  else
    {
      fprintf (stderr, "getpixel (%d %d) outside (%d %d)\n", c, r, cols, rows);
      return 0;
    }
}





/*
   Returns 1 if *fp points to a PBM (magic is P1 or P4) file.
   leaves the fp in it's original position.
   usage: if(pbm_isapbmfile(fp)) process(fp);
 */
int 
pbm_isapbmfile (FILE * fp)
{
  u_long magic;

  magic = getmagicno_short (fp);
  if ((magic == MAGIC_P4) || (magic == MAGIC_P1))
    return 1;
  else
    return 0;
}


/*
   Frees the memory used in the bitmap structure.
   usage: pbm_freearray(&bitmap);
   Sets bitmap to be NULL on return.
 */
void 
pbm_freearray (Pixel *** bitmap, int rows)
{
  int r;
  if (rows == 0)
    rows++;
  for (r = 0; r < rows; r++)
    free ((*bitmap)[r]);
  free (*bitmap);
  (*bitmap) = NULL;
}


/*
   This function allocates the bitmap.
   usage: bitmap=pbm_allocarray(cols,rows);
 */
Pixel **
pbm_allocarray (int cols, int rows)
{
  Pixel **p;
  int i, newc = (cols + PBITS) / PBITS;

  if (cols == 0)
    cols++;
  if (rows == 0)
    rows++;
  p = (Pixel **) malloc (rows * sizeof (Pixel *));
  if (p == NULL)
    error_msg ("pbm_allocarray", "out of memory", "");
  for (i = 0; i < rows; i++)
    {
      p[i] = (Pixel *) calloc (newc, sizeof (Pixel));
      if (p[i] == NULL)
	error_msg ("pbm_allocarray", "out of memory", "");
    }
  return p;
}



Pixel **
pbm_copy (Pixel ** bitmap, int cols, int rows)
{
  Pixel **p;
  int newc = (cols + PBITS) / PBITS;
  int x, y;

  p = pbm_allocarray (cols, rows);
  for (x = 0; x < newc; x++)
    for (y = 0; y < rows; y++)
      p[y][x] = bitmap[y][x];
  return p;
}




/*
   Reads in a pbm file and returns a pointer to the allocated bitmap, 
   in the P4 format.
   Assumes fp was opened and keeps it that way.
   usage: bitmap=pbm_readfile(fp,&cols,&rows);
 */
Pixel **
pbm_readfile (FILE * fp, int *cols, int *rows)
{
  Pixel **p = NULL;
  char s[100];
  unsigned char *buf;
  int r, c, ii = 0;
  int p1 = 0;
  int arg;


  do
    {
      readline (s, fp);
    }
  while (s[0] == '#');

  if (strcmp (s, "P1") == 0)
    p1 = 1;
  else if (strcmp (s, "P4") == 0)
    p1 = 0;
  else
    error_msg ("pbm_readfile", "only pbm files are supported", "");

  do
    {
      readline (s, fp);
    }
  while (s[0] == '#');

  arg = sscanf (s, "%d %d", cols, rows);
  if (arg == 1)
    {
      readline (s, fp);
      arg = sscanf (s, "%d", rows);
      if (arg != 1)
	error_msg ("pbm_readfile", "bizzare pbm format", "");
    }
  p = pbm_allocarray (*cols, *rows);
  buf = (unsigned char *) malloc ((*cols + 2) * sizeof (unsigned char));
  if (!buf)
    error_msg ("pbm_readfile", "out of memory", "");

  if (p1)
    {
      for (r = 0; r < *rows; r++)
	for (c = 0; c < *cols; c++)
	  {
	    fscanf (fp, "%d", &ii);
	    pbm_putpixel (p, c, r, ii);
	  }
    }
  else
    {
      for (r = 0; r < *rows; r++)
	{
	  c = 0;
	  fread (buf, 1, (*cols + 7) / 8, fp);
	  for (c = 0; c < *cols; c++)
	    {
	      if (!(c % 8))
		ii = buf[c / 8];
	      if (ii < 0)
		error_msg ("pbm_readfile", "file reading error", "");
	      pbm_putpixel (p, c, r, (ii & 128) ? 1 : 0);
	      ii <<= 1;
	    }
	}
    }				/* p4 type */
  free (buf);
  return p;
}


/*
   Writes a pbm file given a file pointer and bitmap. Assumes that
   fp is already open, and keeps in that way.
   usage: pbm_writefile(fp,bitmap,cols,rows);
 */
void 
pbm_writefile (FILE * fp, Pixel ** bitmap, int cols, int rows)
{
  int r, c, ii = 0, j;
  unsigned char *buf;

  buf = (unsigned char *) malloc (((cols + 14) / 8) * sizeof (unsigned char));

  fprintf (fp, "%s\n", "P4");
  fprintf (fp, "%d %d\n", cols, rows);

  for (r = 0; r < rows; r++)
    {
      for (c = 0; c < cols; c += 8)
	{
	  ii = 0;
	  for (j = 0; j < 8; j++)
	    if (c + j < cols)
	      {
		if (pbm_getpixel (bitmap, c + j, r))
		  ii |= (128 >> j);
	      }
	  buf[c / 8] = ii;
	}
      fwrite (buf, 1, (cols + 7) / 8, fp);
    }
  free (buf);
/*      putc('\n',fp);   if this line is included programs don't say things like 'short image' &c, but then again,
   in most cases a 'cmp' between the original will fail because of the last byte...sigh, "standards". */
}

#define BUF_SIZE 131072
char buffer[BUF_SIZE];

void 
pbm_writenamedfile (char fn[], Pixel ** bitmap, int cols, int rows)
{
  FILE *tempfp;

  tempfp = fopen (fn, "wb");
  if (tempfp == NULL)
    error_msg ("pbm_writenamedfile", "can't open file:", fn);
  setbuffer (tempfp, buffer, BUF_SIZE);
  pbm_writefile (tempfp, bitmap, cols, rows);
  fclose (tempfp);
}



Pixel **
pbm_readnamedfile (char fn[], int *cols, int *rows)
{
  FILE *tempfp;
  Pixel **temp;

  tempfp = fopen (fn, "rb");
  if (tempfp == NULL)
    return NULL;
  setbuffer (tempfp, buffer, BUF_SIZE);

  temp = pbm_readfile (tempfp, cols, rows);

  fclose (tempfp);
  return temp;
}
