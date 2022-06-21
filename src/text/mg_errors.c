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
 * $Id: mg_errors.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "memlib.h"

#include "mg_errors.h"



int mg_errno = MG_NOERROR;

char *mg_errorstrs[] =
{
  "No error",
  "Out of memory",
  "File \"%s\" not found",
  "Bad magic number in \"%s\"",
  "Error reading \"%s\"",
  "MG_BUFTOOSMALL",
  "Files required for level 2 and 3 inversion are missing"};


/* [RJM 07/98: Memory Leak] -- rest of file */

static char null_data[] = "";
char *mg_error_data = null_data;


void MgErrorData (char *s)
{
  /* free the current error string, unless it is the null string */
  if ((mg_error_data != NULL) && (mg_error_data != null_data)) {
    Xfree (mg_error_data);
    mg_error_data = null_data;
  }

  /* make a copy of the string */
  mg_error_data = Xstrdup (s);

  /* if the alloc failed set the error to the null string */
  if (!mg_error_data) mg_error_data = null_data;
}


void MgErrorDeinit (void)
{
  /* free the current error string, unless it is the null string */
  if ((mg_error_data != NULL) && (mg_error_data != null_data)) {
    Xfree (mg_error_data);
    mg_error_data = null_data;
  }
}
