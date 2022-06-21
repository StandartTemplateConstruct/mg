/**************************************************************************
 *
 * mgfelics.c -- Program to compress/decompress grey scale images
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
 * $Id: mgfelics.c,v 1.1.1.1 1994/08/11 03:26:12 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "utils.h"


#define MAXKVALUE 32



void encode (unsigned int *line, int width, int height, int maxval, int maxk,
	     FILE * fp_in, FILE * fp_out, int (*get) (FILE *));

void decode (unsigned int *line, int width, int height, int maxval, int maxk,
	     FILE * fp_in, FILE * fp_out);


void 
usage (char *prog)
{
  fprintf (stderr, "usage:\n");
  fprintf (stderr, "\t%s -e [-k n]   infile    [>compressed]\n"
	   "or\n"
	   "\t%s -d  compressed-infile   [>outfile]\n", prog, prog);
  exit (1);
}




int 
main (int argc, char **argv)
{
  FILE *fp_in = (stdin), *fp_out = (stdout);
  int width, height, maxval;
  u_long magic;
  char *filename = NULL;
  unsigned int *line;
  int maxk = 0;
  int i;
  int encode_flag = 0, decode_flag = 0;

  if (argc < 2)
    usage (argv[0]);

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-h"))
	usage (argv[0]);
      else if (!strcmp (argv[i], "-e"))
	encode_flag = 1;
      else if (!strcmp (argv[i], "-d"))
	decode_flag = 1;
      else if (!strcmp (argv[i], "-k"))
	{
	  maxk = atoi (argv[++i]);
	}
      else if (argv[i][0] == '-')
	error_msg (argv[0], "unknown switch:", argv[i]);
      else if (!filename)
	filename = argv[i];
      else
	error_msg (argv[0], "too many filenames specified.", "");
    }

  if (encode_flag == decode_flag)
    error_msg (argv[0], "please specify either encode XOR decode.", "");

  if (!filename)
    error_msg (argv[0], "please specify a filename.", "");

  if ((encode_flag) && (maxk >= MAXKVALUE))
    {				/* to fit into 32 bit integers */
      fprintf (stderr, "%s: k value of %d is too large; it must be less than %d\n",
	       argv[0], maxk, MAXKVALUE);
      exit (1);
    }


  if ((fp_in = fopen (filename, "rb")) == NULL)
    error_msg (argv[0], "unable to open file:", filename);

  /* read file header to determine the type and dimensions */


  magic = magic_read (fp_in);
  if (magic != MAGIC_FELICS)
    {
      rewind (fp_in);
      magic = getmagicno_short_pop (fp_in);
    }

  if ((decode_flag && (magic != MAGIC_FELICS)) ||
      (encode_flag && ((magic != MAGIC_P2) && (magic != MAGIC_P5))))
    {
      fclose (fp_in);
      error_msg (argv[0], encode_flag ? "only PGM files can be used." : "only felics compressed files can be used.", "");
    }

  width = gethint (fp_in);
  height = gethint (fp_in);
  maxval = gethint (fp_in);

  if ((line = (unsigned int *) malloc (width * sizeof (int *))) == NULL)
    {
      fclose (fp_in);
      error_msg (argv[0], "unable to allocate memory,", "");
    }


  if (encode_flag)
    {
      magic_write (fp_out, MAGIC_FELICS);
      fprintf (fp_out, "%d\n%d\n%d\n%d\n", width, height, maxval, maxk);
      encode (line, width, height, maxval, maxk, fp_in, fp_out,
	      magic == MAGIC_P2 ? getint : fgetc);
    }
  else if (decode_flag)
    {
      if (maxk)
	warn (argv[0], "k value not required for decompression, ignored.", "");

      maxk = gethint (fp_in);
      fprintf (fp_out, "P%d\n%d %d\n%d\n", maxval < 256 ? 5 : 2,
	       width, height, maxval);
      decode (line, width, height, maxval, maxk, fp_in, fp_out);
    }


  fclose (fp_in);
  fclose (fp_out);

  return 0;
}
