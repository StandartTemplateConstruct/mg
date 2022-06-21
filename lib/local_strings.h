/**************************************************************************
 *
 * local_strings.h -- string routines
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
 * $Id: local_strings.h,v 1.2 1994/07/05 01:17:15 tes Exp $
 *
 **************************************************************************/

#ifndef LOCAL_STRINGS_H
#define LOCAL_STRINGS_H

#include "sysfuncs.h"

int arg_atoi (char *str);
int compare (u_char * s1, u_char * s2);
u_char *copy_string (u_char * src_str);
char *str255_to_string (u_char * str255, char *str);
int prefixlen (u_char * s1, u_char * s2);
char *char2str (u_char c);
char *word2str (u_char * s);
char *de_escape_string (char *s);

#endif
