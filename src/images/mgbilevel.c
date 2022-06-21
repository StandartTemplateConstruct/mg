/**************************************************************************
 *
 * mgbilevel.c -- Program to compress/decompress bilevel images
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
 * $Id: mgbilevel.c,v 1.1.1.1 1994/08/11 03:26:12 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "utils.h"
#include "pbmtools.h"
#include "arithcode.h"
#include "bilevel.h"



static char default_template[] =
{
  ".ppppp.;"
  "pp222pp;"
  "p22222p;"
  "p22*...;"
};

void 
Encode_bitmap (marktype b, char template[])
{
  InitArithEncoding ();

  bl_compress (b, template);

  CloseDownArithEncoding ();
}

void 
Decode_bitmap (marktype * b)
{
  InitArithDecoding ();

  bl_decompress (b);

  CloseDownArithDecoding ();
}


void 
usage ()
{
  fprintf (stderr,
	   "usage:\n"
	   "       mgbilevel -e  [-t \"string\"]  infile      >compressed\n"
	   "or\n"
	   "       mgbilevel -d                 compressed  >outfile\n");
  exit (1);
}

#define BUF_SIZE 65536

int 
main (int argc, char *args[])
{
  marktype d;
  char *filename = NULL, *template = NULL;
  Pixel **bitmap;
  int encode = 0, decode = 0, i;
  char buffer[BUF_SIZE];

  if (argc < 2)
    usage ();
  template = default_template;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (args[i], "-e") == 0)
	encode = 1;
      else if (strcmp (args[i], "-d") == 0)
	decode = 1;
      else if (strcmp (args[i], "-h") == 0)
	usage ();
      else if (strcmp (args[i], "-t") == 0)
	{
	  template = args[i + 1];
	  i++;
	}
      else if (args[i][0] == '-')
	error_msg (args[0], "unknown switch:", args[i]);
      else if (!filename)
	filename = args[i];
      else
	error_msg (args[0], "only specify 1 filename", "");
    }
  if (encode == decode)
    {
      fprintf (stderr, "You must specify either encode XOR decode.\n");
      exit (1);
    }

  if (!filename)
    {
      fprintf (stderr, "You must specify a filename.\n");
      exit (1);
    }

  if (encode)
    {
      bitmap = pbm_readnamedfile (filename, &d.w, &d.h);
      if (bitmap == NULL)
	error_msg (args[0], "problem reading bitmap", "");

      arith_out = stdout;
      setbuffer (stdout, buffer, BUF_SIZE);
      magic_write (stdout, MAGIC_BILEVEL);
      if (bitmap)
	{
	  d.bitmap = bitmap;
	  Encode_bitmap (d, template);
	}
    }
  else if (decode)
    {
      FILE *tpin;
      tpin = fopen (filename, "rb");
      if (!tpin)
	error_msg ("mgbilevel", "Can't open file:", filename);
      arith_in = tpin;
      setbuffer (tpin, buffer, BUF_SIZE);
      magic_check (tpin, MAGIC_BILEVEL);
      Decode_bitmap (&d);
      fclose (tpin);

      setbuffer (stdout, buffer, BUF_SIZE);
      pbm_writefile (stdout, d.bitmap, d.w, d.h);
    }
  exit(0);
}

