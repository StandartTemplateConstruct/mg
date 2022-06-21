/**************************************************************************
 *
 * extractor.h -- Functions related to extracting marks
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
 * $Id: extractor.h,v 1.2 1994/09/20 04:41:27 tes Exp $
 *
 **************************************************************************/


#ifndef _EXTRACTOR_H_
#define _EXTRACTOR_H_

#include "marklist.h"
#include "pbmtools.h"

extern int splitjoined;

void RunRegionFill (Pixel ** bitmap, marktype * mark, int i, int j, int xl, int yt, int cols, int rows);

int ExtractNextMark (Pixel ** bitmap, marklistptr * marklist, int *tx, int *ty, int cols, int rows);
void ExtractAllMarks (Pixel ** bitmap, marklistptr * marklist, int cols, int rows);

#endif
