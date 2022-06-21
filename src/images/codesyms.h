/**************************************************************************
 *
 * codesyms.h -- Functions which code the symbol sequence
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
 * $Id: codesyms.h,v 1.2 1994/09/20 04:41:21 tes Exp $
 *
 **************************************************************************/
#ifndef _CodeSymbols_h_
#define _CodeSymbols_h_

#include "marklist.h"
#include "arithcode.h"

#ifndef FIXEDORDER
#define FIXEDORDER	3
#endif

#define PPM_CHUNK	1024

extern int kbytes;
extern void decodestring ();
extern void encodestring ();

void EncodeSymbols (marklistptr MarkList, int NSyms);
marklistptr DecodeSymbols (int NSyms);

void InitPPM (void);

#endif
