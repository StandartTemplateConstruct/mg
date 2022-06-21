/**************************************************************************
 *
 * bitio_stdio.c -- Functions for bitio to a file
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
 * $Id: bitio_stdio.c,v 1.1 1994/08/22 00:24:41 tes Exp $
 *
 **************************************************************************/

/*
   $Log: bitio_stdio.c,v $
   * Revision 1.1  1994/08/22  00:24:41  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: bitio_stdio.c,v 1.1 1994/08/22 00:24:41 tes Exp $";



#include "sysfuncs.h"

#include "bitio_m_stdio.h"
#include "bitio_m.h"



void 
BIO_Stdio_Encode_Start (FILE * f, stdio_bitio_state * bs)
{
  ENCODE_START (f)
    ENCODE_PAUSE (*bs)
}

void 
BIO_Stdio_Encode_Done (stdio_bitio_state * bs)
{
  ENCODE_CONTINUE (*bs)
    ENCODE_DONE
}


void 
BIO_Stdio_Decode_Start (FILE * f, stdio_bitio_state * bs)
{
  DECODE_START (f)
    DECODE_PAUSE (*bs)
}

void 
BIO_Stdio_Encode_Bit (int bit, stdio_bitio_state * bs)
{
  ENCODE_CONTINUE (*bs)
    ENCODE_BIT (bit);
  ENCODE_PAUSE (*bs)
}


int 
BIO_Stdio_Decode_Bit (stdio_bitio_state * bs)
{
  register int val;
  DECODE_CONTINUE (*bs)
    val = DECODE_BIT;
  DECODE_PAUSE (*bs)
    return (val);
}




void 
BIO_Stdio_Unary_Encode (unsigned long val, stdio_bitio_state * bs,
			unsigned long *bits)
{
  ENCODE_CONTINUE (*bs)
    if (bits)
    UNARY_ENCODE_L (val, *bits);
  else
    UNARY_ENCODE (val);
  ENCODE_PAUSE (*bs)
}


unsigned long 
BIO_Stdio_Unary_Decode (stdio_bitio_state * bs,
			unsigned long *bits)
{
  register unsigned long val;
  DECODE_CONTINUE (*bs)
    if (bits)
    UNARY_DECODE_L (val, *bits);
  else
    UNARY_DECODE (val);
  DECODE_PAUSE (*bs)
    return (val);
}







void 
BIO_Stdio_Binary_Encode (unsigned long val, unsigned long b,
			 stdio_bitio_state * bs, unsigned long *bits)
{
  ENCODE_CONTINUE (*bs)
    if (bits)
    BINARY_ENCODE_L (val, b, *bits);
  else
    BINARY_ENCODE (val, b);
  ENCODE_PAUSE (*bs)
}


unsigned long 
BIO_Stdio_Binary_Decode (unsigned long b, stdio_bitio_state * bs,
			 unsigned long *bits)
{
  register unsigned long val;
  DECODE_CONTINUE (*bs)
    if (bits)
    BINARY_DECODE_L (val, b, *bits);
  else
    BINARY_DECODE (val, b);
  DECODE_PAUSE (*bs)
    return (val);
}







void 
BIO_Stdio_Gamma_Encode (unsigned long val, stdio_bitio_state * bs,
			unsigned long *bits)
{
  ENCODE_CONTINUE (*bs)
    if (bits)
    GAMMA_ENCODE_L (val, *bits);
  else
    GAMMA_ENCODE (val);
  ENCODE_PAUSE (*bs)
}


unsigned long 
BIO_Stdio_Gamma_Decode (stdio_bitio_state * bs,
			unsigned long *bits)
{
  register unsigned long val;
  DECODE_CONTINUE (*bs)
    if (bits)
    GAMMA_DECODE_L (val, *bits);
  else
    GAMMA_DECODE (val);
  DECODE_PAUSE (*bs)
    return (val);
}




void 
BIO_Stdio_Delta_Encode (unsigned long val, stdio_bitio_state * bs,
			unsigned long *bits)
{
  ENCODE_CONTINUE (*bs)
    if (bits)
    DELTA_ENCODE_L (val, *bits);
  else
    DELTA_ENCODE (val);
  ENCODE_PAUSE (*bs)
}


unsigned long 
BIO_Stdio_Delta_Decode (stdio_bitio_state * bs,
			unsigned long *bits)
{
  register unsigned long val;
  DECODE_CONTINUE (*bs)
    if (bits)
    DELTA_DECODE_L (val, *bits);
  else
    DELTA_DECODE (val);
  DECODE_PAUSE (*bs)
    return (val);
}




void 
BIO_Stdio_Elias_Encode (unsigned long val, unsigned long b, double s,
			stdio_bitio_state * bs, unsigned long *bits)
{
  ENCODE_CONTINUE (*bs)
    if (bits)
    ELIAS_ENCODE_L (val, b, s, *bits);
  else
    ELIAS_ENCODE (val, b, s);
  ENCODE_PAUSE (*bs)
}


unsigned long 
BIO_Stdio_Elias_Decode (unsigned long b, double s,
			stdio_bitio_state * bs,
			unsigned long *bits)
{
  register unsigned long val;
  DECODE_CONTINUE (*bs)
    if (bits)
    ELIAS_DECODE_L (val, b, s, *bits);
  else
    ELIAS_DECODE (val, b, s);
  DECODE_PAUSE (*bs)
    return (val);
}



void 
BIO_Stdio_Bblock_Encode (unsigned long val, unsigned long b,
			 stdio_bitio_state * bs, unsigned long *bits)
{
  ENCODE_CONTINUE (*bs)
    if (bits)
    BBLOCK_ENCODE_L (val, b, *bits);
  else
    BBLOCK_ENCODE (val, b);
  ENCODE_PAUSE (*bs)
}


unsigned long 
BIO_Stdio_Bblock_Decode (unsigned long b, stdio_bitio_state * bs,
			 unsigned long *bits)
{
  register unsigned long val;
  DECODE_CONTINUE (*bs)
    if (bits)
    BBLOCK_DECODE_L (val, b, *bits);
  else
    BBLOCK_DECODE (val, b);
  DECODE_PAUSE (*bs)
    return (val);
}

void 
BIO_Stdio_Decode_Seek (unsigned long pos, stdio_bitio_state * bs)
{
  DECODE_CONTINUE (*bs)
    DECODE_SEEK (pos);
  DECODE_PAUSE (*bs)
}
