/**************************************************************************
 *
 * stemmer.h -- The stemmer/case folder
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
 * $Id: stemmer.h,v 1.3 1994/10/20 03:57:06 tes Exp $
 *
 **************************************************************************/

#ifndef STEMMER_H
#define STEMMER_H

#include "sysfuncs.h"
#include "stem.h"

#define STEMMER_MASK 3

/*
 * Method 0 - Do not stem or case fold.
 * Method 1 - Case fold.
 * Method 2 - Stem.
 * Method 3 - Case fold and stem.
 */
void stemmer (int method, u_char * word);

#endif
