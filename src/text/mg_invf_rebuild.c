/**************************************************************************
 *
 * mg_invf_rebuild.c -- Program to rebuild an inverted file with skipping
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
 * $Id: mg_invf_rebuild.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "timing.h"
#include "bitio_m.h"
#include "bitio_m_stdio.h"
#include "bitio_gen.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg_files.h"
#include "invf.h"
#include "locallib.h"
#include "words.h"
#include "mg.h"

#define LEN 40
#define WRD 1

typedef struct invf_info
  {
    unsigned long doc_num, count, bits_so_far, count_bits;
  }
invf_info;

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:11  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:46  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:10  rjmcnab
   *** empty log message ***

   * Revision 1.4  1994/11/29  00:32:03  tes
   * Committing the new merged files and changes.
   *
   * Revision 1.3  1994/10/20  03:56:56  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:51  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_invf_rebuild.c 16583 2008-07-29 10:20:36Z davidb $";

static char pathname[256];

static void process_files (char *filename);

static int k = -1;
static int mode = 0;
static int max_nodes = -1;
static int mins = -1;

void 
usage (char *pgname)
{
  fprintf (stderr, "usage: %s [-f input_file]"
       "[-d data directory] -0|-1|-2 [-k num] [-m num] [-s num]\n", pgname);
  printf ("Mode 0: (the default)\n");
  printf ("\tk, m, and s have no meaning and if specified produce an error\n");
  printf ("Mode 1:\n");
  printf ("\tm, and s have no meaning and if specified produce an error\n");
  printf ("\tk The size of skips\n");
  printf ("Mode 2:\n");
  printf ("\tk has no meaning and if specified will produce an error\n");
  printf ("\tm is the the number of accumulators that will be used for the\n");
  printf ("\t  ranking. The program builds an inverted file that is \n");
  printf ("\t  \"optimal\" for that number of accumulators.\n");
  printf ("\ts is the minimum size for skips.\n");
  exit (1);
}



int main (int argc, char **argv)
{
  ProgTime start;
  char *dir_name, *file_name = "";
  int ch;
  msg_prefix = argv[0];
  dir_name = getenv ("MGDATA");
  strcpy (pathname, dir_name ? dir_name : ".");
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "012hf:d:k:b:s:m:")) != -1)
    switch (ch)
      {
      case '0':
	mode = 0;
	break;
      case '1':
	mode = 1;
	break;
      case '2':
	mode = 2;
	break;
      case 'f':		/* input file */
	file_name = optarg;
	break;
      case 'd':
	strcpy (pathname, optarg);
	break;
      case 'k':
	k = atoi (optarg);
	break;
      case 'm':
	max_nodes = atoi (optarg);
	break;
      case 's':
	mins = atoi (optarg);
	if (mins < 1)
	  FatalError (1, "The number for the -m option must be greater"
		      " than or equal to 1");
	break;
      case 'h':
      case '?':
	usage (argv[0]);
      }

  if ((mode == 0 && (k != -1 || max_nodes != -1 || mins != -1)) ||
      (mode == 1 && (max_nodes != -1 || mins != -1)) ||
      (mode == 2 && (k != -1)))
    {
      Message ("Illegal parameters for mode");
      usage (argv[0]);
    }

  if (mode == 1 && k == -1)
    {
      k = 8;
      Message ("k is required for mode 1: defaulting k to %d", k);
    }
  if (mode == 2 && max_nodes == -1)
    {
      max_nodes = 1024;
      Message ("m is required for mode 2: defaulting m to %d", max_nodes);
    }
  if (mode == 2 && mins == -1)
    {
      mins = 1;
      Message ("s is required for mode 2: defaulting s to %d", mins);
    }


  GetTime (&start);
  process_files (file_name);
  Message ("%s\n", ElapsedTime (&start, NULL));
  Message ("**** Don\'t forget to rebuild the stemmed dictionary with mg_invf_dict. ****\n");
  Message ("**** If the collection was built with stem indexes don\'t forget to     ****\n");
  Message ("**** rebuild them with mg_stem_idx.                                     ****\n");

  return 0;
}





