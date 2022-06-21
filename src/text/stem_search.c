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
 * $Id: stem_search.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "filestats.h"
#include "timing.h"
#include "local_strings.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"
#include "words.h"
#include "locallib.h"
#include "stem_search.h"
#include "mg_errors.h"
#include "term_lists.h"
#include "stemmer.h"


/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:22  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.3  1999/07/02 00:18:55  rjmcnab
   Changed so FindWords could be used in new ways.

   Revision 1.2  1998/11/25 07:55:51  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:39  rjmcnab
   *** empty log message ***

   * Revision 1.3  1994/10/20  03:57:04  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:08  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: stem_search.c 16583 2008-07-29 10:20:36Z davidb $";


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
  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(sd->sdh.lookback);
  NTOHUL(sd->sdh.block_size);
  NTOHUL(sd->sdh.num_blocks);
  NTOHUL(sd->sdh.blocks_start);
  NTOHUL(sd->sdh.index_chars);
  NTOHUL(sd->sdh.num_of_docs);
  NTOHUL(sd->sdh.static_num_of_docs);
  NTOHUL(sd->sdh.num_of_words);
  NTOHUL(sd->sdh.stem_method);
  NTOHUL(sd->sdh.indexed);

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
      NTOHUL(sd->pos[i]); /* [RPAP - Jan 97: Endian Ordering] */
    }

  mg_errno = MG_NOERROR;

  /*  fprintf (stderr, "mem for stem dict = %i\n", sd->MemForStemDict); */

  return sd;
}


