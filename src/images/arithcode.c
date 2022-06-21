/**************************************************************************
 *
 * arithcode.c -- Arithmetic coding
 * Copyright (C) 1994  Alistair Moffat
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
 *
 * $Id: arithcode.c,v 1.1.1.1 1994/08/11 03:26:09 tes Exp $
 *
 **************************************************************************
 *
 *	Arithmetic encoding. Taken largely from Witten, Neal, Cleary,
 *		CACM, 1987, p520.
 *	Includes bitfiddling routines.
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "arithcode.h"


FILE *arith_in, *arith_out;

/*
   $log$
 */

static char *RCSID = "$Id: arithcode.c,v 1.1.1.1 1994/08/11 03:26:09 tes Exp $";

#undef BECAREFUL
#define INTEGRATED


codevalue S_low = 0, S_high = 0, S_value = 0;
long S_bitstofollow = 0;
int S_buffer = 0, S_bitstogo = 0;

long cmpbytes = 0, rawbytes = 0;

#ifdef INTEGRATED
long CountOfBitsOut;
#endif


/*==================================*/

#define BITPLUSFOLLOW(b)		\
{	OUTPUTBIT((b));			\
	while (bitstofollow)		\
	{	OUTPUTBIT(!(b));	\
		bitstofollow -= 1;	\
	}				\
}

#ifdef INTEGRATED
#define OUTPUTBIT(b)			\
{	buffer >>= 1;			\
	if (b) buffer |= 0x80;	 	\
	bitstogo -= 1;			\
	if (bitstogo==0)		\
	{	putc(buffer, arith_out);	\
		bitstogo = 8;		\
		cmpbytes += 1;		\
	}				\
	CountOfBitsOut++;		\
}
#else
#define OUTPUTBIT(b)			\
{	buffer >>= 1;			\
	if (b) buffer |= 0x80;	 	\
	bitstogo -= 1;			\
	if (bitstogo==0)		\
	{	putc(buffer, arith_out);	\
		bitstogo = 8;		\
		cmpbytes += 1;		\
	}				\
}
#endif

#define ADDNEXTINPUTBIT(v)		\
{	if (bitstogo==0)		\
	{	buffer = getc(arith_in);	\
		bitstogo = 8;		\
	}				\
	v = (v<<1)+(buffer&1);		\
	buffer >>= 1;			\
	bitstogo -=1;			\
}

/*==================================*/

void
arithmetic_encode (lbnd, hbnd, totl)
     int lbnd, hbnd, totl;
{
  register codevalue low, high;

#ifdef BECAREFUL
  if (!(0 <= lbnd && lbnd < hbnd && hbnd <= totl && totl < maxfrequency))
    {
      fprintf (stderr, "(corrupt file) coding error: %d %d %d\n", lbnd, hbnd, totl);
      abort ();
    }
#endif

  {
    register codevalue range;
    range = S_high - S_low + 1;
    high = S_low + range * hbnd / totl - 1;
    low = S_low + range * lbnd / totl;
  }

  {
    register codevalue H;
    register long bitstofollow;
    register int buffer, bitstogo;

    bitstofollow = S_bitstofollow;
    buffer = S_buffer;
    bitstogo = S_bitstogo;
    H = half;

    for (;;)
      {
	if (high < H)
	  {
	    BITPLUSFOLLOW (0);
	  }
	else if (low >= H)
	  {
	    BITPLUSFOLLOW (1);
	    low -= H;
	    high -= H;
	  }
	else if (low >= firstqtr && high < thirdqtr)
	  {
	    bitstofollow++;
	    low -= firstqtr;
	    high -= firstqtr;
	  }
	else
	  break;
	low += low;
	high += high;
	high++;
      }
    S_bitstofollow = bitstofollow;
    S_buffer = buffer;
    S_bitstogo = bitstogo;
    S_low = low;
    S_high = high;
  }
}

/*==================================*/

/* Made into a macro 

   codevalue
   arithmetic_decode_target(totl)
   register int totl;
   {    return (((S_value-S_low+1)*totl-1) / (S_high -S_low+1));
   } 
 */

/*==================================*/

void
arithmetic_decode (lbnd, hbnd, totl)
     int lbnd, hbnd, totl;
{
  register codevalue low, high;

#ifdef BECAREFUL
  if (!(0 <= lbnd && lbnd < hbnd && hbnd <= totl && totl < maxfrequency))
    {
      fprintf (stderr, "(corrupt file) coding error: %d %d %d\n", lbnd, hbnd, totl);
      abort ();
    }
#endif

  {
    register codevalue range;
    range = S_high - S_low + 1;
    high = S_low + range * hbnd / totl - 1;
    low = S_low + range * lbnd / totl;
  }

  {
    register codevalue value, H;
    register int buffer, bitstogo;

    buffer = S_buffer;
    bitstogo = S_bitstogo;
    H = half;
    value = S_value;

    for (;;)
      {
	if (high < H)
	  {			/* nothing */
	  }
	else if (low >= H)
	  {
	    value -= H;
	    low -= H;
	    high -= H;
	  }
	else if (low >= firstqtr && high < thirdqtr)
	  {
	    value -= firstqtr;
	    low -= firstqtr;
	    high -= firstqtr;
	  }
	else
	  break;
	low += low;
	high += high;
	high += 1;
	ADDNEXTINPUTBIT (value);
      }
    S_buffer = buffer;
    S_bitstogo = bitstogo;
    S_low = low;
    S_high = high;
    S_value = value;
  }
}

