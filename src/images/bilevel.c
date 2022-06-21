/**************************************************************************
 *
 * bilevel.c -- Compress a bilevel bitmap
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
 * $Id: bilevel.c,v 1.1.1.1 1994/08/11 03:26:09 tes Exp $
 *
 **************************************************************************
 *
 *  
 *  
 *  This is the code used to compress a bitmap.  The template is given in
 *  the following format. 
 *   '.'  = pixel not used
 *   'p'  = pixel used in 1st order context (the superset)
 *   '2'  = pixel used in 2nd order context (the subset)
 *   ';'  = marker used to denote the end of a row in the template
 *   '*'  = the current context pixel.
 *  for instance, the common 22/10 template would be shown as:
 *     ".ppppp.;"
 *     "pp222pp;"
 *     "p22222p;"
 *     "p22*...;"
 * 
 **************************************************************************/


#include "sysfuncs.h"

#include "bilevel.h"
#include "extractor.h"
#include "pbmtools.h"
#include "marklist.h"
#include "arithcode.h"
#include "utils.h"



#define INITVALUE 2
#define INCREMENT 4

#define MAX_T_W 32

/* note memory usage is 2*TEMPLATESIZE*sizeof(prob), which is currently 4.9 Mb */
#define TEMPLATESIZE 612317


#define HASH(x) ((x)%TEMPLATESIZE)


typedef struct
  {
    unsigned short s, n;
  }
prob;				/* set and notset */
typedef struct
  {
    short int x, y;
  }
coordinate;

coordinate MM[MAX_T_W][MAX_T_W];	/* the actual (x,y) positions */
unsigned int MMcount[MAX_T_W];	/* number of bits in this row of the template */
unsigned int MMmask[MAX_T_W];	/* the "current" mask value */
unsigned int MMbits[MAX_T_W];	/* the bitmask to mask out higher bits */
unsigned int MMrows;		/* total number of rows */
unsigned int MM2level[MAX_T_W];	/* bitmask to calculate the 2nd level mask */
int twolevel, curr_row, surr;

prob *P = NULL;			/* probability array, up to N bits */
prob *P2 = NULL;

void 
bl_clearmodel ()
{
  int i;

  P = (prob *) malloc (TEMPLATESIZE * sizeof (prob));
  if (!P)
    error_msg ("bl_clearmodel()", "not enough memory.", "");

  P2 = (prob *) malloc (TEMPLATESIZE * sizeof (prob));
  if (!P2)
    error_msg ("bl_clearmodel()", "not enough memory.", "");

  for (i = 0; i < TEMPLATESIZE; i++)
    {
      P[i].s = P[i].n = INITVALUE;
      P2[i].s = P2[i].n = INITVALUE;
    }
}


void 
bl_freemodel ()
{
  free (P);
  free (P2);
}




#define update_counts(p, pixel)                        \
    pixel ? (p->s+=INCREMENT) : (p->n+=INCREMENT);     \
                                                       \
    if(p->s+p->n >= (unsigned)(maxfrequency)) {        \
       p->s=(p->s+1)/(unsigned)2;                      \
       p->n=(p->n+1)/(unsigned)2;                      \
    }




/* mask is the bigger one, mask2l is smaller one */
static void 
bl_encode_in_context (unsigned long mask, unsigned long mask2l, unsigned long pixel)
{
  register int twol, t;
  prob *p = &P[mask];
  prob *p2 = &P2[mask2l];

  twol = twolevel && (p->s + p->n < (unsigned) (2 * INITVALUE + 5 * INCREMENT));

  t = twol ? (p2->s + p2->n) : (p->s + p->n);

  if (twol == 0)
    {
      if (pixel)
	arithmetic_encode (0, p->s, t);
      else
	arithmetic_encode (p->s, t, t);
      update_counts (p, pixel);
    }
  else
    {
      if (pixel)
	arithmetic_encode (0, p2->s, t);
      else
	arithmetic_encode (p2->s, t, t);
      update_counts (p2, pixel);
      update_counts (p, pixel);
    }
}


