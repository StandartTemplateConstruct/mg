/**************************************************************************
 *
 * bilevel.h -- Compress a bilevel bitmap
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
 * $Id: bilevel.h,v 1.2 1994/09/20 04:41:17 tes Exp $
 *
 **************************************************************************/

#ifndef __BILEVEL_H
#define __BILEVEL_H

#include "marklist.h"

/* must be called before the start of de/compression */
void bl_clearmodel ();
void bl_freemodel ();

/* writes the template to the stream, and sets up the de/compression params */
void bl_writetemplate (char inputstr[]);
void bl_readtemplate ();

/* actually performs the de/compression */
void bl_compress (marktype d, char str[]);
void bl_compress_mark (marktype d);
void bl_decompress (marktype * d);
void bl_decompress_mark (marktype * d);

/* clairvoyantly de/compress d1 with respect to clairvoyantly viewable d2 */
void bl_clair_compress (marktype d1, marktype d2);
void bl_clair_decompress (marktype d1, marktype d2);

#endif
