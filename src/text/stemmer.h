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
 * $Id: stemmer.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#ifndef STEMMER_H
#define STEMMER_H

#include "sysfuncs.h"

#define STEMMER_MASK 3
#define MAX_STEM_DESCRIPTION_LEN 16

/* stemmernumber will return the stemmer for
 * a description of the stemmer. Stemmer descriptions
 * are not case sensitive. Valid descriptions are:
 *
 *   'English'
 *   'Lovin'
 *   'French'
 *   'SimpleFrench'
 *
 * More than one description might result in the same
 * stemmer number (for example, for stemming 'English'
 * we currently use the 'Lovin' stemmer).
 *
 * stemmerdescription is a normal C, null-terminated,
 * string.
 *
 * stemmernumber will return -1 if it doesn't know the
 * stemmer description.
 */
int stemmernumber (const u_char *stemmerdescription);

/*
 * Method 0 - Do not stem or case fold.
 * Method 1 - Case fold.
 * Method 2 - Stem.
 * Method 3 - Case fold and stem.
 *
 * The stemmer number should be obtained using function
 * stemmernumber above.
 */
void stemmer (int method, int stemmer, u_char * word);

#endif