static void 
bl_decode_in_context (unsigned long mask, unsigned long mask2l, unsigned long *val)
{
  register int twol, pixel, t;
  prob *p = &P[mask];
  prob *p2 = &P2[mask2l];


  if (twolevel && (p->s + p->n < (unsigned) (2 * INITVALUE + 5 * INCREMENT)))
    {
      pixel = arithmetic_decode_target (p2->s + p2->n) < p2->s, twol = 1;
    }
  else
    {
      pixel = arithmetic_decode_target (p->s + p->n) < p->s, twol = 0;
    }

  t = twol ? (p2->s + p2->n) : (p->s + p->n);

  if (twol == 0)
    {
      if (pixel)
	arithmetic_decode (0, p->s, t);
      else
	arithmetic_decode (p->s, t, t);
      update_counts (p, pixel);
    }
  else
    {
      if (pixel)
	arithmetic_decode (0, p2->s, t);
      else
	arithmetic_decode (p2->s, t, t);
      update_counts (p2, pixel);
      update_counts (p, pixel);
    }

  *val = pixel;
}

static void 
bl_string_to_array (char inputstr[], char parsestr[], int *w, int *h)
{
  int r, c;
  int i, j, lastc, p, l;
  int xc, yc;
  int foundstatus;

  surr = twolevel = 0;
  for (i = 0; i < MAX_T_W; i++)
    MMcount[i] = MM2level[i] = 0;
  MMrows = 0;

  for (i = 0; i < (int) strlen (inputstr); i++)
    {
      p = inputstr[i];
      if ((p != '.') && (p != '2') && (p != '*') && (p != 'p') && (p != ';'))
	{
	  fprintf (stderr, "Template string can only contain [.p2*;], not \"%c\".\n", inputstr[i]);
	  exit (1);
	}
    }

  /* get the width and height, test rectangularity... */
  parsestr[l = 0] = 0;
  for (c = 0, r = 0, lastc = -1, p = 0; p < (int) strlen (inputstr); p++)
    {
      if ((inputstr[p] == ';'))
	{
	  r++;
	  if (lastc != -1)
	    {			/* this is the 2nd row at least */
	      if (lastc != c)
		{
		  fprintf (stderr, "The rows have to be the same width. "
			   "Row %d has width %d (previous row has width %d)\n", r, c, lastc);
		  exit (1);
		}
	    }
	  lastc = c;
	  c = 0;
	}
      else
	{
	  c++;
	  parsestr[l] = inputstr[p];
	  parsestr[++l] = 0;
	}
    }

  if (c != 0)
    lastc = c;
  *w = lastc;
  *h = r;
  if ((*w) <= 0)
    {
      fprintf (stderr, "Must have at least one column!\n");
      exit (1);
    }
  if ((*h) <= 0)
    {
      fprintf (stderr, "Must have at least one row! Terminate each row with a ';'\n");
      exit (1);
    }
  if ((*w > MAX_T_W) || (*h > MAX_T_W))
    {
      fprintf (stderr, "template size bigger than internal threshold of %d\n", MAX_T_W);
      exit (1);
    }

  /* get the (x,y) position of the current (*) pixel */
  for (xc = -1, yc = 0, i = 0; i < *w; i++)
    for (j = 0; j < *h; j++)
      if (parsestr[j * (*w) + i] == '*')
	{
	  if (xc >= 0)
	    {
	      fprintf (stderr, "There can only be ONE current ('*') pixel.\n");
	      exit (1);
	    }
	  xc = i;
	  yc = j;
	  curr_row = j + 1;
	}
  if (xc < 0)
    error_msg ("bl_string_to_array", "Error in mask, no current ('*') pixel.", "");
  /* found the position (xc,yc) */


  /* convert parsestr[] into a MM[][] structure */
  for (j = 0; j < *h; j++)
    {
      foundstatus = 0;
      for (i = 0; i < *w; i++)
	{
	  p = j * (*w) + i;
	  if ((parsestr[p] == 'p') || (parsestr[p] == '2'))
	    {
	      if (!foundstatus)
		foundstatus = 1;
	      MM2level[j] = MM2level[j] * 2 | (parsestr[p] == '2');
	      if (parsestr[p] == '2')
		twolevel = 1;

	      MM[j][MMcount[j]].x = i - xc;
	      MM[j][MMcount[j]++].y = j - yc;
	      if (((i - xc) >= 0) && ((j - yc) >= 0))
		surr = 1;
	    }
	  else
	    {
	      if (foundstatus == 1)
		foundstatus = 2;
	    }

	}
      if (foundstatus)
	MMrows++;
      MMbits[j] = (1 << MMcount[j]) - 1;
    }

  /* reverse matrix, so [][0] is the closest to the right */
  {
    coordinate temp;
    for (i = 0; i < MMrows; i++)
      for (j = 0; j < MMcount[i] / (unsigned) 2; j++)
	{
	  temp = MM[i][MMcount[i] - 1 - j];
	  MM[i][MMcount[i] - 1 - j] = MM[i][j];
	  MM[i][j] = temp;
	}
  }
}








