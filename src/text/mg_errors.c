/**************************************************************************
 *
 * mg_errors.c -- Error related stuff for mgquery
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
 * $Id: mg_errors.c,v 1.1.1.1 1994/08/11 03:26:11 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "memlib.h"

#include "mg_errors.h"



int mg_errno = MG_NOERROR;

char *mg_error_data = NULL;

char *mg_errorstrs[] =
{
  "No error",
  "Out of memory",
  "File \"%s\" not found",
  "Bad magic number in \"%s\"",
  "Error reading \"%s\"",
  "MG_BUFTOOSMALL",
  "Files required for level 2 and 3 inversion are missing"};



static char null_data[] = "";



void 
MgErrorData (char *s)
{
  if (mg_error_data)
    Xfree (mg_error_data);

  mg_error_data = Xstrdup (s);

  if (!mg_error_data)
    mg_error_data = null_data;
}