/* [RPAP - Jan 97: Stem Index Change] */
stemmed_idx *
ReadStemIdxBlk (File * stem_idx_file)
{
  unsigned long i;
  stemmed_idx *si;
  u_char *buffer;

  if (!(si = Xmalloc (sizeof (stemmed_idx))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  si->stem_idx_file = stem_idx_file;
  si->MemForStemIdx = 0;

  Fread (&si->sih, sizeof (si->sih), 1, stem_idx_file);
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
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  si->MemForStemIdx += si->sih.index_chars;

  if (!(si->index = Xmalloc (si->sih.num_blocks * sizeof (*si->index))))
    {
      Xfree (si);
      Xfree (buffer);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  si->MemForStemIdx += si->sih.num_blocks * sizeof (*si->index);

  if (!(si->pos = Xmalloc (si->sih.num_blocks * sizeof (*si->pos))))
    {
      Xfree (si->index);
      Xfree (si);
      Xfree (buffer);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  si->MemForStemIdx += si->sih.num_blocks * sizeof (*si->pos);

  if (!(si->buffer = Xmalloc (si->sih.block_size * sizeof (*si->buffer))))
    {
      Xfree (buffer);
      Xfree (si->index);
      Xfree (si->buffer);
      Xfree (si);
      mg_errno = MG_NOMEM;
      return (NULL);
    };
  si->MemForStemIdx += si->sih.block_size * sizeof (*si->buffer);

  si->active = -1;

  for (i = 0; i < si->sih.num_blocks; i++)
    {
      register u_char len;
      si->index[i] = buffer;
      len = Getc (stem_idx_file);
      *buffer++ = len;
      Fread (buffer, sizeof (u_char), len, stem_idx_file);
      buffer += len;
      Fread (&si->pos[i], sizeof (*si->pos), 1, stem_idx_file);
      NTOHUL(si->pos[i]);  /* [RPAP - Jan 97: Endian Ordering] */
    }
  mg_errno = MG_NOERROR;

  /*  fprintf (stderr, "mem for stem idx = %i\n", si->MemForStemIdx); */

  return si;
}


/* [RPAP - Jan 97: Stem Index Change] */
/* word should be appropriately stemed */
static int
GetIdxBlock (stemmed_idx * si, u_char * word)
{
  register int lo = 0, hi = si->sih.num_blocks - 1;
  register int mid = 0, c = 0;

  while (lo <= hi)
    {
      mid = (lo + hi) / 2;
      c = casecompare (word, si->index[mid]);
      if (c < 0)
	hi = mid - 1;
      else if (c > 0)
	lo = mid + 1;
      else
	return mid;
    }
  return hi < 0 ? 0 : (c < 0 ? mid - 1 : mid);
}


static int 
GetBlock (stemmed_dict * sd, u_char * Word)
{
  register int lo = 0, hi = sd->sdh.num_blocks - 1;
  register int mid = 0, c = 0;
  while (lo <= hi)
    {
      mid = (lo + hi) / 2;
      c = casecompare (Word, sd->index[mid]);  /* [RPAP - Jan 97: Stem Index Change] */
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
  /* [RPAP - Jan 97: Endian Ordering] */
  if (sd->active != sd->pos[block])
    {
      int i;

      Fseek (sd->stem_file, sd->pos[block] + sd->sdh.blocks_start, 0);
      Fread (sd->buffer, sd->sdh.block_size, sizeof (u_char), sd->stem_file);
      sd->active = sd->pos[block];

      /* [RPAP - Jan 97: Endian Ordering] */
      first_word = (unsigned long *) (sd->buffer);
      NTOHUL(*first_word);
      last_invf_len = (unsigned long *) (first_word + 1);
      NTOHUL(*last_invf_len);
      num_words = (unsigned short *) (last_invf_len + 1);
      NTOHUS(*num_words);
      index = num_words + 1;
      num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;

      for (i = 0; i < num_indexes; i++)
	NTOHUS(index[i]);
    }
  else
    {
      first_word = (unsigned long *) (sd->buffer);
      last_invf_len = (unsigned long *) (first_word + 1);
      num_words = (unsigned short *) (last_invf_len + 1);
      index = num_words + 1;
      num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;
    }
  base = (u_char *) (index + num_indexes);

  lo = 0;
  hi = num_indexes - 1;
  while (lo <= hi)
    {
      mid = (lo + hi) / 2;
      c = casecompare (Word, base + index[mid] + 1);  /* [RPAP - Jan 97: Stem Index Change] */
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

      c = casecompare (Word, prev);   /* [RPAP - Jan 97: Stem Index Change] */
      if (c < 0)
	return (-1);

      if (c == 0 && doc_count)
	{
	  bcopy ((char *) base, (char *) doc_count, sizeof (*doc_count));
	  NTOHUL(*doc_count);  /* [RPAP - Jan 97: Endian Ordering] */
	}
      base += sizeof (*doc_count);

      if (c == 0 && count)
	{
	  bcopy ((char *) base, (char *) count, sizeof (*count));
	  NTOHUL(*count);  /* [RPAP - Jan 97: Endian Ordering] */
	}
      base += sizeof (*count);

      if (c == 0 && invf_ptr)
	{
	  bcopy ((char *) base, (char *) &invfp, sizeof (invf_ptr));
	  NTOHUL(invfp);  /* [RPAP - Jan 97: Endian Ordering] */
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
	  NTOHUL(next_invfp);  /* [RPAP - Jan 97: Endian Ordering] */
	  *invf_len = next_invfp - invfp;
	  return (*first_word + res);
	}
      res++;
    }
}


/* [RPAP - Jan 97: Stem Index Change] */
int 
FindWords (stemmed_dict * sd, u_char * sWord, int stem_method, TermList ** tl)
{
  register unsigned int res;
  unsigned int idx_res;
  unsigned copy, suff;
  int j, k;

  int block, num_indexes;
  unsigned long *first_word, *last_invf_len;
  unsigned short *num_words;
  u_char *base;
  unsigned short *index;
  u_char prev[MAXSTEMLEN + 1];

  int idx_block, idx_num_indexes;
  unsigned long *idx_first_word;
  unsigned short *idx_num_words;
  u_char *idx_base;
  unsigned short *idx_index;
  u_char idx_prev[MAXSTEMLEN + 1];

  unsigned int num_entries, num_cases;
  unsigned short blk_index, offset;
  stemmed_idx * si = NULL;

  /* handle stem_method 0 seperately */
  if (stem_method == 0) {
    TermEntry te;
    if ((te.WE.word_num = FindWord (sd, sWord, &te.WE.count, &te.WE.doc_count,
				   &te.WE.invf_ptr, &te.WE.invf_len)) != -1) {
      te.WE.max_doc_count = te.WE.doc_count;

      te.Count = 1;
      te.Word = copy_string (sWord);
      if (!te.Word)
	FatalError (1, "Could NOT create memory to add term");
      te.Stem = copy_string (sWord);
      if (!te.Stem)
	FatalError (1, "Could NOT create memory to add term");
      /*      te.query_mask = NULL;*/
      
      AddTermEntry (tl, &te);
      return (*tl)->num;
      
    } else {
      /* didn't match */
      return 0;
    }
  }
  
  if (stem_method == 1)
    si = sd->stem1;
  else if (stem_method == 2)
    si = sd->stem2;
  else
    si = sd->stem3;

  /* Locate block */
  idx_block = GetIdxBlock (si, sWord);

  /* [RPAP - Jan 97: Endian Ordering] */
  if (si->active != si->pos[idx_block])
    {
      Fseek (si->stem_idx_file, si->pos[idx_block] + si->sih.blocks_start, 0);
      Fread (si->buffer, si->sih.block_size, sizeof (u_char), si->stem_idx_file);
      si->active = si->pos[idx_block];

      idx_first_word = (unsigned long *) (si->buffer);
      NTOHUL(*idx_first_word);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_num_words = (unsigned short *) (idx_first_word + 1);
      NTOHUS(*idx_num_words);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_index = idx_num_words + 1;
      idx_num_indexes = ((*idx_num_words - 1) / si->sih.lookback) + 1;

      /* [RPAP - Jan 97: Endian Ordering] */
      for (j = 0; j < idx_num_indexes; j++)
	NTOHUS(idx_index[j]);
    }
  else
    {
      idx_first_word = (unsigned long *) (si->buffer);
      idx_num_words = (unsigned short *) (idx_first_word + 1);
      idx_index = idx_num_words + 1;
      idx_num_indexes = ((*idx_num_words - 1) / si->sih.lookback) + 1;
    }
  idx_base = (u_char *) (idx_index + idx_num_indexes);
  
  {
    /* Locate 3-in-4 block */
    register int lo, hi, mid, c;
    lo = 0;
    hi = idx_num_indexes - 1;
    while (lo <= hi)
      {
	mid = (lo + hi) / 2;
	c = casecompare (sWord, idx_base + idx_index[mid] + 1);
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

    idx_res = hi * si->sih.lookback;
    idx_base += idx_index[hi];
  }

  /* Locate actual word entry */
  for (;;)
    {
      int c;
      if (idx_res >= *idx_num_words)
	return (-1);
      copy = *idx_base++;
      suff = *idx_base++;
      bcopy ((char *) idx_base, (char *) (idx_prev + copy + 1), suff);
      idx_base += suff;
      *idx_prev = copy + suff;

      c = casecompare (sWord, idx_prev);
      if (c < 0)
	return (-1);
     
      bcopy ((char *) idx_base, (char *) &num_entries, sizeof (num_entries));
      NTOHUI(num_entries);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_base += sizeof (num_entries);
      
      if (c > 0)
	idx_base += num_entries * (sizeof (num_cases) + sizeof (block) + 
				   sizeof (blk_index) + sizeof (offset));

      else
	break;

      idx_res++;
    }

  for (k = 0; k < num_entries; k++)
    {
      unsigned copy, suff;
      unsigned long invfp;
      /* Read next stem index pos */
      bcopy ((char *) idx_base, (char *) &num_cases, sizeof (num_cases));
      NTOHUI(num_cases);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_base += sizeof (num_cases);
      bcopy ((char *) idx_base, (char *) &block, sizeof (block));
      NTOHUI(block);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_base += sizeof (block);
      bcopy ((char *) idx_base, (char *) &blk_index, sizeof (blk_index));
      NTOHUS(blk_index);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_base += sizeof (blk_index);
      bcopy ((char *) idx_base, (char *) &offset, sizeof (offset));
      NTOHUS(offset);  /* [RPAP - Jan 97: Endian Ordering] */
      idx_base += sizeof (offset);

      /* [RPAP - Jan 97: Endian Ordering] */
      if (sd->active != sd->pos[block])
	{
	  Fseek (sd->stem_file, sd->pos[block] + sd->sdh.blocks_start, 0);
	  Fread (sd->buffer, sd->sdh.block_size, sizeof (u_char), sd->stem_file);
	  sd->active = sd->pos[block];

	  first_word = (unsigned long *) (sd->buffer);
	  NTOHUL(*first_word);  /* [RPAP - Jan 97: Endian Ordering] */
	  last_invf_len = (unsigned long *) (first_word + 1);
	  NTOHUL(*last_invf_len);  /* [RPAP - Jan 97: Endian Ordering] */
	  num_words = (unsigned short *) (last_invf_len + 1);
	  NTOHUS(*num_words);  /* [RPAP - Jan 97: Endian Ordering] */
	  index = num_words + 1;
	  num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;

	  /* [RPAP - Jan 97: Endian Ordering] */
	  for (j = 0; j < num_indexes; j++)
	    NTOHUS(index[j]);
	}
      else
	{
	  first_word = (unsigned long *) (sd->buffer);
	  last_invf_len = (unsigned long *) (first_word + 1);
	  num_words = (unsigned short *) (last_invf_len + 1);
	  index = num_words + 1;
	  num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;
	}
      base = (u_char *) (index + num_indexes);
      
      res = blk_index * sd->sdh.lookback;
      base += index[blk_index];
      
      for (j = 0; j < offset; j++)
	{
	  copy = *base++;
	  suff = *base++;
	  bcopy ((char *) base, (char *) (prev + copy + 1), suff);
	  base += suff;
	  *prev = copy + suff;
	  base += sizeof (unsigned long);   /* skip doc_count */
	  base += sizeof (unsigned long);   /* skip count */
	  base += sizeof (unsigned long);   /* skip invf_ptr */
	  res++;
	}

      for (j = 0; j < num_cases; j++)
	{
	  TermEntry te;
	  
	  if (res >= *num_words)
	    return (-1);
	  copy = *base++;
	  suff = *base++;
	  bcopy ((char *) base, (char *) (prev + copy + 1), suff);
	  base += suff;
	  *prev = copy + suff;
	      
	  te.Word = copy_string (prev);
	  if (!te.Word)
	    FatalError (1, "Could NOT create memory to add term");
	  te.Stem = copy_string (prev);
	  if (!te.Stem)
	    FatalError (1, "Could NOT create memory to add term");
	  stemmer (2, sd->sdh.stemmer_num, te.Stem);

	  te.Count = 1;
	  te.WE.word_num = *first_word + res;
	  bcopy ((char *) base, (char *) &te.WE.doc_count, sizeof (te.WE.doc_count));
	  NTOHUL(te.WE.doc_count);  /* [RPAP - Jan 97: Endian Ordering] */
	  te.WE.max_doc_count = te.WE.doc_count;
	  base += sizeof (te.WE.doc_count);
	      
	  bcopy ((char *) base, (char *) &te.WE.count, sizeof (te.WE.count));
	  NTOHUL(te.WE.count);
	  base += sizeof (te.WE.count);
	      
	  bcopy ((char *) base, (char *) &invfp, sizeof (te.WE.invf_ptr));
	  NTOHUL(invfp);  /* [RPAP - Jan 97: Endian Ordering] */
	  te.WE.invf_ptr = invfp;
	  base += sizeof (te.WE.invf_ptr);
		
	  /* If the current word is the last word of the block the get the 
	     length from last_invf_len */
	  if (res == *num_words - 1)
	    te.WE.invf_len = *last_invf_len;
	  else
	    {
	      unsigned long next_invfp;
	      u_char *oldbase = base;
		  
	      /* Skip over most of the next word to get to the invf_ptr */
	      base++;
	      suff = *base++;
	      base += suff + sizeof (unsigned long) * 2;
	      bcopy ((char *) base, (char *) &next_invfp, sizeof (next_invfp));
	      NTOHUL(next_invfp);  /* [RPAP - Jan 97: Endian Ordering] */
	      te.WE.invf_len = next_invfp - invfp;
	      base = oldbase;
	    }
	      
	  /* Add term entry to term list */
	  /*	  te.query_mask = NULL;*/
	  AddTermEntry (tl, &te);
	      
	  if (res == *num_words - 1 && j + 1 < num_cases)
	    {
	      int ii;
	      /* Read in next block */
	      block++;
	      Fseek (sd->stem_file, sd->pos[block] + sd->sdh.blocks_start, 0);
	      Fread (sd->buffer, sd->sdh.block_size, sizeof (u_char), sd->stem_file);
	      sd->active = sd->pos[block];

	      first_word = (unsigned long *) (sd->buffer);
	      NTOHUL(*first_word);  /* [RPAP - Jan 97: Endian Ordering] */
	      last_invf_len = (unsigned long *) (first_word + 1);
	      NTOHUL(*last_invf_len);  /* [RPAP - Jan 97: Endian Ordering] */
	      num_words = (unsigned short *) (last_invf_len + 1);
	      NTOHUS(*num_words);  /* [RPAP - Jan 97: Endian Ordering] */
	      index = num_words + 1;
	      num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;

	      /* [RPAP - Jan 97: Endian Ordering] */
	      for (ii = 0; ii < num_indexes; ii++)
		NTOHUS(index[ii]);

	      base = (u_char *) (index + num_indexes);
	      base += index[0];
	      res = 0;
	      blk_index = 0;
	    }
	  else
	    res++;
	} /* end for num_cases */
    } /* end for num_entries */
  return (*tl)->num;
}


void 
FreeStemDict (stemmed_dict * sd)
{
  /* [RPAP - Jan 97: Stem Index Change] */
  if (sd->stem1)
    FreeStemIdx (sd->stem1);
  if (sd->stem2)
    FreeStemIdx (sd->stem2);
  if (sd->stem3)
    FreeStemIdx (sd->stem3);

  Xfree (sd->index[0]);
  Xfree (sd->index);
  Xfree (sd->buffer);
  Xfree (sd->pos);
  Xfree (sd);
}

/* [RPAP - Jan 97: Stem Index Change] */
void
FreeStemIdx (stemmed_idx * si)
{
  Xfree (si->index[0]);
  Xfree (si->index);
  Xfree (si->buffer);
  Xfree (si->pos);
  Xfree (si);
}