void 
bl_writetemplate (char inputstr[])
{
  register int i, j;
  int w, h;
  char ind[255];
  char *str;

  str = (char *) malloc (strlen (inputstr) * sizeof (char) + 2);

  bl_string_to_array (inputstr, str, &w, &h);

  EncodeGammaDist (surr);
  EncodeGammaDist (w);
  EncodeGammaDist (h);
  ind['.'] = 0;
  ind['p'] = 1;
  ind['2'] = 2;
  ind['*'] = 3;
  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++)
      EncodeGammaDist (ind[(int) str[j * w + i]]);

  free (str);
}


void 
bl_readtemplate ()
{
  register int i, j;
  int w, h, p;
  char ind[4];
  char *inputstr, *str;

  surr = DecodeGammaDist (surr);
  if (surr)
    {
      fprintf (stderr, "It's fine compressing an image using a 'surrounding' template, but I can't "
	       "decompress it!\n");
      exit (1);
    }
  w = DecodeGammaDist ();
  h = DecodeGammaDist ();

  inputstr = (char *) malloc ((w + 2) * h * sizeof (char) + 2);
  ind[0] = '.';
  ind[1] = 'p';
  ind[2] = '2';
  ind[3] = '*';
  for (p = 0, j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
	{
	  inputstr[p++] = ind[DecodeGammaDist ()];
	}
      inputstr[p++] = ';';
      inputstr[p] = 0;
    }

  str = (char *) malloc (strlen (inputstr) * sizeof (char) + 2);
  bl_string_to_array (inputstr, str, &w, &h);
}





/* Currently if there are more than 32 bits, the topmost 32 will be discarded, if the 
   topbits are to be used in compression, define _MORETHAN32 */
#undef _MORETHAN32

void 
bl_compress_mark (marktype d)
{
  register int r, c;
  register int i, j;
  register unsigned long mask, mask2level;
  char *array[MAX_T_W];


  for (i = 0; i < MAX_T_W; i++)
    {
      array[i] = (char *) calloc ((size_t) d.w + 2 * MAX_T_W + 1, (size_t) sizeof (char));
      array[i] = array[i] + MAX_T_W;
    }

  EncodeGammaDist (d.w);
  EncodeGammaDist (d.h);

  for (r = 0; r < min (curr_row, d.h); r++)
    for (c = 0; c < d.w; c++)
      array[r % MAX_T_W][c] = pbm_getpixel (d.bitmap, c, r);

  for (r = 0; r < d.h; r++)
    {
      if (V)
	if (r && (r % 200 == 0))
	  fprintf (stderr, ".");

      if (r + curr_row < d.h)
	for (c = 0; c < d.w; c++)
	  array[(r + curr_row) % MAX_T_W][c] = pbm_getpixel (d.bitmap, c, r + curr_row);
      else
	for (c = 0; c < d.w; c++)
	  array[(r + curr_row) % MAX_T_W][c] = 0;

      for (c = 0; c < d.w; c++)
	{
	  for (mask = 0, mask2level = 0, i = 0; i < MMrows; i++)
	    {
	      if (c != 0)
		{
		  MMmask[i] = (MMmask[i] * 2);
		  MMmask[i] |= array[(r + MM[i][0].y + MAX_T_W) % MAX_T_W][c + MM[i][0].x];
		}
	      else
		{
		  for (MMmask[i] = 0, j = MMcount[i] - 1; j >= 0; j--)
		    MMmask[i] = 2 * MMmask[i] | array[(r + MM[i][j].y + MAX_T_W) % MAX_T_W][c + MM[i][j].x];
		}
	      MMmask[i] &= MMbits[i];
#ifdef _MORETHAN32
	      mask = ((mask * 8191) << MMcount[i]) | MMmask[i];
	      mask2level = ((mask2level * 499) << MMcount[i]) | (MMmask[i] & MM2level[i]);
#else
	      mask = (mask << MMcount[i]) | MMmask[i];
	      mask2level = (mask2level << MMcount[i]) | (MMmask[i] & MM2level[i]);
#endif
	    }

	  mask = HASH (mask);
	  mask2level = HASH (mask2level);

	  bl_encode_in_context (mask, mask2level, pbm_getpixel (d.bitmap, c, r));
	}
    }
  for (i = 0; i < MAX_T_W; i++)
    free (array[i] - MAX_T_W);
}