/*==================================*/


static void
startoutputingbits ()
{
  S_buffer = 0;
  S_bitstogo = 8;
}

static void
startinputingbits ()
{
  S_buffer = 0;
  S_bitstogo = 0;
}

static void
doneoutputingbits ()
{				/* write another byte if necessary */
  if (S_bitstogo < 8)
    {
      putc (S_buffer >> S_bitstogo, arith_out);
      cmpbytes += 1;
    }
}

/*==================================*/

static void
startencoding ()
{
  S_low = 0;
  S_high = topvalue;
  S_bitstofollow = 0;
}

static void
startdecoding ()
{
  register int i;
  register int buffer, bitstogo;
  S_low = 0;
  S_high = topvalue;
  S_value = 0;
  bitstogo = S_bitstogo;
  buffer = S_buffer;
  for (i = 0; i < codevaluebits; i++)
    ADDNEXTINPUTBIT (S_value);
  S_bitstogo = bitstogo;
  S_buffer = buffer;
}

static void
doneencoding ()
{
  register long bitstofollow;
  register int buffer, bitstogo;
  bitstofollow = S_bitstofollow;
  buffer = S_buffer;
  bitstogo = S_bitstogo;
  bitstofollow += 1;
  if (S_low < firstqtr)
    {
      BITPLUSFOLLOW (0);
    }
  else
    {
      BITPLUSFOLLOW (1);
    }
  S_bitstofollow = bitstofollow;
  S_buffer = buffer;
  S_bitstogo = bitstogo;
}



void 
EncodeGammaDist (int Off)
/* Encode Off with probability distribution derived from the Elias gamma distribution. */
{
  int Len, i;

  Off++;
  for (Len = 31; Len >= 0 && !(Off & (1 << Len)); Len--);

  for (i = Len; i >= 0; i--)
    arithmetic_encode (0, 1, 2);
  arithmetic_encode (1, 2, 2);

  for (i = Len - 1; i >= 0; i--)
    if (Off & (1 << i))
      arithmetic_encode (1, 2, 2);
    else
      arithmetic_encode (0, 1, 2);
}



int 
DecodeGammaDist ()
/* Decode next Off using probability distribution derived from the Elias gamma distribution. Inverse of EncodeGammaDist. */
{
  int i = 1, Len = 0, bit;

  do
    {
      bit = arithmetic_decode_target (2) < 1 ? 0 : 1;
      if (bit)
	arithmetic_decode (1, 2, 2);
      else
	{
	  arithmetic_decode (0, 1, 2);
	  Len++;
	}
    }
  while (!bit);

  Len--;
  while (Len-- > 0)
    {
      bit = arithmetic_decode_target (2) < 1 ? 0 : 1;
      if (bit)
	arithmetic_decode (1, 2, 2);
      else
	arithmetic_decode (0, 1, 2);
      i = (i << 1) | bit;
    }
  return (i - 1);
}





void 
EncodeGammaSigned (int snum, int *pos, int *neg)
{
  if (snum >= 0)
    arithmetic_encode (0, *pos, *pos + (*neg)), (*pos)++;
  else
    arithmetic_encode (*pos, *pos + (*neg), *pos + (*neg)), (*neg)++;
  if (*pos + (*neg) >= maxfrequency)
    {
      *pos = (*pos + 1) / 2;
      *neg = (*neg + 1) / 2;
    }
  EncodeGammaDist (abs (snum));
}


int 
DecodeGammaSigned (int *pos, int *neg)
{
  int signedpos = 1;
  if (arithmetic_decode_target (*pos + (*neg)) < *pos)
    arithmetic_decode (0, *pos, *pos + (*neg)), (*pos)++;
  else
    arithmetic_decode (*pos, *pos + (*neg), *pos + (*neg)), (*neg)++, signedpos = 0;

  if (*pos + (*neg) >= maxfrequency)
    {
      *pos = (*pos + 1) / 2;
      *neg = (*neg + 1) / 2;
    }

  if (signedpos)
    return DecodeGammaDist ();
  else
    return (-DecodeGammaDist ());
}


void 
InitArithEncoding (void)
{
  S_low = 0, S_high = 0, S_value = 0;
  S_bitstofollow = 0;
  S_buffer = 0, S_bitstogo = 0;
  cmpbytes = 0, rawbytes = 0;

  arith_in = stdin;
  arith_out = stdout;
  
  startoutputingbits ();
  startencoding ();
}

void 
InitArithDecoding (void)
{
  S_low = 0, S_high = 0, S_value = 0;
  S_bitstofollow = 0;
  S_buffer = 0, S_bitstogo = 0;
  cmpbytes = 0, rawbytes = 0;

  arith_in = stdin;
  arith_out = stdout;
  
  startinputingbits ();
  startdecoding ();
}

void 
CloseDownArithEncoding (void)
{
  doneencoding ();
  doneoutputingbits ();
}

void 
CloseDownArithDecoding (void)
{
}





/* encode that famous number, 42 uses 12 bits */
void 
EncodeChecksum ()
{
  EncodeGammaDist (42);
}

void 
DecodeChecksum (char str[])
{
  if (DecodeGammaDist () != 42)
    {
      fprintf (stderr, "%s: checksum error, file corrupt.\n", str);
      exit (1);
    }
}
