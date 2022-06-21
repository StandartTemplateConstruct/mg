/**************************************************************************
 *
 * mg_text_estimate.c -- Program to estimate the compressed text size
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
 * $Id: mg_text_estimate.c,v 1.3 1994/10/20 03:56:59 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "local_strings.h"
#include "bitio_gen.h"
#include "bitio_m.h"
#include "bitio_m_stdio.h"
#include "timing.h"
#include "heap.h"

#include "mg_files.h"
#include "locallib.h"
#include "invf.h"
#include "text.h"
#include "words.h"
#include "mg.h"
#include "comp_dict.h"
#include "hash.h"





/*
   $Log: mg_text_estimate.c,v $
   * Revision 1.3  1994/10/20  03:56:59  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:54  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_text_estimate.c,v 1.3 1994/10/20 03:56:59 tes Exp $";

typedef struct stats_t
  {
    u_long freq, occur;
    u_long len;
  }
stats_t;

static u_long huff_count = 0;
static u_long escape_count = 0;


char mode = 'H';

static double ReadInWordsStandard (char *name, u_long * num_bytes);
static double ReadInWordsSpecial (char *name, u_long * num_bytes,
				  u_long * num_extra_bytes,
				  u_long * aux_compressed);




int 
main (int argc, char **argv)
{
  char *stats_dict = NULL, *comp_dict = NULL;
  ProgTime StartTime;
  u_long num_bytes, num_extra_bytes = 0, aux_compressed;
  int ch;
  double bytes;
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "BMDYHh")) != -1)
    switch (ch)
      {
      case 'B':
	Message ("The compressed text size for this method cannot be\n"
		 "estimated exactly, so an upper bound will be calculated.");
	mode = ch;
	break;
      case 'M':
	Message ("The compressed text size for this method cannot be\n"
		 "estimated exactly, so an upper bound will be calculated.");
	mode = 'Y';
	break;
      case 'D':
      case 'Y':
      case 'H':
	mode = ch;
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-h] [B|M|D|Y|H] stats_dict comp_dict\n",
		 argv[0]);
	exit (1);
      }

  if (optind < argc)
    stats_dict = argv[optind++];
  else
    stats_dict = NULL;
  if (optind < argc)
    comp_dict = argv[optind++];
  else
    {
      fprintf (stderr, "usage: %s [-h] [B|M|D|Y|H] stats_dict comp_dict\n",
	       argv[0]);
      exit (1);
    }



  GetTime (&StartTime);

  if (LoadCompressionDictionary (comp_dict) == COMPERROR)
    exit (1);

  if (mode == 'H')
    bytes = ReadInWordsStandard (stats_dict, &num_bytes);
  else
    bytes = ReadInWordsSpecial (stats_dict, &num_bytes, &num_extra_bytes,
				&aux_compressed);


  Message ("Compressed Text %.2f Mb ( %.0f bytes   %0.3f %%)",
	   bytes / 1024 / 1024, bytes, bytes * 100 / num_bytes);
  Message ("Words portion of the dictionary %.2f Mb ( %.0f bytes   %0.3f %%)",
	   Words_disk / 1024.0 / 1024, Words_disk * 1.0,
	   (Words_disk * 100.0) / num_bytes);
  if (mode == 'H')
    Message ("Huffman info for chars in dictionary %.2f Mb ( %.0f bytes   %0.3f %%)",
	     Chars_disk / 1024.0 / 1024, Chars_disk * 1.0,
	     (Chars_disk * 100.0) / num_bytes);
  else
    {
      Message ("Aux dictionary (Uncompressed) %.2f Mb ( %.0f bytes   %0.3f %%)",
	       num_extra_bytes / 1024.0 / 1024, num_extra_bytes * 1.0,
	       (num_extra_bytes * 100.0) / num_bytes);
      Message ("Aux dictionary (Compressed)   %.2f Mb ( %.0f bytes   %0.3f %%)",
	       aux_compressed / 1024.0 / 1024, aux_compressed * 1.0,
	       (aux_compressed * 100.0) / num_bytes);
    }
  Message ("Words and non-words in the text %u", huff_count + escape_count);
  Message ("Words and non-words coded by escapes %u ( %0.3f %%)", escape_count,
	   escape_count * 100.0 / huff_count);
  Message ("%s", ElapsedTime (&StartTime, NULL));

  exit(0);
}





static double 
ReadInWordsStandard (char *name, u_long * num_bytes)
{
  compression_stats_header csh;
  FILE *f;
  u_long which;
  double bytes, bits = 0;

  f = open_named_file (name, "r", MAGIC_STATS_DICT, MG_ABORT);

  fread (&csh, sizeof (csh), 1, f);

  for (which = 0; which < 2; which++)
    {
      frags_stats_header fsh;
      int j;
      fread (&fsh, sizeof (fsh), 1, f);
      for (j = 0; j < fsh.num_frags; j++)
	{
	  int len, freq, res, occur_num;
	  u_char Word[16];
	  fread (&freq, sizeof (freq), 1, f);
	  fread (&occur_num, sizeof (occur_num), 1, f);
	  len = fgetc (f);

	  Word[0] = len;
	  fread (Word + 1, len, 1, f);

	  /* Search the hash table for Word */
	  if (ht[which])
	    {
	      register unsigned long hashval, step;
	      register int tsize = ht[which]->size;
	      register u_char **wptr;
	      HASH (hashval, step, Word, tsize);
	      for (;;)
		{
		  register u_char *s1;
		  register u_char *s2;
		  register int len;
		  wptr = ht[which]->table[hashval];
		  if (wptr == NULL)
		    {
		      res = COMPERROR;
		      break;
		    }

		  /* Compare the words */
		  s1 = Word;
		  s2 = *wptr;
		  len = *s1 + 1;
		  for (; len; len--)
		    if (*s1++ != *s2++)
		      break;

		  if (len)
		    {
		      hashval += step;
		      if (hashval >= tsize)
			hashval -= tsize;
		    }
		  else
		    {
		      res = ht[which]->table[hashval] - ht[which]->words;
		      break;
		    }
		}
	    }
	  else
	    res = COMPERROR;

	  /* Check that the word was found in the dictionary */
	  if (res == COMPERROR)
	    {
	      escape_count += freq;
	      if (cdh.dict_type == MG_COMPLETE_DICTIONARY)
		Message ("Unknown word \"%.*s\"\n", *Word, Word + 1);
	      if (cdh.dict_type == MG_PARTIAL_DICTIONARY)
		{
		  unsigned i;
		  if (ht[which])
		    {
		      res = ht[which]->hd->num_codes - 1;
		      bits += ht[which]->hd->clens[res] * freq;
		    }
		  bits += lens_huff[which].clens[Word[0]] * freq;
		  for (i = 0; i < Word[0]; i++)
		    bits += char_huff[which].clens[Word[i + 1]] * freq;
		}
	      if (cdh.dict_type == MG_SEED_DICTIONARY)
		{
		  unsigned i;
		  if (ht[which])
		    {
		      res = ht[which]->hd->num_codes - 1;
		      bits += ht[which]->hd->clens[res] * freq;
		    }
		  bits += lens_huff[which].clens[Word[0]] * freq;
		  for (i = 0; i < Word[0]; i++)
		    bits += char_huff[which].clens[Word[i + 1]] * freq;
		}
	    }
	  else
	    {
	      bits += ht[which]->hd->clens[res] * freq;
	      huff_count += freq;
	    }
	}
    }
  fclose (f);


  /*
   * The 5.5 bits added on per document consist of
   *   1 bit for the word/non-word flag at the start of the document.
   *   1 bit on average for document termination.
   *   3.5 bits on average for padding to a byte boundary.
   */
  bits += 5.5 * csh.num_docs;

  bytes = bits / 8 + sizeof (compressed_text_header) + sizeof (u_long);

  *num_bytes = csh.num_bytes;

  return bytes;
}


