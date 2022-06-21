/**************************************************************************
 *
 * mgticdump.c -- Program to dump out a library in a human readable form
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
 * $Id: mgticdump.c,v 1.1.1.1 1994/08/11 03:26:12 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "pbmtools.h"
#include "extractor.h"
#include "utils.h"


void 
usage ()
{
  fprintf (stderr, "usage:\n"
	   "\tmgticdump   libraryfile\n");
  exit (1);
}



int 
main (int argc, char *args[])
{
  FILE *lib;
  int count, i;
  marktype d;
  char *libraryname = NULL;
  FILE *outf = (stdout);

  if (argc < 2)
    usage ();

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (args[i], "-h"))
	usage ();
      else if (args[i][0] == '-')
	error_msg (args[0], "unknown switch:", args[i]);
      else if (!libraryname)
	libraryname = args[i];
      else
	error_msg (args[0], "too many filenames", "");
    }

  if (!libraryname)
    error_msg (args[0], "please specify a library file", "");


  lib = fopen (libraryname, "rb");
  if (lib == NULL)
    error_msg (args[0], "trouble opening library file:", libraryname);

  count = 0;
  while (!isEOF (lib))
    {
      read_library_mark (lib, &d);

      write_library_mark (outf, d);
      count++;

      pbm_freearray (&d.bitmap, d.h);
    }
  fprintf (stderr, "count=%d", count);
  fclose (lib);
  exit(0);
}
