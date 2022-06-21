/**************************************************************************
 *
 * mgticbuild.c -- Program to build the textual image library
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
 * $Id: mgticbuild.c,v 1.1.1.1 1994/08/11 03:26:12 tes Exp $
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
  fprintf (stderr, "usage: \n"
	"\tmgticbuild   libraryfile   infile.pbm  [infile2 infile3 ...]\n");
  exit (1);
}

int 
main (int argc, char *args[])
{
  int i;
  FILE *lib = NULL, *inf = NULL;
  marklistptr list = NULL, step;
  Pixel **bitmap;
  int cols, rows;
  int count, pos = 0;
  char *libraryname = NULL;

  if (argc < 2)
    usage ();

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (args[i], "-v"))
	V = 1;
      else if (!strcmp (args[i], "-h"))
	usage ();
      else if (args[i][0] == '-')
	error_msg (args[0], "unknown switch:", args[i]);
      else if (!libraryname)
	libraryname = args[i];
      else
	pos = i;
    }

  if (!libraryname)
    error_msg (args[0], "please specify a library file", "");
  if (pos < 2)
    usage ();

  count = 0;
  for (i = pos; i < argc; i++)
    if (args[i][0] != '-')
      {
	inf = fopen (args[i], "rb");
	count = 0;
	if (inf == NULL)
	  fprintf (stderr, "Trouble opening file: %s\n", args[i]);
	else
	  {
	    if (pbm_isapbmfile (inf))
	      {
		if (V)
		  fprintf (stderr, "%s: processing...\n", args[0]);
		if (V)
		  fprintf (stderr, "reading file %s...", args[i]);
		bitmap = pbm_readfile (inf, &cols, &rows);
		if (V && (count == 0))
		  fprintf (stderr, "building...");

		list = NULL;

		ExtractAllMarks (bitmap, &list, cols, rows);

		if (V)
		  fprintf (stderr, "writing...");

		if ((!lib) && (marklist_length (list)))
		  {
		    lib = fopen (libraryname, "r");
		    if (lib)
		      {
			fclose (lib);
			lib = fopen (libraryname, "ab");
			if (lib == NULL)
			  error_msg (args[0], "trouble appending library file", "");
			fprintf (stderr, "\nwarning: appending library file: %s\n", libraryname);
		      }
		    else
		      {
			lib = fopen (libraryname, "wb");
			if (lib == NULL)
			  error_msg (args[0], "trouble creating library file", "");
		      }
		  }		/* if !lib */

		step = list;
		while (step)
		  {
		    step->data.symnum = count;
		    write_library_mark (lib, step->data);
		    count++;
		    step = step->next;
		  }
		pbm_freearray (&bitmap, rows);
	      }
	    else
	      fprintf (stderr, "warning: %s is not a PBM file.\n", args[i]);
	    fclose (inf);
	  }
	if (count)
	  {
	    if (V)
	      fprintf (stderr, "%d marks written.\n", count);
	  }
	else
	  fprintf (stderr, "No marks written!\n");
      }

  if (lib)
    fclose (lib);
  marklist_free (&list);
  exit(0);
}

