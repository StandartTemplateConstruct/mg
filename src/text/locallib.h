/**************************************************************************
 *
 * locallib.h -- Misc functions
 * Copyright (C) 1994  Gary Eddy, Alistair Moffat and Neil Sharman
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
 * $Id: locallib.h,v 1.3 1994/10/20 03:56:51 tes Exp $
 *
 **************************************************************************/

#ifndef LOCALLIB_H
#define LOCALLIB_H

#include "sysfuncs.h"

#include "text.h"

#define NUMOF(a) (sizeof(a)/sizeof((a)[0]))

int vecentropy (int *A, int n);

int huffcodebits (unsigned long *A, int n);

int modelbits (unsigned long *A, int n);

int prime (int p);


int Read_cdh (FILE * f, compression_dict_header * cdh, u_long * mem, u_long * disk);

int Read_cfh (FILE * f, comp_frags_header * cfh, u_long * mem, u_long * disk);

int F_Read_cdh (File * f, compression_dict_header * cdh, u_long * mem,
		u_long * disk);

int F_Read_cfh (File * f, comp_frags_header * cfh, u_long * mem, u_long * disk);


#endif
