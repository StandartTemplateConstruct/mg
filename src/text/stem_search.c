/**************************************************************************
 *
 * stem_search.c -- Functions for searching the blocked stemmed dictionary
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
 * $Id: stem_search.c,v 1.3 1994/10/20 03:57:04 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "memlib.h"
#include "messages.h"
#include "filestats.h"
#include "timing.h"

#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"
#include "words.h"
#include "locallib.h"
#include "stem_search.h"
#include "mg_errors.h"
#include "local_strings.h"


/*
   $Log: stem_search.c,v $
   * Revision 1.3  1994/10/20  03:57:04  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:08  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: stem_search.c,v 1.3 1994/10/20 03:57:04 tes Exp $";


stemmed_dict *
ReadStemDictBlk (File * stem_file)
{
  unsigned long i;
  stemmed_dict *sd;
  u_char *buffer;

  if (!(sd = Xmalloc (sizeof (stemmed_dict))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  sd->stem_file = stem_file;
  sd->MemForStemDict = 0;

  Fread (&sd->sdh, sizeof (sd->sdh), 1, stem_file);

  if (!(buffer = Xmalloc (sd->sdh.index_chars)))
    {
      Xfree (sd);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.index_chars;

  if (!(sd->index = Xmalloc (sd->sdh.num_blocks * sizeof (*sd->index))))
    {
      Xfree (sd);
      Xfree (buffer);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.num_blocks * sizeof (*sd->index);

  if (!(sd->pos = Xmalloc (sd->sdh.num_blocks * sizeof (*sd->pos))))
    {
      Xfree (sd);
      Xfree (buffer);
      Xfree (sd->index);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.num_blocks * sizeof (*sd->pos);

  if (!(sd->buffer = Xmalloc (sd->sdh.block_size * sizeof (*sd->buffer))))
    {
      Xfree (sd);
      Xfree (buffer);
      Xfree (sd->index);
      Xfree (sd->buffer);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.block_size * sizeof (*sd->buffer);

  sd->active = -1;

  for (i = 0; i < sd->sdh.num_blocks; i++)
    {
      register u_char len;
      sd->index[i] = buffer;
      len = Getc (stem_file);
      *buffer++ = len;
      Fread (buffer, sizeof (u_char), len, stem_file);
      buffer += len;
      Fread (&sd->pos[i], sizeof (*sd->pos), 1, stem_file);
    }
  mg_errno = MG_NOERROR;
  return sd;
}


static int 
GetBlock (stemmed_dict * sd, u_char * Word)
{
  register int lo = 0, hi = sd->sdh.num_blocks - 1;
  register int mid = 0, c = 0;
  while (lo <= hi)
    {
      mid = (lo + hi) / 2;
      c = compare (Word, sd->index[mid]);
      if (c < 0)
	hi = mid - 1;
      else if (c > 0)
	lo = mid + 1;
      else
	return mid;
    }
  return hi < 0 ? 0 : (c < 0 ? mid - 1 : mid);
}


/*
 * This function looks up a word in the stemmed dictionary, it returns -1
 * if the word cound not be found, and 0 if it successfully finds the word.
 * If count is non-null the ulong it is pointing to is set to the number of 
 * occurances of the stemmed word in the collection. i.e wcnt.
 * If doc_count is non-null the ulong it is pointing to is set to the number
 * of documents that the word occurs in. i.e fcnt
 * If invf_ptr is non-null the ulong it is pointing to is set to the position
 * of the inverted file where the entry for this word start.
 */
int 
FindWord (stemmed_dict * sd, u_char * Word, unsigned long *count,
	  unsigned long *doc_count, unsigned long *invf_ptr,
	  unsigned long *invf_len)
{
  register int lo, hi, mid, c;
  register unsigned int res;
  int block, num_indexes;
  unsigned long *first_word, *last_invf_len;
  unsigned short *num_words;
  u_char *base;
  unsigned short *index;
  u_char prev[MAXSTEMLEN + 1];

  block = GetBlock (sd, Word);
  if (sd->active != sd->pos[block])
    {
      Fseek (sd->stem_file, sd->pos[block] + sd->sdh.blocks_start, 0);
      Fread (sd->buffer, sd->sdh.block_size, sizeof (u_char), sd->stem_file);
      sd->active = sd->pos[block];
    }
  first_word = (unsigned long *) (sd->buffer);
  last_invf_len = (unsigned long *) (first_word + 1);
  num_words = (unsigned short *) (last_invf_len + 1);
  index = num_words + 1;
  num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;
  base = (u_char *) (index + num_indexes);

  lo = 0;
  hi = num_indexes - 1;
  while (lo <= hi)
    {
      mid = (lo + hi) / 2;
      c = compare (Word, base + index[mid] + 1);
      if (c < 0)
	hi = mid - 1;
      else if (c > 0)
	lo = mid + 1;
      else
	{
	  hi = mid;
	  break;
	}
    }
  if (hi < 0)
    hi = 0;

  res = hi * sd->sdh.lookback;
  base += index[hi];

  for (;;)
    {
      unsigned copy, suff;
      unsigned long invfp;
      if (res >= *num_words)
	return (-1);
      copy = *base++;
      suff = *base++;
      bcopy ((char *) base, (char *) (prev + copy + 1), suff);
      base += suff;
      *prev = copy + suff;

      c = compare (Word, prev);
      if (c < 0)
	return (-1);

      if (c == 0 && doc_count)
	bcopy ((char *) base, (char *) doc_count, sizeof (*doc_count));
      base += sizeof (*doc_count);

      if (c == 0 && count)
	bcopy ((char *) base, (char *) count, sizeof (*count));
      base += sizeof (*count);

      if (c == 0 && invf_ptr)
	{
	  bcopy ((char *) base, (char *) &invfp, sizeof (invf_ptr));
	  *invf_ptr = invfp;
	}
      base += sizeof (*invf_ptr);

      if (c == 0)
	{
	  /* Calculate invf_len is necessary */
	  unsigned long next_invfp;
	  if (!invf_len)
	    return (*first_word + res);

	  /* If the current word is the last word of the block the get the 
	     length from last_invf_len */
	  if (res == *num_words - 1)
	    {
	      *invf_len = *last_invf_len;
	      return (*first_word + res);
	    }

	  /* Skip over most of the next word to get to the invf_ptr */
	  base++;
	  suff = *base++;
	  base += suff + sizeof (unsigned long) * 2;
	  bcopy ((char *) base, (char *) &next_invfp, sizeof (next_invfp));
	  *invf_len = next_invfp - invfp;
	  return (*first_word + res);
	}
      res++;
    }
}


void 
FreeStemDict (stemmed_dict * sd)
{
  Xfree (sd->index[0]);
  Xfree (sd->index);
  Xfree (sd->buffer);
  Xfree (sd->pos);
  Xfree (sd);
}
