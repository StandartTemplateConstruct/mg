/**************************************************************************
 *
 * match.h -- Functions related to matching marks
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
 * $Id: match.h,v 1.2 1994/09/20 04:41:42 tes Exp $
 *
 **************************************************************************/

#ifndef __MATCH_H
#define __MATCH_H

#include "marklist.h"
#include "pbmtools.h"
#include "utils.h"


/*
   #define NOT_SCREENED(b1,b2) ((abs(b1.w-b2.w)<=4) && (abs(b1.h-b2.h)<=4))
 */

#define NOT_SCREENED(b1,b2) ((abs(b1.xcen-b2.xcen)+abs(b1.ycen-b2.ycen))<=3)

#define MATCH_VAL 1000
#define MATCH(b1,b2) (CSIS_match(b1,b2))

int XOR_match (marktype b1, marktype b2);
int WXOR_match (marktype b1, marktype b2);
int WAN_match (marktype b1, marktype b2);
int PMS_match (marktype b1, marktype b2);
int CSIS_match (marktype * b1, marktype * b2);

#endif
