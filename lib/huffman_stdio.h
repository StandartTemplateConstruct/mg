/**************************************************************************
 *
 * huffman_stdio.h -- Huffman coding function to a file
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
 * $Id: huffman_stdio.h,v 1.2 1994/09/20 04:20:05 tes Exp $
 *
 **************************************************************************/

#ifndef H_HUFFMAN_STDIO
#define H_HUFFMAN_STDIO



void BIO_Stdio_Huff_Encode (unsigned long val, unsigned long *codes,
			    char *clens, stdio_bitio_state * bs);


unsigned long BIO_Stdio_Huff_Decode (unsigned long *mincodes,
				     unsigned long **values,
				     stdio_bitio_state * bs);


#endif
