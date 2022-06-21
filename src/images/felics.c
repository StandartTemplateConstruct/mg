/**************************************************************************
 *
 * felics.c -- The main guts of the felics algorithm
 * Copyright (C) 1994  Neil Sharman and Kerry Guise
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
 * $Id: felics.c,v 1.1.1.1 1994/08/11 03:26:10 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "bitio_m.h"
#include "bitio_m_stdio.h"


#undef putw
#define putw(x)	{					\
	short z;					\
	byteptr = (unsigned char *)(x);			\
	for (z = 0; z < sizeof(int); z++)		\
		(void) putc((int) byteptr[z], fp_out);	\
}
#define INTSIZE sizeof(int) * 8
#define BOOLEAN short
#define TRUE 1
#define FALSE 0

#define IN_RANGE 0
#define OUT_OF_RANGE 1
#define BELOW_RANGE 0
#define ABOVE_RANGE 1


void 
encode (unsigned int *line, int width, int height, int maxval, int maxk,
	FILE * fp_in, FILE * fp_out, int (*get) (FILE *))
{
  int x, y;
  unsigned long P, L, H, N1, N2, delta;
  unsigned long cumul_param[256][INTSIZE];
  unsigned max_cumul;

  memset (cumul_param, '0', sizeof (cumul_param));

  if (maxk)
    max_cumul = maxk;
  else
    CEILLOG_2 (maxval, max_cumul);

  ENCODE_START (fp_out)

    for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
	if (y > 0)
	  {
	    if (x > 0)
	      {			/* ******** */
		N1 = line[x];	/* ****1*** */
		N2 = line[x - 1];	/* ***2P*** */
	      }			/* ******** */
	    else
	      {			/* |******* */
		N1 = line[x];	/* |12***** */
		N2 = line[x + 1];	/* |P***** */
	      }			/* |******* */
	  }
	else
	  {
	    if (x > 1)
	      {
		N1 = line[x - 1];	/* -------- */
		N2 = line[x - 2];	/* **12P*** */
	      }			/* ******** */
	    else if (x > 0)
	      {
		N1 = line[x - 1];	/* +------- */
		N2 = N1;	/* |3P***** */
	      }			/* |******* */
	    else
	      {
		line[x] = get (fp_in);	/* +------- */
		BINARY_ENCODE (line[x] + 1, maxval + 1);	/* |P****** */
		continue;	/* |******* */
	      }
	  }


	H = N1 > N2 ? N1 : N2;
	L = N1 < N2 ? N1 : N2;
	delta = H - L;

	line[x] = P = get (fp_in);

	if ((L <= P) && (P <= H))
	  {
	    /* IN-RANGE */
	    unsigned long range, logofrange, thresh, numlong;
	    int nbits;

	    ENCODE_BIT (IN_RANGE);

	    P -= L;

	    range = delta + 1;

	    if (range > 1)
	      {
		/* 2**i <= delta + 1 < 2**(i+1) ; find i ... */
		CEILLOG_2 (range, logofrange);

		/* number of n-bit values in the range */
		thresh = (1 << logofrange) - range;

		/* number of n+1-bit values in the range */
		numlong = range - thresh;

		/* rotate the n-bit codes into the centre */
		P -= (numlong >> 1);
		if ((long) P < 0)
		  P += range;

		if (P < thresh)
		  nbits = logofrange - 1;
		else
		  {
		    nbits = logofrange;
		    P += thresh;
		  }
		while (--nbits >= 0)
		  ENCODE_BIT ((P >> nbits) & 1);
	      }
	  }
	else
	  {
	    /* OUT_OF_RANGE */
	    unsigned long diff, parm, i, min;

	    ENCODE_BIT (OUT_OF_RANGE);

	    if (P < L)
	      {
		ENCODE_BIT (BELOW_RANGE);
		diff = L - P - 1;
	      }
	    else
	      {
		ENCODE_BIT (ABOVE_RANGE);
		diff = P - H - 1;
	      }

	    /* Now code 'diff' as a Golomb-Rice code */

	    parm = 0;
	    min = cumul_param[delta][0];
	    for (i = 1; i < max_cumul; i++)
	      if (cumul_param[delta][i] < min)
		{
		  min = cumul_param[delta][i];
		  parm = i;
		}

	    UNARY_ENCODE ((diff >> parm) + 1);
	    BINARY_ENCODE ((diff & ((1 << parm) - 1)) + 1, 1 << parm);

	    for (i = 0; i < max_cumul; i++)
	      cumul_param[delta][i] += (diff >> i) + 1 + i;
	  }

      }

  ENCODE_DONE

}






