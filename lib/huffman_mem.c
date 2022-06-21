/**************************************************************************
 *
 * huffman_mem.c -- Huffman coding functions to memory
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
 * $Id: huffman_mem.c,v 1.1 1994/08/22 00:24:45 tes Exp $
 *
 **************************************************************************/

/*
   $Log: huffman_mem.c,v $
   * Revision 1.1  1994/08/22  00:24:45  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: huffman_mem.c,v 1.1 1994/08/22 00:24:45 tes Exp $";

#include "sysfuncs.h"
#include "bitio_m.h"
#include "bitio_m_mem.h"
#include "huffman.h"

#if 0
int fprintf (FILE *, const char *,...);
#endif

void 
BIO_Mem_Huff_Encode (unsigned long val, unsigned long *codes,
		     char *clens, mem_bitio_state * bs)
{
  ENCODE_CONTINUE (*bs)
    HUFF_ENCODE (val, codes, clens);
  ENCODE_PAUSE (*bs)
}

unsigned long 
BIO_Mem_Huff_Decode (unsigned long *mincodes,
		     unsigned long **values, mem_bitio_state * bs)
{
  unsigned long val;
  DECODE_CONTINUE (*bs)
    HUFF_DECODE (val, mincodes, values);
  DECODE_PAUSE (*bs)
    return (val);
}
