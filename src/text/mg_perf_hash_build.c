/**************************************************************************
 *
 * mg_perf_hash_build.c -- Program to build a perfect hash function
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
 * $Id: mg_perf_hash_build.c,v 1.3 1994/10/20 03:56:58 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "memlib.h"
#include "messages.h"
#include "timing.h"
#include "local_strings.h"
#include "perf_hash.h"

#include "mg_files.h"
#include "invf.h"
#include "locallib.h"
#include "words.h"
#include "mg.h"

/*
   $Log: mg_perf_hash_build.c,v $
   * Revision 1.3  1994/10/20  03:56:58  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:53  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_perf_hash_build.c,v 1.3 1994/10/20 03:56:58 tes Exp $";



#define POOL_SIZE 1024*1024

static void process_files (char *filename);

int r = -1;

int 
main (int argc, char **argv)
{
  ProgTime start;
  char *file_name = "";
  int ch;
  msg_prefix = argv[0];
  opterr = 0;
  while ((ch = getopt (argc, argv, "f:d:r:h")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	file_name = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'r':
	r = atoi (optarg);
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-f input_file]"
		 "[-d data directory] [-r random seed] [-h]\n", argv[0]);
	exit (1);
      }

  GetTime (&start);
  process_files (file_name);
  Message ("%s\n", ElapsedTime (&start, NULL));
  exit (0);
}





static void 
process_files (char *filename)
{
  FILE *dict, *hash;
  unsigned long i;
  u_char prev[MAXSTEMLEN + 1];
  struct invf_dict_header idh;
  perf_hash_data *phd;
  u_char *pool;
  int pool_left;
  u_char **starts;


  dict = open_file (filename, INVF_DICT_SUFFIX, "r",
		    MAGIC_STEM_BUILD, MG_ABORT);

  fread ((char *) &idh, sizeof (idh), 1, dict);

  hash = create_file (filename, INVF_DICT_HASH_SUFFIX, "w",
		      MAGIC_HASH, MG_ABORT);

  if (!(pool = Xmalloc (POOL_SIZE)))
    FatalError (1, "Out of memory");
  pool_left = POOL_SIZE;

  if (!(starts = Xmalloc (sizeof (u_char *) * idh.dict_size)))
    FatalError (1, "Out of memory");

  for (i = 0; i < idh.dict_size; i++)
    {
      register unsigned long copy, suff, l;
      unsigned long wcnt, fcnt;

      /* build a new word on top of prev */
      copy = getc (dict);
      suff = getc (dict);
      *prev = copy + suff;
      fread (prev + copy + 1, sizeof (u_char), suff, dict);

      /* read other data, but no need to store it */
      fread (&fcnt, sizeof (fcnt), 1, dict);
      fread (&wcnt, sizeof (wcnt), 1, dict);
      l = *prev + 1;
      if (pool_left < l)
	{
	  pool = Xmalloc (POOL_SIZE);
	  pool_left = POOL_SIZE;
	}
      starts[i] = pool;
      bcopy ((char *) prev, (char *) pool, l);
      pool += l;
      pool_left -= l;
    }
  if (!(phd = gen_hash_func (idh.dict_size, starts, r)))
    FatalError (1, "Unable to generate hash function");
  if (write_perf_hash_data (hash, phd) == -1)
    FatalError (1, "Unable to write hash function");
  fclose (dict);
  fclose (hash);
}
