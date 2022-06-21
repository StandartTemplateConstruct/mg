/**************************************************************************
 *
 * mgticstat.c -- Program to print out stats about a textual image library
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
 * $Id: mgticstat.c,v 1.1.1.1 1994/08/11 03:26:12 tes Exp $
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
	   "\tmgticstat   libraryfile\n");
  exit (1);
}

int 
main (int argc, char *args[])
{
  FILE *lib, *outf = (stdout);
  int count;
  int small1, small2, small4, small9, small16, small25, small36, small49,
    small64, small81, small100;
  marktype d;
  int avx, avy;
  int hhh, www, i;
  int avruns_h = 0, avruns_v = 0;
  int av_area = 0;
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
	error_msg (args[0], "too many filenames", "");
    }

  if (!libraryname)
    error_msg (args[0], "please specify a library file", "");


  lib = fopen (libraryname, "r");
  if (lib == NULL)
    error_msg (args[0], "Trouble opening library file", "");

  count = 0;
  small1 = small2 = small4 = small9 = small16 = small25 = small36 = small49 = small64 = small81 = small100 = 0;
  avx = avy = 0;
  www = hhh = 0;
  while (!isEOF (lib))
    {
      if (!read_library_mark (lib, &d))
	error_msg (args[0], "error in library file", "");

      av_area += d.set;
      if (d.set <= 1)
	small1++;
      if (d.set <= 2)
	small2++;
      if (d.set <= 4)
	small4++;
      if (d.set <= 9)
	small9++;
      if (d.set <= 16)
	small16++;
      if (d.set <= 25)
	small25++;
      if (d.set <= 36)
	small36++;
      if (d.set <= 49)
	small49++;
      if (d.set <= 64)
	small64++;
      if (d.set <= 81)
	small81++;
      if (d.set <= 100)
	small100++;
      avx += d.xcen;
      avy += d.ycen;
      www += d.w;
      hhh += d.h;
      avruns_h += d.h_runs;
      avruns_v += d.v_runs;
      pbm_freearray (&d.bitmap, d.h);
      count++;
      if (count % 5000 == 0)
	fprintf (outf, "%dk ", count / 1000);
    }
  fprintf (outf, "number of marks=%d\n", count);
  fprintf (outf, "average area=%d\n", av_area / count);
  fprintf (outf, "area <=1:%d, <=2:%d, <=4:%d, <=9:%d, <=16:%d, <=25:%d, <=36:%d, <=49:%d, <=64:%d, <=81:%d, <=100:%d\n", small1, small2, small4, small9, small16, small25, small36, small49, small64, small81, small100);

  fprintf (outf, "average num runs, horizontal=%d, vertical=%d\n", avruns_h / count, avruns_v / count);
  fprintf (outf, "average centroid position=(%d,%d)\n", avx / count, avy / count);
  fprintf (outf, "average width=%d, average height=%d\n", www / count, hhh / count);
  fclose (lib);
  exit(0);
}