static void 
process_files (char *filename)
{
  FILE *in, *out, *idx, *odx, *dict;
  unsigned long magic, outmode, N, in_k, out_k;
  unsigned long bits_out, bytes_out, i, j;
  stdio_bitio_state out_buf, in_buf;
  struct invf_dict_header idh;
  struct invf_file_header ifh_in, ifh_out;
  char FName[256];

  outmode = mode;

  /* open .invf.ORG, rename .invf if have to */
  sprintf (FName, FILE_NAME_FORMAT ".ORG", pathname, filename, INVF_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
  if (!(in = fopen (FName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
    {
      char fname[256];
      sprintf (fname, FILE_NAME_FORMAT, pathname, filename, INVF_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
      rename (fname, FName);
      if (!(in = fopen (FName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
	FatalError (1, "Unable to open \"%s\".\n", FName);
    }
  else
    {
      char fname[256];
      sprintf (fname, FILE_NAME_FORMAT, pathname, filename, INVF_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
      unlink (fname);
    }
  Message ("Opening \"%s\"\n", FName);

  /* check the magic number for .invf.ORG */
  if (fread (&magic, sizeof (magic), 1, in) != 1 || NTOHUL(magic) != MAGIC_INVF)  /* [RPAP - Jan 97: Endian Ordering] */
    FatalError (1, "Bad magic number in \"%s\".\n", FName);


  /* open .invf.idx.ORG, rename .invf.idx if have to */
  sprintf (FName, FILE_NAME_FORMAT ".ORG", pathname, filename, INVF_IDX_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
  if (!(idx = fopen (FName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
    {
      char fname[256];
      sprintf (fname, FILE_NAME_FORMAT, pathname, filename, INVF_IDX_SUFFIX);
      rename (fname, FName);
      if (!(idx = fopen (FName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
	FatalError (1, "Unable to open \"%s\".\n", FName);
    }
  else
    {
      char fname[256];
      sprintf (fname, FILE_NAME_FORMAT, pathname, filename, INVF_IDX_SUFFIX);
      unlink (fname);
    }
  Message ("Opening \"%s\"\n", FName);

  /* check the magic number for .invf.idx.ORG */
  if (fread (&magic, sizeof (magic), 1, idx) != 1 || NTOHUL(magic) != MAGIC_INVI)  /* [RPAP - Jan 97: Endian Ordering] */
    FatalError (1, "Bad magic number in \"%s\".\n", FName);

  sprintf (FName, FILE_NAME_FORMAT, pathname, filename, INVF_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
  Message ("Creating \"%s\"\n", FName);
  if (!(out = fopen (FName, "wb")))  /* [RPAP - Feb 97: WIN32 Port] */
    FatalError (1, "Unable to open \"%s\".\n", FName);

  sprintf (FName, FILE_NAME_FORMAT, pathname, filename, INVF_IDX_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
  Message ("Creating \"%s\"\n", FName);
  if (!(odx = fopen (FName, "wb")))  /* [RPAP - Feb 97: WIN32 Port] */
    FatalError (1, "Unable to open \"%s\".\n", FName);

  sprintf (FName, FILE_NAME_FORMAT, pathname, filename, INVF_DICT_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
  Message ("Opening \"%s\"\n", FName);
  if (!(dict = fopen (FName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
    FatalError (1, "Unable to open \"%s\".\n", FName);

  if (fread (&magic, sizeof (magic), 1, dict) != 1 || NTOHUL(magic) != MAGIC_STEM_BUILD)  /* [RPAP - Jan 97: Endian Ordering] */
    FatalError (1, "Bad magic number in \"%s\".\n", FName);

  fread ((char *) &idh, sizeof (idh), 1, dict);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(idh.lookback);
  NTOHUL(idh.dict_size);
  NTOHUL(idh.total_bytes);
  NTOHUL(idh.index_string_bytes);
  NTOHD(idh.input_bytes); /* [RJM 07/97: 4G limit] */
  NTOHUL(idh.num_of_docs);
  NTOHUL(idh.static_num_of_docs);
  NTOHUL(idh.num_of_words);
  NTOHUL(idh.stemmer_num);
  NTOHUL(idh.stem_method);

  HTONUL2(MAGIC_INVF, magic);  /* [RPAP - Jan 97: Endian Ordering] */
  fwrite ((char *) &magic, sizeof (magic), 1, out);

  fread ((char *) &ifh_in, sizeof (ifh_in), 1, in);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(ifh_in.no_of_words);
  NTOHUL(ifh_in.no_of_ptrs);
  NTOHUL(ifh_in.skip_mode);
  for (i = 0; i < 16; i++)
    NTOHUL(ifh_in.params[i]);
  NTOHUL(ifh_in.InvfLevel);

  ifh_out = ifh_in;
  ifh_out.skip_mode = outmode;
  bzero ((char *) ifh_out.params, sizeof (ifh_out.params));
  switch (outmode)
    {
    case 0:
      break;
    case 1:
      ifh_out.params[0] = k;
      break;
    case 2:
      ifh_out.params[0] = max_nodes;
      ifh_out.params[1] = mins;
      break;
    }

  /* [RPAP - Jan 97: Endian Ordering] */
  HTONUL(ifh_out.no_of_words);
  HTONUL(ifh_out.no_of_ptrs);
  HTONUL(ifh_out.skip_mode);
  for (i = 0; i < 16; i++)
    HTONUL(ifh_out.params[i]);
  HTONUL(ifh_out.InvfLevel);

  fwrite ((char *) &ifh_out, sizeof (ifh_out), 1, out);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(ifh_out.no_of_words);
  NTOHUL(ifh_out.no_of_ptrs);
  NTOHUL(ifh_out.skip_mode);
  for (i = 0; i < 16; i++)
    NTOHUL(ifh_out.params[i]);
  NTOHUL(ifh_out.InvfLevel);

  Message ("The file is a level %d inverted file.\n", ifh_in.InvfLevel);

  bits_out = ftell (out) * 8;


  HTONUL2(MAGIC_INVI, magic);  /* [RPAP - Jan 97: Endian Ordering] */
  fwrite ((char *) &magic, sizeof (magic), 1, odx);

  DECODE_START (in)
    DECODE_PAUSE (in_buf)

    ENCODE_START (out)
    ENCODE_PAUSE (out_buf)

    N = idh.num_of_docs;

  for (i = 0; i < ifh_in.no_of_words; i++)
    {
      unsigned long blk, p;
      unsigned long odn_blk = 0, olen_blk = 0;
      unsigned long idn_blk = 0, ilen_blk = 0;
      register unsigned long suff;
      unsigned long fcnt, wcnt, doc_num, bits_so_far, last_major;
      unsigned long next_mjr_dn, kd;
      char dummy2[MAXSTEMLEN + 1];
      invf_info *ii;

      fgetc (dict);
      suff = fgetc (dict);
      fread (dummy2, sizeof (u_char), suff, dict);
      fread ((char *) &fcnt, sizeof (fcnt), 1, dict);
      fread ((char *) &wcnt, sizeof (wcnt), 1, dict);

      /* [RPAP - Jan 97: Endian Ordering] */
      NTOHUL(fcnt);
      NTOHUL(wcnt);

      HTONUL2(bits_out >> 3, bytes_out);  /* [RPAP - Jan 97: Endian Ordering] */
      fwrite ((char *) &bytes_out, sizeof (bytes_out), 1, odx);
      NTOHUL(bytes_out);  /* [RPAP - Jan 97: Endian Ordering] */

      p = fcnt;
      blk = BIO_Bblock_Init (idh.static_num_of_docs, p);
      switch (outmode)
	{
	case 1:
	  {
	    unsigned long len;
	    if (p <= ifh_out.params[0])
	      out_k = 0;
	    else
	      {
		out_k = ifh_out.params[0];
		len = BIO_Bblock_Bound (N, p);
		if (ifh_in.InvfLevel >= 2)
		  len += wcnt;
		odn_blk = BIO_Bblock_Init (idh.num_of_docs, (p + out_k - 1) / out_k);
		olen_blk = BIO_Bblock_Init (len, (p + out_k - 1) / out_k);
	      }
	    break;
	  }
	case 2:
	  {
	    unsigned long len;
	    if (p <= mins)
	      out_k = 0;
	    else
	      {
		out_k = (int) (2 * sqrt ((double) p / max_nodes));
		if (out_k <= mins)
		  out_k = mins;
		len = BIO_Bblock_Bound (N, p);
		if (ifh_in.InvfLevel >= 2)
		  len += wcnt;
		odn_blk = BIO_Bblock_Init (idh.num_of_docs,
					   (p + out_k - 1) / out_k);
		olen_blk = BIO_Bblock_Init (len, (p + out_k - 1) / out_k);
	      }
	    break;
	  }
	default:
	  out_k = 0;
	}

      switch (ifh_in.skip_mode)
	{
	case 1:
	  {
	    unsigned long len;
	    if (p <= ifh_in.params[0])
	      in_k = 0;
	    else
	      {
		in_k = ifh_in.params[0];
		len = BIO_Bblock_Bound (N, p);
		if (ifh_in.InvfLevel >= 2)
		  len += wcnt;
		idn_blk = BIO_Bblock_Init (idh.num_of_docs, (p + in_k - 1) / in_k);
		ilen_blk = BIO_Bblock_Init (len, (p + in_k - 1) / in_k);
	      }
	    break;
	  }
	case 2:
	  {
	    unsigned long len;
	    if (p <= ifh_in.params[1])
	      {
		in_k = 0;
	      }
	    else
	      {
		in_k = (int) (2 * sqrt ((double) p / ifh_in.params[0]));
		if (in_k <= ifh_in.params[1])
		  in_k = ifh_in.params[1];
		len = BIO_Bblock_Bound (N, p);
		if (ifh_in.InvfLevel >= 2)
		  len += wcnt;
		idn_blk = BIO_Bblock_Init (idh.num_of_docs,
					   (p + in_k - 1) / in_k);
		ilen_blk = BIO_Bblock_Init (len, (p + in_k - 1) / in_k);
	      }
	    break;
	  }
	default:
	  in_k = 0;
	}

      if (!(ii = Xmalloc (sizeof (invf_info) * p)))
	FatalError (1, "Unable to allocate memory for \"ii\"\n");

      doc_num = bits_so_far = 0;
      next_mjr_dn = 0;
      kd = 0;
      DECODE_CONTINUE (in_buf)
	for (j = 0; j < p; j++, kd++)
	{
	  unsigned long doc_diff, count = 0;
	  if (kd == in_k)
	    kd = 0;
	  if (in_k && kd == 0 && j + in_k < p)
	    {
	      int temp;
	      BBLOCK_DECODE (next_mjr_dn, idn_blk);
	      next_mjr_dn += doc_num;
	      BBLOCK_DECODE (temp, ilen_blk);
	    }
	  ii[j].bits_so_far = bits_so_far;
	  if (in_k && kd == in_k - 1 && j != p - 1)
	    {
	      int count;
	      BBLOCK_LENGTH (next_mjr_dn - doc_num, blk, count);
	      bits_so_far += count;
	      doc_num = next_mjr_dn;
	    }
	  else
	    {
	      BBLOCK_DECODE_L (doc_diff, blk, bits_so_far);
	      doc_num += doc_diff;
	    }
	  ii[j].doc_num = doc_num;
	  if (ifh_in.InvfLevel >= 2)
	    {
	      int count_bits = 0;
	      GAMMA_DECODE_L (count, count_bits);
	      ii[j].count_bits = count_bits;
	      bits_so_far += count_bits;
	      ii[j].count = count;
	    }
	}

      /* read till a byte boundary */
      while (__btg)
	{
	  DECODE_BIT;
	  bits_so_far++;
	}

      DECODE_PAUSE (in_buf)

	doc_num = bits_so_far = 0;
      last_major = 0;
      kd = 0;
      ENCODE_CONTINUE (out_buf)
	for (j = 0; j < p; j++, kd++)
	{
	  if (kd == out_k)
	    kd = 0;
	  if (out_k && kd == 0)
	    {
	      if (j + out_k < p)
		{
		  int num = ii[j + out_k - 1].doc_num - last_major;
		  BBLOCK_ENCODE_L (num, odn_blk, bits_out);
		  last_major = ii[j + out_k - 1].doc_num;

		  num = ii[j + out_k - 1].bits_so_far + ii[j + out_k - 1].count_bits -
		    bits_so_far;
		  BBLOCK_ENCODE_L (num, olen_blk, bits_out);
		  bits_so_far = ii[j + out_k].bits_so_far;
		}
	    }
	  if (!(out_k && kd == out_k - 1 && j != p - 1))
	    BBLOCK_ENCODE_L (ii[j].doc_num - doc_num, blk, bits_out);
	  doc_num = ii[j].doc_num;
	  if (ifh_in.InvfLevel >= 2)
	    GAMMA_ENCODE_L (ii[j].count, bits_out);
	}

      /* write till a byte boundary */
      while (__btg != 8)
	{
	  ENCODE_BIT (0);
	  bits_out++;
	}
      ENCODE_PAUSE (out_buf)

	Xfree (ii);

    }
  ENCODE_CONTINUE (out_buf)
    ENCODE_DONE

    HTONUL2(bits_out >> 3, bytes_out);  /* [RPAP - Jan 97: Endian Ordering] */
  fwrite ((char *) &bytes_out, sizeof (bytes_out), 1, odx);
  NTOHUL(bytes_out);

  fclose (idx);
  fclose (odx);
  fclose (in);
  fclose (out);
  fclose (dict);

}
