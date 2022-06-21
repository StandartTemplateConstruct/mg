/**************************************************************************
 *
 * mg_invf_dump.c -- Program to dump uot an inverted fil
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
 * $Id: mg_invf_dump.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "messages.h"
#include "timing.h"
#include "bitio_m.h"
#include "bitio_m_stdio.h"
#include "bitio_gen.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg_files.h"
#include "locallib.h"
#include "words.h"
#include "invf.h"

extern unsigned long S_btg;

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.2  2001/06/12 01:41:39  kjm18
   changed the help message

   Revision 1.1  1999/08/10 21:18:09  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:46  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:05  rjmcnab
   *** empty log message ***

   * Revision 1.3  1994/11/29  00:32:01  tes
   * Committing the new merged files and changes.
   *
   * Revision 1.2  1994/09/20  04:41:50  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_invf_dump.c 16583 2008-07-29 10:20:36Z davidb $";



static void process_files (char *filename);

int binary = 0;
int word_counts = 0;
int term_dump = 0;


int main (int argc, char **argv)
{
  ProgTime start;
  char *dir_name, *file_name = "";
  int ch;
  msg_prefix = argv[0];
  dir_name = getenv ("MGDATA");
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "hbwtf:d:")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	file_name = optarg;
	break;
      case 'd':
        set_basepath(optarg);
	break;
      case 'b':
	binary = 1;
	break;
      case 'w':
	word_counts = 1;
	break;
      case 't':
	term_dump = 1;
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-h] [-b] [-w] [-t] [-f input_file]"
		 "[-d data directory]\n"
		 "(b - binary mode, w - wordcount, t - term dump)\n", argv[0]);
	exit (1);
      }
  GetTime (&start);
  process_files (file_name);
  Message ("%s\n", ElapsedTime (&start, NULL));
  return 0;
}

static void 
process_files (char *name)
{
  unsigned long N, Nstatic, k;
  FILE *dict;
  FILE *invf;
  struct invf_dict_header idh;
  struct invf_file_header ifh;
  int i;  /* [RPAP - Jan 97: Endian Ordering] */

  dict = open_file (name, INVF_DICT_SUFFIX, "rb", MAGIC_STEM_BUILD, MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

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

  if (!(invf = open_file (name, INVF_SUFFIX ".ORG", "rb", MAGIC_INVF, MG_CONTINUE)))  /* [RPAP - Feb 97: WIN32 Port] */
    invf = open_file (name, INVF_SUFFIX, "rb", MAGIC_INVF, MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  fread ((char *) &ifh, sizeof (ifh), 1, invf);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(ifh.no_of_words);
  NTOHUL(ifh.no_of_ptrs);
  NTOHUL(ifh.skip_mode);
  for (i = 0; i < 16; i++)
    NTOHUL(ifh.params[i]);
  NTOHUL(ifh.InvfLevel);

  if (ifh.skip_mode != 0)
    FatalError (1, "The invf file contains skips. Unable to dump.");

  DECODE_START (invf)
    N = idh.num_of_docs;
  Nstatic = idh.static_num_of_docs;
  if (binary)
    {
      fwrite ((char *) &N, sizeof (N), 1, stdout);
      fwrite ((char *) &ifh.no_of_words, sizeof (ifh.no_of_words), 1, stdout);
    }
  else
    printf ("%ld %ld\n", N, ifh.no_of_words);
  for (k = 0; k < ifh.no_of_words; k++)
    {
      int i, blk, doc;
      register unsigned long suff, prefix;
      unsigned long fcnt, wcnt;
      char term[MAXSTEMLEN + 1];

      prefix = fgetc (dict);
      suff = fgetc (dict);
      fread (&term[prefix], sizeof (char), suff, dict);
      fread ((char *) &fcnt, sizeof (fcnt), 1, dict);
      fread ((char *) &wcnt, sizeof (wcnt), 1, dict);
      term[prefix + suff] = '\0';

      /* [RPAP - Jan 97: Endian Ordering] */
      NTOHUL(fcnt);
      NTOHUL(wcnt);

      if (binary)
	fwrite ((char *) &fcnt, sizeof (fcnt), 1, stdout);
      else
	{
	  if (term_dump)
	    printf ("%ld    \"%s\"\n", fcnt, term);
	  else
	    printf ("%ld\n", fcnt);
	}
      blk = BIO_Bblock_Init (Nstatic, fcnt);
      for (doc = i = 0; i < fcnt; i++)
	{
	  int num;
	  BBLOCK_DECODE (num, blk);
	  doc += num;
	  if (binary)
	    fwrite ((char *) &doc, sizeof (doc), 1, stdout);
	  else
	    printf (" %d", doc);
	  if (ifh.InvfLevel >= 2)
	    {
	      int count;
	      GAMMA_DECODE (count);
	      if (word_counts)
		if (binary)
		  fwrite ((char *) &count, sizeof (count), 1, stdout);
		else
		  printf (" %d", count);
	    }
	  if (!binary)
	    putchar ('\n');
	}
      while (__btg)
	DECODE_BIT;
    }
  DECODE_DONE
}
