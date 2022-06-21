/**************************************************************************
 *
 * mg_files.c -- Routines for handling files for the auxillary programs
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
 * $Id: mg_files.c,v 1.1.1.1 1994/08/11 03:26:11 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "memlib.h"
#include "messages.h"

#include "mg_files.h"



/* This must contain a valid path without a trailing slash */
static char *basepath = NULL;



/* This sets the base path for all file operations */
void 
set_basepath (const char *bp)
{
  char *s;
  /* Free the memory for the base path if it has already been allocated */
  if (basepath)
    {
      Xfree (basepath);
      basepath = NULL;
    }

  if (!bp || *bp == '\0')
    bp = ".";

  s = strrchr (bp, '/');
  if (s && *(s + 1) == '\0')
    {
      basepath = Xmalloc (strlen (bp) + 2);
      if (!basepath)
	return;
      strcpy (basepath, bp);
      strcat (basepath, ".");
    }
  else
    basepath = Xstrdup (bp);
}



/* return the currently defined basepath */
char *
get_basepath (void)
{
  if (!basepath)
    set_basepath (getenv ("MGDATA"));
  return basepath;
}



/* This generates the name of a file. It places the name in the buffer
   specified or if that is NULL it uses a static buffer. */
char *
make_name (const char *name, const char *suffix, char *buffer)
{
  static char path[512];
  if (!buffer)
    buffer = path;
  if (!basepath)
    set_basepath (getenv ("MGDATA"));
  sprintf (buffer, "%s/%s%s", basepath, name, suffix);
  return buffer;
}



/* This will open the specified file and check its magic number.
   Mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *
open_named_file (const char *name, const char *mode,
		 u_long magic_num, int err_mode)
{
  unsigned long magic;
  FILE *f = NULL;
  char *err;
  f = fopen (name, mode);

  if (!f)
    {
      err = "Unable to open \"%s\"";
      goto error;
    }

  if (magic_num)
    {
      if (fread (&magic, sizeof (magic), 1, f) != 1)
	{
	  err = "No magic number \"%s\"";
	  goto error;
	}

      if (!IS_MAGIC (magic))
	{
	  err = "No MG magic number \"%s\"";
	  goto error;
	}

      if (magic != magic_num)
	{
	  err = "Wrong MG magic number \"%s\"";
	  goto error;
	}
    }
  return f;

error:
  if (f)
    fclose (f);
  if (err_mode == MG_ABORT)
    FatalError (1, err, name);
  if (err_mode == MG_MESSAGE)
    Message (err, name);
  return NULL;
}



/* This will open the specified file and check its magic number.
   Mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *
open_file (const char *name, const char *suffix, const char *mode,
	   u_long magic_num, int err_mode)
{
  char path[512];
  if (!basepath)
    set_basepath (getenv ("MGDATA"));
  sprintf (path, "%s/%s%s", basepath, name, suffix);
  return open_named_file (path, mode, magic_num, err_mode);
}






/* This will create the specified file and set its magic number.

   Mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *
create_named_file (const char *name, const char *mode,
		   u_long magic_num, int err_mode)
{
  FILE *f = NULL;
  char *err;
  f = fopen (name, mode);

  if (!f)
    {
      err = "Unable to open \"%s\"";
      goto error;
    }

  if (magic_num)
    if (fwrite (&magic_num, sizeof (magic_num), 1, f) != 1)
      {
	err = "Couldn't write magic number \"%s\"";
	goto error;
      }

  return f;

error:
  if (f)
    fclose (f);
  if (err_mode == MG_ABORT)
    FatalError (1, err, name);
  if (err_mode == MG_MESSAGE)
    Message (err, name);
  return NULL;
}







/* This will create the specified file and set its magic number.

   Mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *
create_file (const char *name, const char *suffix, const char *mode,
	     u_long magic_num, int err_mode)
{
  char path[512];
  if (!basepath)
    set_basepath (getenv ("MGDATA"));
  sprintf (path, "%s/%s%s", basepath, name, suffix);
  return create_named_file (path, mode, magic_num, err_mode);
}