void 
bl_decompress_mark (marktype * d)
{
  register int r, c;
  register int i, j;
  unsigned long pix;
  register unsigned long mask, mask2level;
  char *array[MAX_T_W];


  d->w = DecodeGammaDist ();
  d->h = DecodeGammaDist ();

  d->bitmap = pbm_allocarray (d->w, d->h);
  for (i = 0; i < MAX_T_W; i++)
    {
      array[i] = (char *) calloc ((size_t) d->w + 2 * MAX_T_W + 1, (size_t) sizeof (char));
      array[i] = array[i] + MAX_T_W;
    }

  for (r = 0; r < d->h; r++)
    {
      if (V)
	if (r && (r % 200 == 0))
	  fprintf (stderr, ".");
      for (c = 0; c < d->w; c++)
	array[r % MAX_T_W][c] = pbm_getpixel (d->bitmap, c, r);
      for (c = 0; c < d->w; c++)
	{
	  for (mask = 0, mask2level = 0, i = 0; i < MMrows; i++)
	    {
	      if (c != 0)
		{
		  MMmask[i] = (MMmask[i] * 2);
		  MMmask[i] |= array[(r + MM[i][0].y + MAX_T_W) % MAX_T_W][c + MM[i][0].x];
		}
	      else
		{
		  for (MMmask[i] = 0, j = MMcount[i] - 1; j >= 0; j--)	/* get all pixels */
		    MMmask[i] = 2 * MMmask[i] | array[(r + MM[i][j].y + MAX_T_W) % MAX_T_W][c + MM[i][j].x];
		}
	      MMmask[i] &= MMbits[i];

#ifdef _MORETHAN32
	      mask = ((mask * 8191) << MMcount[i]) | MMmask[i];
	      mask2level = ((mask2level * 499) << MMcount[i]) | (MMmask[i] & MM2level[i]);
#else
	      mask = (mask << MMcount[i]) | MMmask[i];
	      mask2level = (mask2level << MMcount[i]) | (MMmask[i] & MM2level[i]);
#endif
	    }

	  mask = HASH (mask);
	  mask2level = HASH (mask2level);

	  bl_decode_in_context (mask, mask2level, &pix);
	  if (pix)
	    {
	      pbm_putpixel (d->bitmap, c, r, pix);
	      array[r % MAX_T_W][c] = pix;
	    }
	}
    }
  for (i = 0; i < MAX_T_W; i++)
    free (array[i] - MAX_T_W);
}



void 
bl_compress (marktype d, char inputstr[])
{
  bl_clearmodel ();
  bl_writetemplate (inputstr);
  bl_compress_mark (d);
  bl_freemodel ();
}


void 
bl_decompress (marktype * d)
{
  bl_clearmodel ();
  bl_readtemplate ();
  bl_decompress_mark (d);
  bl_freemodel ();
}






/* 
   The bitmap in d2, can be examined clairvoyantly.  The ordering of the
   params is therefore non-clairvoyant, and clairvoyant in that order.

 */
/*
   coordinate M1[6]={        { 0,-2},
   {-1,-1},{ 0,-1}, {1,-1},
   {-2,0},  {-1, 0}};
   };
   coordinate M2[13]={       { 0,-2},
   {-1,-1}, { 0,-1},   {1, -1},
   {-2,0}, {-1, 0}, { 0, 0},   { 1, 0},  {2,0},
   {-1, 1}, { 0, 1},   { 1, 1},
   { 0, 2}
 */

