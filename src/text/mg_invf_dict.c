/**************************************************************************
 *
 * mg_invf_dict.c -- Program to build the blocked stemmed dictionary
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
 * $Id: mg_invf_dict.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg_files.h"
#include "invf.h"
#include "locallib.h"
#include "words.h"
#include "mg.h"

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:08  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:45  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:03  rjmcnab
   *** empty log message ***

   * Revision 1.4  1994/11/29  00:32:00  tes
   * Committing the new merged files and changes.
   *
   * Revision 1.3  1994/10/20  03:56:56  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:49  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_invf_dict.c 16583 2008-07-29 10:20:36Z davidb $";

int block_size = 1024 * 4;

int force = 0;

static void process_files (char *filename);

int main (int argc, char **argv)
{
  char *file_name = "";
  int ch;
  msg_prefix = argv[0];
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "f:d:b:Fh")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	file_name = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'b':
	block_size = atoi (optarg);
	break;
      case 'F':
	force = 1;
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-f input_file]"
		 "[-d data directory] [-b num] [-F] [-h]\n", argv[0]);
	exit (1);
      }

  process_files (file_name);
  return 0;
}




static void 
process_files (char *filename)
{
  FILE *id, *idb, *tmp, *ii;
  unsigned long i, pos, num, First_word, invf_ptr, invf_len;
  unsigned long last_ptr = 0;
  char *FName;
  struct invf_dict_header idh;
  struct stem_dict_header sdh;
  u_char prev[MAXSTEMLEN + 1];
  u_char *buffer;
  unsigned short *pointers;
  int buf_in_use;
  unsigned short ptrs_in_use, word_num;

  id = open_file (filename, INVF_DICT_SUFFIX, "rb", MAGIC_STEM_BUILD, MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  ii = open_file (filename, INVF_IDX_SUFFIX, "rb", MAGIC_INVI, MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  idb = create_file (filename, INVF_DICT_BLOCKED_SUFFIX, "w+b", MAGIC_STEM,
		     MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  FName = make_name (filename, ".tmp", NULL);
  if (!(tmp = fopen (FName, "w+b")))  /* [RPAP - Feb 97: WIN32 Port] */
    FatalError (1, "Unable to open \"%s\".\n", FName);

  /* Delete the file now */
  unlink (FName);

  fread (&idh, sizeof (idh), 1, id);
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

  sdh.lookback = idh.lookback;
  sdh.block_size = block_size;
  sdh.num_blocks = 0;
  sdh.index_chars = 0;
  sdh.blocks_start = 0;
  sdh.num_of_docs = idh.num_of_docs;
  sdh.static_num_of_docs = idh.static_num_of_docs;
  sdh.num_of_words = idh.num_of_words;
  sdh.stemmer_num = idh.stemmer_num;
  sdh.stem_method = idh.stem_method;
  sdh.indexed = 0;  /* [RPAP - Jan 97: Stem Index Change] */

  fwrite (&sdh, sizeof (sdh), 1, idb);

  if (!(buffer = Xmalloc (block_size + 512)))
    FatalError (1, "Unable to allocate memory for \"buffer\"\n");
  if (!(pointers = Xmalloc (block_size + 512)))
    FatalError (1, "Unable to allocate memory for \"buffer\"\n");

  buf_in_use = 0;
  pos = 0;
  word_num = 0;
  ptrs_in_use = 0;
  First_word = 0;
  for (i = 0; i < idh.dict_size; i++)
    {
      register unsigned long extra, copy, suff;
      unsigned long wcnt, fcnt;

      /* build a new word on top of prev */
      copy = getc (id);
      suff = getc (id);
      *prev = copy + suff;
      fread (prev + copy + 1, sizeof (u_char), suff, id);

      /* read other data, but no need to store it */
      fread (&fcnt, sizeof (fcnt), 1, id);
      fread (&wcnt, sizeof (wcnt), 1, id);

      /* [RPAP - Jan 97: Endian Ordering] */
      NTOHUL(fcnt);
      NTOHUL(wcnt);

      /* read in the inverted file position */
      fread (&invf_ptr, sizeof (invf_ptr), 1, ii);
      NTOHUL(invf_ptr);  /* [RPAP - Jan 97: Endian Ordering] */
      if (word_num % idh.lookback == 0)
	extra = copy + sizeof (*pointers);
      else
	extra = 0;
      if ((ptrs_in_use + 1) * sizeof (*pointers) + sizeof (ptrs_in_use) + extra +
	  buf_in_use + suff + 1 + sizeof (fcnt) + sizeof (wcnt) +
	  sizeof (First_word) + sizeof (invf_ptr) + sizeof (invf_len) > block_size)
	{
	  int chunk;
	  invf_len = invf_ptr - last_ptr;

	  /* [RPAP - Jan 97: Endian Ordering] */
	  HTONUL(First_word);
	  HTONUL(invf_len);
	  HTONUS(word_num);

	  fwrite (&First_word, sizeof (First_word), 1, tmp);
	  fwrite (&invf_len, sizeof (invf_len), 1, tmp);
	  fwrite (&word_num, sizeof (word_num), 1, tmp);
	  fwrite (pointers, sizeof (*pointers), ptrs_in_use, tmp);
	  fwrite (buffer, sizeof (u_char), buf_in_use, tmp);
	  bzero ((char *) buffer, block_size);
	  chunk = buf_in_use + ptrs_in_use * sizeof (*pointers) +
	    sizeof (ptrs_in_use) + sizeof (First_word) + sizeof (invf_len);
	  if (force && chunk < block_size)
	    {
	      fwrite (buffer, sizeof (u_char), block_size - chunk, tmp);
	      chunk = block_size;
	    }

	  pos += chunk;

	  buf_in_use = 0;
	  word_num = 0;
	  ptrs_in_use = 0;
	  sdh.num_blocks++;
	}

      if (word_num % idh.lookback == 0)
	{
	  HTONUS2(buf_in_use, pointers[ptrs_in_use++]);  /* [RPAP - Jan 97: Endian Ordering] */
	  suff += copy;
	  copy = 0;
	}
      buffer[buf_in_use++] = copy;
      buffer[buf_in_use++] = suff;
      bcopy ((char *) (prev + copy + 1), (char *) (buffer + buf_in_use), suff);
      buf_in_use += suff;
      HTONUL(fcnt);  /* [RPAP - Jan 97: Endian Ordering] */
      bcopy ((char *) &fcnt, (char *) (buffer + buf_in_use), sizeof (fcnt));
      buf_in_use += sizeof (fcnt);
      HTONUL(wcnt);  /* [RPAP - Jan 97: Endian Ordering] */
      bcopy ((char *) &wcnt, (char *) (buffer + buf_in_use), sizeof (wcnt));
      buf_in_use += sizeof (wcnt);
      last_ptr = invf_ptr;
      HTONUL(invf_ptr);  /* [RPAP - Jan 97: Endian Ordering] */
      bcopy ((char *) &invf_ptr, (char *) (buffer + buf_in_use), sizeof (invf_ptr));
      NTOHUL(invf_ptr);  /* [RPAP - Jan 97: Endian Ordering] */
      buf_in_use += sizeof (invf_ptr);
      if (buf_in_use + ptrs_in_use * sizeof (*pointers) +
	  sizeof (ptrs_in_use) > block_size)
	FatalError (1, "Fatal Internal Error # 34876234\n");
      if (word_num == 0)
	{
	  fwrite (prev, sizeof (u_char), *prev + 1, idb);
	  HTONUL(pos);  /* [RPAP - Jan 97: Endian Ordering] */
	  fwrite (&pos, sizeof (pos), 1, idb);
	  NTOHUL(pos);  /* [RPAP - Jan 97: Endian Ordering] */
	  sdh.index_chars += *prev + 1;
	  First_word = i;
	}
      word_num++;
    }
  if (buf_in_use)
    {
      int chunk;
      fread (&invf_ptr, sizeof (invf_ptr), 1, ii);
      NTOHUL(invf_ptr);  /* [RPAP - Jan 97: Endian Ordering] */
      invf_len = invf_ptr - last_ptr;

      /* [RPAP - Jan 97: Endian Ordering] */
      HTONUL(First_word);
      HTONUL(invf_len);
      HTONUS(word_num);

      fwrite (&First_word, sizeof (First_word), 1, tmp);
      fwrite (&invf_len, sizeof (invf_len), 1, tmp);
      fwrite (&word_num, sizeof (word_num), 1, tmp);
      fwrite (pointers, sizeof (*pointers), ptrs_in_use, tmp);
      fwrite (buffer, sizeof (u_char), buf_in_use, tmp);
      bzero ((char *) buffer, block_size);
      chunk = buf_in_use + ptrs_in_use * sizeof (*pointers) +
	sizeof (ptrs_in_use) + sizeof (First_word) + sizeof (invf_len);
      if (force && chunk < block_size)
	{
	  fwrite (buffer, sizeof (u_char), block_size - chunk, tmp);
	  chunk = block_size;
	}

      sdh.num_blocks++;
    }
  fclose (id);
  fclose (ii);

  rewind (tmp);
  sdh.blocks_start = sdh.index_chars + sizeof (u_long) + sizeof (sdh) +
    sdh.num_blocks * sizeof (pos);
  if (force)
    {
      int amount;
      amount = sdh.blocks_start % block_size;
      if (amount != 0)
	{
	  bzero ((char *) buffer, block_size);
	  fwrite (buffer, sizeof (u_char), block_size - amount, idb);
	  sdh.blocks_start += block_size - amount;
	}
    }

  while ((num = fread (buffer, sizeof (u_char), block_size, tmp)) != 0)
    fwrite (buffer, sizeof (u_char), num, idb);
  fclose (tmp);

  /* skip over the magic number */
  fseek (idb, sizeof (u_long), 0);

  /* [RPAP - Jan 97: Endian Ordering] */
  HTONUL(sdh.lookback);
  HTONUL(sdh.block_size);
  HTONUL(sdh.num_blocks);
  HTONUL(sdh.blocks_start);
  HTONUL(sdh.index_chars);
  HTONUL(sdh.num_of_docs);
  HTONUL(sdh.static_num_of_docs);
  HTONUL(sdh.num_of_words);
  HTONUL(sdh.stemmer_num);
  HTONUL(sdh.stem_method);
  HTONUL(sdh.indexed);

  fwrite (&sdh, sizeof (sdh), 1, idb);
  fclose (idb);


  Message ("Block size = %d\n", block_size);
  Message ("Number of blocks written = %d\n", NTOHUL(sdh.num_blocks));

}
