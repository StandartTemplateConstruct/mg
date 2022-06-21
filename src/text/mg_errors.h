/**************************************************************************
 *
 * mg_errors.h -- Error related stuff for mgquery
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
 * $Id: mg_errors.h,v 1.2 1994/09/20 04:41:46 tes Exp $
 *
 **************************************************************************/


extern int mg_errno;
extern char *mg_error_data;
extern char *mg_errorstrs[];


#define MG_NOERROR 0
#define MG_NOMEM 1
#define MG_NOFILE 2
#define MG_BADMAGIC 3
#define MG_READERR 4
#define MG_BUFTOOSMALL 5
#define MG_INVERSION 6

void MgErrorData (char *s);