coordinate M1[6] =
{
  {0, -2},
  {-2, 0},
  {1, -1},
  {-1, -1},
  {0, -1},
  {-1, 0}};
coordinate M2[13] =
{
  {0, -2},
  {1, -1},
  {-2, 0},
  {2, 0},
  {-1, 1},
  {1, 1},
  {0, 2},
  {-1, -1},
  {0, -1},
  {1, 0},
  {-1, 0},
  {0, 1},
  {0, 0}
};
#define M1S 6
#define M1S_2level 3
#define M2S 13
#define M2S_2level 6


void 
bl_clair_compress (marktype d1, marktype d2)
{
  register int r, c;
  register int i, j, n;
  register unsigned long mask, mask2level = 0;
  int cols, rows;
  int gap;
  Pixel **b1, **b2;


  bl_clearmodel ();
  twolevel = 1;

  b1 = d1.bitmap;
  b2 = d2.bitmap;
  cols = d1.w;
  rows = d1.h;

  gap = max (50, rows / 10);

  EncodeGammaDist (cols);
  EncodeGammaDist (rows);

  for (r = 0; r < rows; r++)
    {
      if (V)
	if (!(r % gap))
	  fprintf (stderr, ".");
      for (c = 0; c < cols; c++)
	{

	  mask = 0;
	  for (n = 0; n < M1S; n++)
	    {
	      i = c + M1[n].x;
	      j = r + M1[n].y;
	      mask = (2 * mask) | pbm_getpixel_trunc (b1, i, j, cols, rows);
	    }

	  mask2level = mask & ((1 << M1S_2level) - 1);

	  for (n = 0; n < M2S; n++)
	    {
	      i = c + M2[n].x;
	      j = r + M2[n].y;
	      mask = (2 * mask) | pbm_getpixel_trunc (b2, i, j, cols, rows);
	    }

	  mask2level = (mask2level << M2S_2level) | (mask & ((1 << M2S_2level) - 1));

	  bl_encode_in_context (mask, mask2level, pbm_getpixel (b1, c, r));

	}
    }
  if (V)
    fprintf (stderr, "\n");
  bl_freemodel ();
}


void 
bl_clair_decompress (marktype d1, marktype d2)
{
  register int r, c;
  register int i, j, n;
  register unsigned long mask, mask2level = 0;
  unsigned long p;
  int cols, rows;
  int gap;
  Pixel **b1, **b2;

  bl_clearmodel ();
  twolevel = 1;

  b1 = d1.bitmap;
  b2 = d2.bitmap;
  cols = d1.w;
  rows = d1.h;

  gap = max (50, rows / 10);

  i = DecodeGammaDist ();
  j = DecodeGammaDist ();
  if ((i != cols) || (j != rows))
    error_msg ("bl_clair_decompress", "incorrect size of residue image", "");

  if (V)
    fprintf (stderr, "cols=%d rows=%d\n", cols, rows);

  for (r = 0; r < rows; r++)
    {
      if (V)
	if (!(r % gap))
	  fprintf (stderr, ".");
      for (c = 0; c < cols; c++)
	{
	  mask = 0;
	  for (n = 0; n < M1S; n++)
	    {
	      i = c + M1[n].x;
	      j = r + M1[n].y;
	      mask = (2 * mask) | pbm_getpixel_trunc (b1, i, j, cols, rows);
	    }
	  mask2level = mask & ((1 << M1S_2level) - 1);
	  for (n = 0; n < M2S; n++)
	    {
	      i = c + M2[n].x;
	      j = r + M2[n].y;
	      mask = (2 * mask) | pbm_getpixel_trunc (b2, i, j, cols, rows);
	    }
	  mask2level = (mask2level << M2S_2level) | (mask & ((1 << M2S_2level) - 1));


	  bl_decode_in_context (mask, mask2level, &p);
	  if (p)
	    pbm_putpixel (b1, c, r, 1);
	}
    }
  if (V)
    fprintf (stderr, "\n");
  bl_freemodel ();
}
