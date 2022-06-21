/**************************************************************************
 *
 * mgstemidxlist.c -- Text dumper for the stem indexes
 * Copyright (C) 1997  Ross Peeters
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
 **************************************************************************/

#include "sysfuncs.h"

#include "messages.h"
#include "memlib.h"
#include "local_strings.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg_files.h"
#include "mg.h"
#include "invf.h"
#include "words.h"
#include "backend.h"

void
read_3_in_4 (FILE * idbi)
{
  unsigned long i;
  stemmed_idx *si;
  u_char *buffer;
  int block = 0;

  if (!(si = Xmalloc (sizeof (stemmed_idx))))
    {
      return;
    }

  si->MemForStemIdx = 0;

  fread (&(si->sih), sizeof (si->sih), 1, idbi);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(si->sih.lookback);
  NTOHUL(si->sih.block_size);
  NTOHUL(si->sih.num_blocks);
  NTOHUL(si->sih.blocks_start);
  NTOHUL(si->sih.index_chars);
  NTOHUL(si->sih.num_of_words);

  if (!(buffer = Xmalloc (si->sih.index_chars)))
    {
      Xfree (si);
      return;
    };
  si->MemForStemIdx += si->sih.index_chars;

  if (!(si->index = Xmalloc (si->sih.num_blocks * sizeof (*si->index))))
    {
      Xfree (si);
      Xfree (buffer);
      return;
    };
  si->MemForStemIdx += si->sih.num_blocks * sizeof (*si->index);

  if (!(si->pos = Xmalloc (si->sih.num_blocks * sizeof (*si->pos))))
    {
      Xfree (si->index);
      Xfree (si);
      Xfree (buffer);
      return;
    };
  si->MemForStemIdx += si->sih.num_blocks * sizeof (*si->pos);

  if (!(si->buffer = Xmalloc (si->sih.block_size * sizeof (*si->buffer))))
    {
      Xfree (buffer);
      Xfree (si->index);
      Xfree (si->buffer);
      Xfree (si);
      return;
    };
  si->MemForStemIdx += si->sih.block_size * sizeof (*si->buffer);

  si->active = -1;

  for (i = 0; i < si->sih.num_blocks; i++)
    {
      register u_char len;
      si->index[i] = buffer;
      len = fgetc (idbi);
      *buffer++ = len;
      fread (buffer, sizeof (u_char), len, idbi);
      buffer += len;
      fread (&si->pos[i], sizeof (*si->pos), 1, idbi);
      NTOHUL(si->pos[i]);  /* [RPAP - Jan 97: Endian Ordering] */
    }

  printf ("# lookback     = %lu\n", si->sih.lookback);
  printf ("# block_size   = %lu\n", si->sih.block_size);
  printf ("# num_blocks   = %lu\n", si->sih.num_blocks);
  printf ("# blocks_start = %lu\n", si->sih.blocks_start);
  printf ("# index_chars  = %lu\n", si->sih.index_chars);
  printf ("# num_of_words = %lu\n", si->sih.num_of_words);

  block = 0;
  while (block < si->sih.num_blocks)
    {
      unsigned long *first_word;
      unsigned short *num_words;
      unsigned short *index;
      long res;
      u_char *base;
      int num_indexes;

      /* Read in next block */
      fseek (idbi, si->pos[block] + si->sih.blocks_start, 0);
      fread (si->buffer, si->sih.block_size, sizeof (u_char), idbi);
      si->active = si->pos[block];

      first_word = (unsigned long *) (si->buffer);
      NTOHUL(*first_word);  /* [RPAP - Jan 97: Endian Ordering] */
      num_words = (unsigned short *) (first_word + 1);
      NTOHUS(*num_words);  /* [RPAP - Jan 97: Endian Ordering] */
      index = num_words + 1;
      num_indexes = ((*num_words - 1) / si->sih.lookback) + 1;

      /* [RPAP - Jan 97: Endian Ordering] */
      for (i = 0; i < num_indexes; i++)
	NTOHUS(index[i]);

      base = (u_char *) (index + num_indexes);
      base += index[0];

      printf ("\n# block      = %d\n", block);
      printf ("# first_word = %lu\n", *first_word);
      printf ("# num_words  = %u\n", *num_words);

      res = 0;
      while (res < *num_words)
	{
	  unsigned copy, suff;
	  u_char prev[MAXSTEMLEN + 1];
	  unsigned int num_entries, num_cases, blk;
	  unsigned short blk_index, offset;

	  /* Read word entry */
	  copy = *base++;
	  suff = *base++;
	  bcopy ((char *) base, (char *) (prev + copy + 1), suff);
	  *prev = copy + suff;
	  base += suff;
	  bcopy ((char *) base, (char *) &num_entries, sizeof (num_entries));
	  base += sizeof (num_entries);
	  NTOHUI(num_entries);  /* [RPAP - Jan 97: Endian Ordering] */
	  printf ("%u \"%s\"\n", num_entries, word2str (prev));

	  /* For all the PosEntries for the word... */
	  for (i = 0; i < num_entries; i++)
	    {
	      bcopy ((char *) base, (char *) &num_cases, sizeof (num_cases));
	      NTOHUI(num_cases);  /* [RPAP - Jan 97: Endian Ordering] */
	      base += sizeof (num_cases);
	      bcopy ((char *) base, (char *) &blk, sizeof (blk));
	      NTOHUI(blk);  /* [RPAP - Jan 97: Endian Ordering] */
	      base += sizeof (blk);
	      bcopy ((char *) base, (char *) &blk_index, sizeof (blk_index));
	      NTOHUS(blk_index);  /* [RPAP - Jan 97: Endian Ordering] */
	      base += sizeof (blk_index);
	      bcopy ((char *) base, (char *) &offset, sizeof (offset));
	      NTOHUS(offset);  /* [RPAP - Jan 97: Endian Ordering] */
	      base += sizeof (offset);

	      printf ("    -> %4u %4u %4u %4u\n", num_cases, blk, blk_index, offset);
	    }
	  res++;
	}
      block++;
    }
  fclose (idbi);
}

int
main (int argc, char **argv)
{
  FILE *idbi;
  char *filename = "";
  int ch;
  int stem_method = 0;

  msg_prefix = argv[0];
  opterr = 0;
  while ((ch = getopt (argc, argv, "f:d:hs:")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	filename = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 's':
	stem_method = atoi (optarg);
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-d data directory] [-h] -s 1|2|3 -f name\n", argv[0]);
	exit (1);
      }

  /* Open required files */
  switch (stem_method)
    {
    case (1):
      idbi = open_file (filename, INVF_DICT_BLOCKED_1_SUFFIX, "rb", MAGIC_STEM_1,
			MG_ABORT);
      break;
    case (2):
      idbi = open_file (filename, INVF_DICT_BLOCKED_2_SUFFIX, "rb", MAGIC_STEM_2,
			MG_ABORT);
      break;
    case (3):
      idbi = open_file (filename, INVF_DICT_BLOCKED_3_SUFFIX, "rb", MAGIC_STEM_3,
			MG_ABORT);
      break;
    default:
      FatalError (1, "Stem method must be 1, 2 or 3\n");
    }

  if (!idbi)
    FatalError (1, "Could NOT open file");

  read_3_in_4 (idbi);

  return 0;
}