void 
decode (unsigned int *line, int width, int height, int maxval, int maxk,
	FILE * fp_in, FILE * fp_out)
{
  int x, y;
  unsigned int P, L, H, N1, N2, delta;
  unsigned long cumul_param[256][INTSIZE];
  unsigned max_cumul;

  memset (cumul_param, '0', sizeof (cumul_param));

  if (maxk)
    max_cumul = maxk;
  else
    CEILLOG_2 (maxval, max_cumul);

  DECODE_START (fp_in);

  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
	if (y > 0)
	  {
	    if (x > 0)
	      {			/* ******** */
		N1 = line[x];	/* ****1*** */
		N2 = line[x - 1];	/* ***2P*** */
	      }			/* ******** */
	    else
	      {			/* |******* */
		N1 = line[x];	/* |12***** */
		N2 = line[x + 1];	/* |P***** */
	      }			/* |******* */
	  }
	else
	  {
	    if (x > 1)
	      {
		N1 = line[x - 1];	/* -------- */
		N2 = line[x - 2];	/* **12P*** */
	      }			/* ******** */
	    else if (x > 0)
	      {
		N1 = line[x - 1];	/* +------- */
		N2 = N1;	/* |3P***** */
	      }			/* |******* */
	    else
	      {
		BINARY_DECODE (line[x], maxval + 1);
		line[x]--;
		if (maxval < 256)
		  putc (line[x], fp_out);	/* +------- */
		else		/* |P****** */
		  fprintf (fp_out, "%u ", line[x]);	/* |******* */
		continue;
	      }
	  }


	H = N1 > N2 ? N1 : N2;
	L = N1 < N2 ? N1 : N2;
	delta = H - L;

	if (DECODE_BIT == IN_RANGE)
	  {
	    unsigned long i, range, logofrange, thresh, numlong;
	    unsigned xx, l;

	    range = delta + 1;

	    if (range > 1)
	      {
		/* 2**i <= delta + 1 < 2**(i+1) ; find i ... */
		CEILLOG_2 (range, logofrange);

		/* number of n-bit values in the range */
		thresh = (1 << logofrange) - range;
		for (P = i = 0; i < logofrange - 1; i++)
		  DECODE_ADD (P);
		xx = P;
		l = logofrange - 1;
		if (P >= thresh)
		  {
		    DECODE_ADD (P);
		    xx = P;
		    l++;
		    P -= thresh;
		  }

		/* number of n+1-bit values in the range */
		numlong = range - thresh;

		/* rotate the n-bit codes into the centre */
		P += (numlong >> 1);
		if ((long) P >= range)
		  P -= range;

		P += L;
	      }
	    else
	      P = L;
	  }
	else
	  {
	    int out_of_range = DECODE_BIT;
	    unsigned long diff, unary, binary, i, min, parm;

	    /* Now code 'diff' as a Golomb-Rice code */

	    parm = 0;
	    min = cumul_param[delta][0];
	    for (i = 1; i < max_cumul; i++)
	      if (cumul_param[delta][i] < min)
		{
		  min = cumul_param[delta][i];
		  parm = i;
		}

	    UNARY_DECODE (unary);
	    diff = (unary - 1) << parm;
	    BINARY_DECODE (binary, 1 << parm);
	    diff |= binary - 1;

	    for (i = 0; i < max_cumul; i++)
	      cumul_param[delta][i] += (diff >> i) + 1 + i;

	    if (out_of_range == BELOW_RANGE)
	      {
		P = L - diff - 1;
	      }
	    else
	      {
		P = diff + H + 1;
	      }
	  }

	line[x] = P;

	if (maxval < 256)
	  (void) putc (P, fp_out);
	else
	  fprintf (fp_out, "%u ", P);
      }

  DECODE_DONE
}
