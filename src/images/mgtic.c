/**************************************************************************
 *
 * mgtic.c -- Program to compress/decompress textual images
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
 * $Id: mgtic.c,v 1.2 1994/11/08 23:34:23 tes Exp $
 *
 **************************************************************************
 *
 * This file is the basis of mgtic, which compresses the components of an
 * image. 
 * The compressed file contains the following sections:
 *         mgtic magic_no
 *         mgtic version
 *         lossy/lossless flag 
 *         internal/external library flag 
 *         cols, rows, number of marks, library size
 *         [library sequence]
 *         symbol sequence
 *         offset sequence
 *         [residue bitmap]
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "pbmtools.h"
#include "extractor.h"
#include "utils.h"
#include "match.h"
#include "sortmarks.h"
#include "codesyms.h"
#include "arithcode.h"
#include "codeoffsets.h"
#include "bilevel.h"

static char library_template[] =
{
  ".ppppp.;"
  "pp222pp;"
  "p22222p;"
  "p22*...;"
};

void 
usage ()
{
  fprintf (stderr, "usage: \n"
	"\tmgtic -e [-L] [-Q] [-X]  libraryfile   infile    [>compressed]\n"
	   "or\n"
	   "\tmgtic -d [-L] [-X libraryfile]  compressed  [>outfile]\n");
  exit (1);
}


/* encode the list of marks 'list', w.r.t. the library 'library' */
void 
match_sequence (marklistptr list, marklistptr library, marklistptr * symbol_list, Pixel ** recon, int quick)
{
  int carryx, carryy, gap;
  int lastx = 0, lasty = 0;
  int i, j;
  marktype d, d2;
  marklistptr liststep, librarystep;
  int score, found;
  int r, c;
  int p;

  carryx = carryy = 0;
  gap = max (1, marklist_length (list) / 10);

  for (liststep = list, i = 0; liststep; liststep = liststep->next, i++)
    {
      if (V)
	if (!(i % gap))
	  fprintf (stderr, ".");
      d = liststep->data;

      found = -1;
      score = INT_MAX;
      for (librarystep = library, j = 0; librarystep; librarystep = librarystep->next, j++)
	{
	  d2 = librarystep->data;

	  if (NOT_SCREENED (d2, d))
	    if ((p = MATCH (&d, &d2)) < MATCH_VAL)
	      {
		if (p < score)
		  {
		    score = p;
		    found = j;
		    if (quick)
		      break;
		  }
	      }
	}
      /*
         if(found<0){
         for(librarystep=library,j=0; librarystep; librarystep=librarystep->next,j++){
         d2=librarystep->data;

         if((p=MATCH(&d,&d2))<MATCH_VAL){
         if(p<score){
         score=p;found=j; if(quick) break;
         }
         }
         }
         }
       */
      if (found >= 0)
	{
	  liststep->data.symnum = found;
	  marklist_getat (library, found, &d2);
	  d2.symnum = found;

	  d2.xoffset = d.xpos - lastx + carryx;		/* propagate changes to next mark */
	  d2.yoffset = d.ypos - lasty + carryy;
	  carryx = (d2.xcen - d.xcen);	/* offet centroids because thats where we match them! */
	  carryy = (d2.ycen - d.ycen);
	  d2.xoffset -= carryx;	/* make the changes to this mark */
	  d2.yoffset -= carryy;
	  lastx = d.xpos + d2.w;
	  lasty = d.ypos;

	  marklist_add (symbol_list, d2);

	  /* add to the reconstructed image--this is the lossy version */
	  for (r = 0; r < d2.h; r++)
	    for (c = 0; c < d2.w; c++)
	      if (pbm_getpixel (d2.bitmap, c, r))
		pbm_putpixel_trunc (recon, d.xpos + c - carryx, d.ypos + r - carryy, 1, d2.w, d2.h);
	}
      else
	{
	  /* didn't match at all..hopefully noise */
	  if ((liststep->data.w >= 10) || (liststep->data.w >= 10))
	    {
	      fprintf (stderr, "mgtic: warning, unable to match a char.\n");
	      write_library_mark (stderr, liststep->data);
	    }
	  if (V)
	    fprintf (stderr, "x");
	  liststep->data.symnum = -1;
	}
    }
  if (V)
    fprintf (stderr, "\n");
}