static int 
stats_comp (const void *a, const void *b)
{
  return ((stats_t *) a)->occur - ((stats_t *) b)->occur;
}





static double 
ReadInWordsSpecial (char *name, u_long * num_bytes,
		    u_long * num_extra_bytes,
		    u_long * aux_compressed)
{
  compression_stats_header csh;
  FILE *f;
  u_long magic, which;
  double bytes, bits = 0;
  double size[2];

  if (!(f = fopen (name, "r")))
    FatalError (1, "Unable to open file \"%s\"\n", name);

  if (fread (&magic, sizeof (magic), 1, f) != 1)
    FatalError (1, "Unable to read magic number");

  if (magic != MAGIC_STATS_DICT)
    FatalError (1, "Bad magic number");

  fread (&csh, sizeof (csh), 1, f);

  for (which = 0; which < 2; which++)
    {
      stats_t *stats;
      frags_stats_header fsh;
      int j, num = 0;
      long lens[16], chars[256];

      bzero ((char *) lens, sizeof (lens));
      bzero ((char *) chars, sizeof (chars));

      fread (&fsh, sizeof (fsh), 1, f);

      stats = Xmalloc (fsh.num_frags * sizeof (*stats));
      if (!stats)
	FatalError (1, "Unable to allocate memory for stats");
      for (j = 0; j < fsh.num_frags; j++)
	{
	  int len, res;
	  u_char Word[16];
	  fread (&stats[num].freq, sizeof (stats[num].freq), 1, f);
	  fread (&stats[num].occur, sizeof (stats[num].occur), 1, f);
	  len = fgetc (f);
	  stats[num].len = len + 1;

	  Word[0] = len;
	  fread (Word + 1, len, 1, f);

	  /* Search the hash table for Word */
	  if (ht[which])
	    {
	      register unsigned long hashval, step;
	      register int tsize = ht[which]->size;
	      register u_char **wptr;
	      HASH (hashval, step, Word, tsize);
	      for (;;)
		{
		  register u_char *s1;
		  register u_char *s2;
		  register int len;
		  wptr = ht[which]->table[hashval];
		  if (wptr == NULL)
		    {
		      res = COMPERROR;
		      break;
		    }

		  /* Compare the words */
		  s1 = Word;
		  s2 = *wptr;
		  len = *s1 + 1;
		  for (; len; len--)
		    if (*s1++ != *s2++)
		      break;

		  if (len)
		    {
		      hashval += step;
		      if (hashval >= tsize)
			hashval -= tsize;
		    }
		  else
		    {
		      res = ht[which]->table[hashval] - ht[which]->words;
		      break;
		    }
		}
	    }
	  else
	    res = COMPERROR;

	  assert (stats[num].freq > 0);

	  /* Check that the word was found in the dictionary */
	  if (res == COMPERROR)
	    {
	      if (cdh.dict_type == MG_COMPLETE_DICTIONARY)
		Message ("Unknown word \"%.*s\"\n", *Word, Word + 1);
	      if ((cdh.dict_type == MG_PARTIAL_DICTIONARY ||
		   cdh.dict_type == MG_SEED_DICTIONARY) &&
		  ht[which])
		{
		  int k;
		  res = ht[which]->hd->num_codes - 1;
		  bits += ht[which]->hd->clens[res] * stats[num].freq;
		  lens[Word[0]]++;
		  for (k = 0; k < (int) Word[0]; k++)
		    chars[Word[k + 1]]++;
		}
	      num++;
	    }
	  else
	    {
	      bits += ht[which]->hd->clens[res] * stats[num].freq;
	    }
	}

      {
	int j, num_buckets = 0;
	int start[10], end[10];
	long flens[16], fchars[256];

	for (j = 0; j < 256; j++)
	  if (!chars[j] && INAWORD (j) == which)
	    fchars[j] = 1;
	  else
	    fchars[j] = chars[j];
	for (j = 0; j < 16; j++)
	  if (!lens[j])
	    flens[j] = 1;
	  else
	    flens[j] = lens[j];

	size[which] = Calculate_Huffman_Size (16, flens, lens) +
	  Calculate_Huffman_Size (256, fchars, chars);


	qsort (stats, num, sizeof (*stats), stats_comp);


	if (mode == 'Y')
	  {
	    int k;
	    num_buckets = 1;
	    start[0] = 0;
	    end[0] = cdh.num_words[which] - 1;
	    Message ("There are %d novel %swords", num, which ? "" : "non-");
	    while (end[num_buckets - 1] < num)
	      {
		start[num_buckets] = end[num_buckets - 1] + 1;
		end[num_buckets] = start[num_buckets] +
		  (end[num_buckets - 1] - start[num_buckets - 1]) * 2;
		num_buckets++;
	      }
	    for (k = 0; k < num_buckets; k++)
	      Message ("Bucket %d   %d <= x <= %d\n", k + 1, start[k], end[k]);
	  }

	for (j = 0; j < num; j++)
	  {
	    (*num_extra_bytes) += stats[j].len;
	    switch (mode)
	      {
	      case 'D':
		bits += BIO_Delta_Length (j + 1) * stats[j].freq;
		break;
	      case 'B':
		bits += BIO_Binary_Length (j + 1, num) * stats[j].freq;
		break;
	      case 'Y':
		{
		  int k = 0;
		  while (j > end[k])
		    k++;
		  if (!(j - start[k] + 1 >= 1 &&
			j - start[k] + 1 <= end[k] - start[k] + 1))
		    {
		      Message ("k = %d, j = %d", k, j);
		      assert (j - start[k] + 1 >= 1 &&
			      j - start[k] + 1 <= end[k] - start[k] + 1);
		    }
		  else
		    {
		      bits += (BIO_Gamma_Length (k + 1) +
			       BIO_Binary_Length (j - start[k] + 1,
						  end[k] - start[k] + 1)) *
			stats[j].freq;
		    }
		}
	      }
	  }
	if (mode == 'B' && num)
	  bits += BIO_Delta_Length (num) * csh.num_docs;
      }
      Xfree (stats);
    }
  fclose (f);

  *aux_compressed = (size[0] + size[1]) / 8;

  /*
   * The 5.5 bits added on per document consist of
   *   1 bit for the word/non-word flag at the start of the document.
   *   1 bit on average for document termination.
   *   3.5 bits on average for padding to a byte boundary.
   */
  bits += 5.5 * csh.num_docs;

  bytes = bits / 8 + sizeof (compressed_text_header) + sizeof (u_long);

  *num_bytes = csh.num_bytes;

  return bytes;
}