int 
main (int argc, char *args[])
{
  int i, n, r, c, ch;
  FILE *inf;
  marklistptr library = NULL, list = NULL, step = NULL;
  marklistptr symbol_list = NULL;
  marktype d, d2;
  Pixel **bitmap, **recon, **bitmapcopy;
  char *splitfilename = NULL;
  int cols, rows, count, librarysize, lossy = 1;
  int quick = 0;
  int ticencode = 0, ticdecode = 0, external = 0;
  char *libraryname = NULL, *infile = NULL;
  char *s1 = NULL, *s2 = NULL;
  char bufferin[BUFSIZ], bufferout[BUFSIZ];

  if (argc < 2)
    usage ();

  while ((ch = getopt (argc, args, "edlLQhvXR:")) != -1)
    switch (ch)
      {
      case 'e':
	ticencode = 1;
	break;
      case 'd':
	ticdecode = 1;
	break;
      case 'l':
	lossy = 1;
	break;
      case 'L':
	lossy = 0;
	break;
      case 'Q':
	quick = 1;
	break;
      case 'v':
	V = 1;
	break;
      case 'X':
	external = 1;
	break;
      case 'R':
	splitfilename = optarg;
	break;
      case 'h':
      case '?':
	usage ();
      }

  for (i = 1; i < argc; i++)
    if (args[i][0] != '-')
      if (strcmp (args[i - 1], "-R"))
	{			/* ignore arg following -R, ie. -R <filename> */
	  if (!s1)
	    s1 = args[i];
	  else if (!s2)
	    s2 = args[i];
	  else
	    error_msg (args[0], "too many filenames", "");
	}

  if (ticencode == ticdecode)
    error_msg (args[0], "please specify either encode XOR decode", "");

  if (ticencode)
    {
	  /*****************
	    ENCODING STAGE
	  *****************/
      long PrevBits = 0;
      long BITS_header = 0, BITS_symbols = 0, BITS_offsets = 0, BITS_residue = 0,
        BITS_footer = 0, BITS_library = 0;

      libraryname = s1;
      if (!libraryname)
	error_msg (args[0], "please specify a library file", "");
      infile = s2;
      if (!infile)
	error_msg (args[0], "please specify a file name", "");

      inf = fopen (infile, "rb");
      if (inf == NULL)
	error_msg (args[0], "Trouble opening file:", infile);
      setbuf (inf, bufferin);

      count = librarysize = read_library (libraryname, &library);

      if (V)
	fprintf (stderr, "%s: processing...\n", args[0]);

      if (pbm_isapbmfile (inf))
	{
	  if (V)
	    fprintf (stderr, "reading file %s...\n", infile);
	  bitmap = pbm_readfile (inf, &cols, &rows);
	  fclose (inf);

	  bitmapcopy = pbm_copy (bitmap, cols, rows);

	  if (V)
	    fprintf (stderr, "extracting...");
	  ExtractAllMarks (bitmap, &list, cols, rows);
	  if (V)
	    fprintf (stderr, "(%d marks)\n", marklist_length (list));

	  /* sort into reading order */
	  if (V)
	    fprintf (stderr, "sorting...\n");
	  list = sortmarks (list);

	  pbm_freearray (&bitmap, rows);	/* clear old version */
	  bitmap = bitmapcopy;	/* point to the copy */
	  recon = pbm_allocarray (cols, rows);

	  if (V)
	    fprintf (stderr, "matching...");

	  match_sequence (list, library, &symbol_list, recon, quick);


	  PrevBits = 0;
	  setbuf (stdout, bufferout);
	  /* start output */
	  magic_write (stdout, MAGIC_TIC);
	  InitArithEncoding ();

	  EncodeGammaDist (1);	/* version 1 of the program */
	  EncodeGammaDist (lossy);	/* lossy=1 = no residue */
	  EncodeGammaDist (external);	/* 1== external file */

	  count = marklist_length (symbol_list);
	  if (V)
	    fprintf (stderr, "encoding cols, rows, and number of symbols=%d\n", count);

	  EncodeGammaDist (cols);
	  EncodeGammaDist (rows);
	  EncodeGammaDist (count);
	  EncodeGammaDist (librarysize);

	  BITS_header = CountOfBitsOut - PrevBits;
	  PrevBits = CountOfBitsOut;


	  /* output the library sequence */
	  if (external == 0)
	    {
	      if (V)
		fprintf (stderr, "encoding library\n");

	      bl_clearmodel ();
	      bl_writetemplate (library_template);
	      for (step = library; step; step = step->next)
		bl_compress_mark (step->data);
	      bl_freemodel ();
	      BITS_library = CountOfBitsOut - PrevBits;
	      PrevBits = CountOfBitsOut;
	    }

	  /* output the symbol sequence */
	  if (V)
	    fprintf (stderr, "encoding symbol sequence\n");
	  InitPPM ();
	  EncodeSymbols (symbol_list, count);
	  BITS_symbols = CountOfBitsOut - PrevBits;
	  PrevBits = CountOfBitsOut;


	  /* output the offset sequence */
	  if (V)
	    fprintf (stderr, "encoding offset sequence\n");
	  EncodeOffsets (symbol_list, count);
	  BITS_offsets = CountOfBitsOut - PrevBits;
	  PrevBits = CountOfBitsOut;

	  EncodeChecksum ();	/* code lossy checksum */

	  /* calculate the residue...and compress it---if need be! */
	  if (!lossy)
	    {
	      if (V)
		fprintf (stderr, "encoding residue...\n");

	      d.bitmap = bitmap;
	      d.h = rows;
	      d.w = cols;
	      d2.bitmap = recon;
	      d2.h = rows;
	      d2.w = cols;

	      if (splitfilename)
		{
		  FILE *temp;

		  CloseDownArithEncoding ();
		  fclose (stdout);
		  /* no residue result */
		  BITS_footer = CountOfBitsOut - PrevBits;

		  temp = fopen (splitfilename, "wb");
		  if (temp == NULL)
		    error_msg (args[0], "Trouble creating file:", splitfilename);

		  arith_out = temp;
		  InitArithEncoding ();
		  bl_clair_compress (d, d2);
		  CloseDownArithEncoding ();
		  BITS_residue = CountOfBitsOut;
		  fclose (temp);
		}
	      else
		{
		  bl_clair_compress (d, d2);
		  BITS_residue = CountOfBitsOut - PrevBits;
		  PrevBits = CountOfBitsOut;
		  EncodeChecksum ();	/* code lossless checksum */
		  CloseDownArithEncoding ();
		  BITS_footer = CountOfBitsOut - PrevBits;
		}
	    }
	  else
	    {
	      if (V)
		fprintf (stderr, "not encoding residue..lossy mode\n");
	      CloseDownArithEncoding ();
	      BITS_footer = CountOfBitsOut - PrevBits;
	    }

	  /* because we edit the values above */
	  CountOfBitsOut = BITS_header + BITS_library + BITS_symbols + BITS_offsets + BITS_residue + BITS_footer;

	  fprintf (stderr, "bits: header=%ld, library=%ld, "
		   "symbols=%ld, offsets=%ld, residue=%ld, footer=%ld\n",
		   BITS_header, BITS_library, BITS_symbols,
		   BITS_offsets, BITS_residue, BITS_footer);
	  fprintf (stderr, "total bits: %ld, ", CountOfBitsOut);
	  fprintf (stderr, "Lossy CR: %4.2f", (cols * rows) / (float) (CountOfBitsOut - BITS_residue));
	  if (external)
	    fprintf (stderr, " (excluding external lib)");
	  fprintf (stderr, ", Lossless CR: %4.2f\n", (!lossy) * (cols * rows) / (float) (CountOfBitsOut));
	}
      else
	error_msg (args[0], "unknown format of bitmap--expecting PBM.", "");
    }
  else
    {
	  /*****************
	    DECODING STAGE
	  *****************/
      int lastx, lasty;
      librarysize = 0;

      if (external)
	{
	  libraryname = s1;
	  infile = s2;
	  count = librarysize = read_library (libraryname, &library);
	}
      else
	{
	  infile = s1;
	  if (infile && s2)
	    error_msg (args[0], "too many filenames", "");
	}

      if (!freopen (infile, "rb", stdin))
	error_msg (args[0], "Trouble opening file:", infile);


      if (V)
	fprintf (stderr, "decompressing...\n");

      setbuf (stdin, bufferin);
      magic_check (stdin, MAGIC_TIC);

      InitArithDecoding ();

      {
	int version = DecodeGammaDist ();
	if (version != 1)
	  error_msg (args[0], "Need later version of decompressor.", "");
      }

      {
	int templossy = DecodeGammaDist ();
	if (!lossy)
	  lossy = templossy;	/* can only choose if encoded file is lossless */
	if (V)
	  {
	    if (lossy)
	      fprintf (stderr, "lossy mode\n");
	    else
	      fprintf (stderr, "lossless mode\n");
	  }
      }

      if (DecodeGammaDist ())
	{			/* if compressed file doesn't contain library */
	  if (!external)
	    error_msg (args[0], "compressed file doesn't contain library, specify externally", "");
	  external = 1;
	}
      else
	{			/* if compressed file contains library */
	  if (external)
	    fprintf (stderr, "ignoring external library file\n");
	  external = 0;
	}

      cols = DecodeGammaDist ();
      rows = DecodeGammaDist ();
      count = DecodeGammaDist ();

      i = DecodeGammaDist ();	/* librarysize */
      if (external)
	{
	  if (i > librarysize)
	    error_msg (args[0], "external library file is too small!", "");
	  else if (i < librarysize)
	    fprintf (stderr, "%s: warning, expecting a different (smaller) library.\n", args[0]);
	}
      librarysize = i;

      if (V)
	fprintf (stderr, "cols %d, rows %d, num syms %d, library size %d\n", cols, rows, count, librarysize);

      /* decode library */
      if (external == 0)
	{
	  if (V)
	    fprintf (stderr, "reading library\n");
	  bl_clearmodel ();
	  bl_readtemplate ();
	  for (n = 0; n < librarysize; n++)
	    {
	      bl_decompress_mark (&d);
	      d.symnum = n;
	      if (library == NULL)
		step = marklist_add (&library, d);
	      else
		step = marklist_add (&step, d);
	    }
	  bl_freemodel ();
	  if (V)
	    fprintf (stderr, "read %d marks from library\n", marklist_length (library));
	}

      recon = pbm_allocarray (cols, rows);

      /* decode symbols */
      InitPPM ();
      if (V)
	fprintf (stderr, "decompressing %d symbols\n", count);
      symbol_list = DecodeSymbols (count);

      /* decode offsets */
      if (V)
	fprintf (stderr, "reading offsets...\n");
      DecodeOffsets (symbol_list, count);
      lastx = lasty = 0;
      for (step = symbol_list; step; step = step->next)
	{
	  lastx = lastx + step->data.xoffset;
	  lasty = lasty + step->data.yoffset;

	  marklist_getat (library, step->data.symnum, &d2);
	  for (r = 0; r < d2.h; r++)
	    for (c = 0; c < d2.w; c++)
	      if (pbm_getpixel (d2.bitmap, c, r))
		pbm_putpixel_trunc (recon, lastx + c, lasty + r, 1, cols, rows);	/* we don't care, already warned them! */
	  lastx += d2.w;
	}

      DecodeChecksum (args[0]);

      /* decode the residue */
      if (!lossy)
	{
	  if (V)
	    fprintf (stderr, "decoding residue...\n");

	  bitmap = pbm_allocarray (cols, rows);
	  d.bitmap = bitmap;
	  d.w = d2.w = cols;
	  d.h = d2.h = rows;

	  d2.bitmap = recon;

	  /* NOTE: the 2nd argument is clairvoyantly compressed */
	  if (splitfilename)
	    {
	      FILE *temp;

	      CloseDownArithDecoding ();
	      fclose (stdin);

	      temp = fopen (splitfilename, "rb");
	      if (temp == NULL)
		error_msg (args[0], "Trouble opening file:", splitfilename);

	      arith_in = temp;
	      InitArithDecoding ();
	      bl_clair_decompress (d, d2);
	      CloseDownArithDecoding ();
	      fclose (temp);
	    }
	  else
	    {
	      bl_clair_decompress (d, d2);
	      DecodeChecksum (args[0]);
	      CloseDownArithDecoding ();
	    }
	  pbm_freearray (&recon, rows);
	  recon = bitmap;	/* point to the bitmap */
	}
      else
	{
	  CloseDownArithDecoding ();
	}

      if (V)
	fprintf (stderr, "writing pbm file...\n");
      setbuf (stdout, bufferout);
      pbm_writefile (stdout, recon, cols, rows);
      pbm_freearray (&recon, rows);
    }				/* end decoding */
  exit(0);
}

