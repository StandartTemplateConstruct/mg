/**************************************************************************
 *
 * text.pass2.c -- Text compression (Pass 2)
 * Copyright (C) 1994  Neil Sharman, Gary Eddy and Alistair Moffat
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
 * $Id: text.pass2.c,v 1.3 1994/10/20 03:57:10 tes Exp $
 *
 **************************************************************************/


#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "local_strings.h"
#include "bitio_m_mem.h"
#include "bitio_m.h"
#include "huffman.h"
#include "bitio_stdio.h"
#include "huffman_stdio.h"

#include "mg.h"
#include "mg_files.h"
#include "build.h"
#include "words.h"
#include "text.h"
#include "hash.h"
#include "locallib.h"
#include "comp_dict.h"





/*
   $Log: text.pass2.c,v $
   * Revision 1.3  1994/10/20  03:57:10  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:14  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: text.pass2.c,v 1.3 1994/10/20 03:57:10 tes Exp $";

#define POOL_SIZE 1024*256

typedef struct char_pool
  {
    struct char_pool *next;
    u_long left;
    u_char *ptr;
    u_char pool[POOL_SIZE];
  }
char_pool;

typedef struct novel_hash_rec
  {
    u_long ordinal_num;
    u_char *word;
  }
novel_hash_rec;


#define INITIAL_HASH_SIZE 7927
#define MAX_SWAPS 10000

typedef struct novel_hash_table
  {
    novel_hash_rec *HashTable;
    u_long HashSize, HashUsed;
    char_pool *first_pool;
    char_pool *pool;
    u_long next_num, binary_start;
    novel_hash_rec **code_to_nhr;
  }
novel_hash_table;


static FILE *text, *text_idx;

static u_char *comp_buffer;

static u_long text_length;

static u_long stats_in_tot_bytes = 0;
static u_long stats_in_bytes = 0;
static u_long stats_out_bytes = 0;


static novel_hash_table nht[2];

static u_long prefix_len = 0;

int blk_start[2][33], blk_end[2][33];


static char_pool *
new_pool (char_pool * pool)
{
  char_pool *p = Xmalloc (sizeof (char_pool));
  if (!p)
    FatalError (1, "Unable to allocate memory for pool");
  if (pool)
    pool->next = p;
  p->next = NULL;
  p->left = POOL_SIZE;
  p->ptr = p->pool;
  return p;
}



int 
init_text_2 (char *file_name)
{
  char path[512];
  int i;

  if (LoadCompressionDictionary (make_name (file_name, TEXT_DICT_SUFFIX,
					    path)) == COMPERROR)
    return COMPERROR;

  if (!(text = create_file (file_name, TEXT_SUFFIX, "w+",
			    MAGIC_TEXT, MG_MESSAGE)))
    return COMPERROR;

  bzero ((char *) &cth, sizeof (cth));

  if (fwrite (&cth, sizeof (cth), 1, text) != 1)
    return COMPERROR;

  text_length = sizeof (u_long) + sizeof (cth);

  if (!(text_idx = create_file (file_name, TEXT_IDX_SUFFIX, "w+",
				MAGIC_TEXI, MG_MESSAGE)))
    return COMPERROR;

  if (fwrite (&cth, sizeof (cth), 1, text_idx) != 1)
    return COMPERROR;

  if (!(comp_buffer = Xmalloc (sizeof (u_char) * buf_size)))
    {
      Message ("No memory for compression buffer");
      return (COMPERROR);
    }

#if 0
  MaxMemInUse += sizeof (u_char) * buf_size;
#endif

  if (cdh.novel_method != MG_NOVEL_HUFFMAN_CHARS)
    for (i = 0; i <= 1; i++)
      {
	nht[i].HashSize = INITIAL_HASH_SIZE;
	nht[i].HashTable = Xmalloc (sizeof (novel_hash_rec) * nht[i].HashSize);
	bzero ((char *) nht[i].HashTable,
	       sizeof (novel_hash_rec) * nht[i].HashSize);
	nht[i].HashUsed = 0;
	nht[i].HashSize = INITIAL_HASH_SIZE;
	nht[i].pool = nht[i].first_pool = new_pool (NULL);
	nht[i].next_num = 1;
	nht[i].binary_start = 1;
	if (cdh.novel_method == MG_NOVEL_HYBRID_MTF)
	  nht[i].code_to_nhr = Xmalloc (sizeof (novel_hash_rec *) *
					((nht[i].HashSize >> 1) + 2));
	else
	  nht[i].code_to_nhr = NULL;
	if (cdh.novel_method == MG_NOVEL_HYBRID ||
	    cdh.novel_method == MG_NOVEL_HYBRID_MTF)
	  {
	    int num;
	    num = 1;
	    blk_start[i][0] = 0;
	    blk_end[i][0] = cdh.num_words[i] - 1;
	    while (num < 33)
	      {
		blk_start[i][num] = blk_end[i][num - 1] + 1;
		blk_end[i][num] = blk_start[i][num] +
		  (blk_end[i][num - 1] - blk_start[i][num - 1]) * 2;
		num++;
	      }
	  }
      }

  return (COMPALLOK);
}



int 
ic (void *a, void *b)
{
  return *((int *) a) - *((int *) b);
}



/* #define DOCDUMP 477 */

int 
process_text_2 (u_char * s_in, int l_in)
{
  int which, byte_length;
  u_char *end = s_in + l_in - 1;
  int novels_used[2];
  int swaps[2][MAX_SWAPS];

  which = INAWORD (*s_in);

  ENCODE_START (comp_buffer, buf_size)

    ENCODE_BIT (which);

  if (cdh.novel_method == MG_NOVEL_BINARY)
    {
      DELTA_ENCODE_L (nht[0].binary_start, prefix_len);
      DELTA_ENCODE_L (nht[1].binary_start, prefix_len);
    }

  novels_used[0] = novels_used[1] = 0;

#ifdef DOCDUMP
  if (cth.num_of_docs == DOCDUMP)
    {
      printf ("---------------------------------------------------\n");
      printf ("which = %d\n", which);
    }
#endif

  for (; s_in <= end; which = !which)
    {
      u_char Word[MAXWORDLEN + 1];
      int res;

      if (which)
	cth.num_of_words++;

      /* First parse a word or non-word out of the string */
      if (which)
	PARSE_WORD (Word, s_in, end);
      else
	PARSE_NON_WORD (Word, s_in, end);

#ifdef DOCDUMP
      if (cth.num_of_docs == DOCDUMP)
	{
	  printf ("%sword : \"%.*s\"", which ? "    " : "non-", Word[0], Word + 1);
	}
#endif

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
	  if (cdh.dict_type == MG_COMPLETE_DICTIONARY)
	    {
	      Message ("Unknown word \"%.*s\"\n", *Word, Word + 1);
	      return (COMPERROR);
	    }
	  if (cdh.dict_type == MG_PARTIAL_DICTIONARY)
	    {
	      u_long i;
	      if (ht[which])
		{
		  res = ht[which]->hd->num_codes - 1;
		  HUFF_ENCODE (res, ht[which]->codes, ht[which]->hd->clens);
		}
	      HUFF_ENCODE (Word[0], lens_codes[which], lens_huff[which].clens);
	      for (i = 0; i < Word[0]; i++)
		HUFF_ENCODE (Word[i + 1], char_codes[which],
			     char_huff[which].clens);
	    }
	  if (cdh.dict_type == MG_SEED_DICTIONARY)
	    {
	      if (ht[which])
		{
		  res = ht[which]->hd->num_codes - 1;
		  HUFF_ENCODE (res, ht[which]->codes, ht[which]->hd->clens);
		}
	      switch (cdh.novel_method)
		{
		case MG_NOVEL_HUFFMAN_CHARS:
		  {
		    u_long i;
		    HUFF_ENCODE (Word[0], lens_codes[which],
				 lens_huff[which].clens);
		    for (i = 0; i < Word[0]; i++)
		      HUFF_ENCODE (Word[i + 1], char_codes[which],
				   char_huff[which].clens);
		  }
		  break;
		case MG_NOVEL_BINARY:
		case MG_NOVEL_DELTA:
		case MG_NOVEL_HYBRID:
		case MG_NOVEL_HYBRID_MTF:
		  {
		    register unsigned long hashval, step;
		    register novel_hash_table *h = &nht[which];
		    register int hsize = h->HashSize;
		    register novel_hash_rec *ent;
		    HASH (hashval, step, Word, hsize);
		    for (;;)
		      {
			register u_char *s1, *s2;
			register int len;
			ent = h->HashTable + hashval;
			if (!ent->word)
			  {
			    int len = *Word + 1;
			    if (len > h->pool->left)
			      h->pool = new_pool (h->pool);
			    ent->word = h->pool->ptr;
			    ent->ordinal_num = h->next_num++;
			    if (cdh.novel_method == MG_NOVEL_HYBRID_MTF)
			      h->code_to_nhr[ent->ordinal_num - 1] = ent;
			    memcpy (h->pool->ptr, Word, len);
			    h->pool->ptr += len;
			    h->pool->left -= len;
			    h->HashUsed++;
			    break;
			  }
			/* Compare the words */
			s1 = Word;
			s2 = ent->word;
			len = *s1 + 1;
			for (; len; len--)
			  if (*s1++ != *s2++)
			    break;

			if (!len)
			  break;

			hashval = (hashval + step);
			if (hashval >= hsize)
			  hashval -= hsize;
		      }

		    switch (cdh.novel_method)
		      {
		      case MG_NOVEL_BINARY:
			{
			  BINARY_ENCODE (ent->ordinal_num, h->binary_start);
			  if (ent->ordinal_num == h->binary_start)
			    h->binary_start++;
			}
			break;
		      case MG_NOVEL_DELTA:
			{
			  DELTA_ENCODE (ent->ordinal_num);
			}
			break;
		      case MG_NOVEL_HYBRID:
			{
			  int k = 0;
			  int j = ent->ordinal_num - 1;
			  while (j > blk_end[which][k])
			    k++;
			  assert (j - blk_start[which][k] + 1 >= 1 &&
				  j - blk_start[which][k] + 1 <=
			       blk_end[which][k] - blk_start[which][k] + 1);

			  GAMMA_ENCODE (k + 1);
			  BINARY_ENCODE (j - blk_start[which][k] + 1,
					 blk_end[which][k] -
					 blk_start[which][k] + 1);
			}
			break;
		      case MG_NOVEL_HYBRID_MTF:
			{
			  int k = 0;
			  int j = ent->ordinal_num - 1;
			  while (j > blk_end[which][k])
			    k++;
			  assert (j - blk_start[which][k] + 1 >= 1 &&
				  j - blk_start[which][k] + 1 <=
			       blk_end[which][k] - blk_start[which][k] + 1);
			  GAMMA_ENCODE (k + 1);
			  BINARY_ENCODE (j - blk_start[which][k] + 1,
					 blk_end[which][k] -
					 blk_start[which][k] + 1);

			  if (ent->ordinal_num - 1 >= novels_used[which])
			    {
			      int a = novels_used[which];
			      int b = ent->ordinal_num - 1;
			      novel_hash_rec *temp;


/*                            fprintf(stderr, "a = %d , b = %d\n", a, b);
 */
			      temp = h->code_to_nhr[a];
			      h->code_to_nhr[a] = h->code_to_nhr[b];
			      h->code_to_nhr[b] = temp;
			      h->code_to_nhr[a]->ordinal_num = a + 1;
			      h->code_to_nhr[b]->ordinal_num = b + 1;
			      if (novels_used[which] == MAX_SWAPS)
				FatalError (1, "Not enough mem for swapping");
			      swaps[which][novels_used[which]] = b;
			      novels_used[which]++;
			    }
			}
			break;
		      }
		    if (h->HashUsed >= h->HashSize >> 1)
		      {
			novel_hash_rec *ht;
			unsigned long size;
			unsigned long i;
			size = prime (h->HashSize * 2);
			if (cdh.novel_method == MG_NOVEL_HYBRID_MTF)
			  {
			    Xfree (h->code_to_nhr);
			    h->code_to_nhr = Xmalloc (sizeof (novel_hash_rec *) *
						      ((size >> 1) + 2));
			  }
			if (!(ht = Xmalloc (sizeof (novel_hash_rec) * size)))
			  {
			    Message ("Unable to allocate memory for table");
			    return (COMPERROR);
			  }
			bzero ((char *) ht, sizeof (novel_hash_rec) * size);

			for (i = 0; i < h->HashSize; i++)
			  if (h->HashTable[i].word)
			    {
			      register u_char *wptr;
			      register unsigned long hashval, step;

			      wptr = h->HashTable[i].word;
			      HASH (hashval, step, wptr, size);
			      wptr = (ht + hashval)->word;
			      while (wptr)
				{
				  hashval += step;
				  if (hashval >= size)
				    hashval -= size;
				  wptr = (ht + hashval)->word;
				}
			      ht[hashval] = h->HashTable[i];
			      if (cdh.novel_method == MG_NOVEL_HYBRID_MTF)
				h->code_to_nhr[ht[hashval].ordinal_num - 1] =
				  &ht[hashval];
			    }
			Xfree (h->HashTable);
			h->HashTable = ht;
			h->HashSize = size;
		      }
		  }
		  break;
		}
	    }
	}
      else
	{
	  HUFF_ENCODE (res, ht[which]->codes, ht[which]->hd->clens);
#ifdef DOCDUMP
	  if (cth.num_of_docs == DOCDUMP)
	    {
	      printf ("   %d %d\n", ht[which]->hd->clens[res],
		      ht[which]->codes[res]);
	    }
#endif
	}
    }


  /* Add a 1 bit onto the end of the buffer the remaining bits in the last
     byte will all be zero */

  ENCODE_BIT (1);

  ENCODE_FLUSH;

  byte_length = __pos - __base;
  if (!__remaining)
    {
      Message ("The end of the buffer was probably overrun");
      return COMPERROR;
    }

  ENCODE_DONE

#ifdef DOCDUMP
    if (cth.num_of_docs == DOCDUMP)
    {
      printf ("unused bits = %d\n", bits_unused);
    }
#endif

  fwrite (&text_length, sizeof (text_length), 1, text_idx);
  text_length += byte_length;

#ifdef DOCDUMP
  if (cth.num_of_docs == DOCDUMP)
    {
      int i;
      for (i = 0; i < byte_length; i++)
	printf ("%02x ", comp_buffer[i]);
      printf ("\n");
    }
#endif

  if (cdh.novel_method == MG_NOVEL_HYBRID_MTF)
    for (which = 0; which <= 1; which++)
      for (novels_used[which]--; novels_used[which] >= 0; novels_used[which]--)
	{
	  int a = novels_used[which];
	  int b = swaps[which][novels_used[which]];
	  novel_hash_rec *temp;
	  temp = nht[which].code_to_nhr[a];
	  nht[which].code_to_nhr[a] = nht[which].code_to_nhr[b];
	  nht[which].code_to_nhr[b] = temp;
	  nht[which].code_to_nhr[a]->ordinal_num = a + 1;
	  nht[which].code_to_nhr[b]->ordinal_num = b + 1;
	}


  fwrite (comp_buffer, sizeof (*comp_buffer), byte_length, text);

  if ((double) l_in / (double) byte_length > cth.ratio)
    cth.ratio = (double) l_in / (double) byte_length;

  cth.num_of_docs++;
  if (l_in > cth.length_of_longest_doc)
    cth.length_of_longest_doc = l_in;

  cth.num_of_bytes += l_in;

  if (Comp_Stats)
    {
      stats_in_tot_bytes += l_in;
      stats_in_bytes += l_in;
      stats_out_bytes += byte_length;
      if (stats_in_bytes >= comp_stat_point)
	{
	  fprintf (Comp_Stats, "%ld %ld %ld %f\n", stats_in_tot_bytes,
		   stats_in_bytes, stats_out_bytes,
		   (double) stats_out_bytes / (double) stats_in_bytes);
	  stats_in_bytes = 0;
	  stats_out_bytes = 0;
	}
    }

  return COMPALLOK;
}






int 
write_aux_dict (char *FileName)
{
  int i;
  FILE *aux;
  if (!(aux = create_file (FileName, TEXT_DICT_AUX_SUFFIX, "w",
			   MAGIC_AUX_DICT, MG_MESSAGE)))
    return COMPERROR;

  for (i = 0; i <= 1; i++)
    {
      aux_frags_header afh;
      char_pool *cp;

      afh.num_frags = nht[i].HashUsed;
      afh.mem_for_frags = 0;
      for (cp = nht[i].first_pool; cp; cp = cp->next)
	afh.mem_for_frags += POOL_SIZE - cp->left;

      fwrite (&afh, sizeof (afh), 1, aux);

      for (cp = nht[i].first_pool; cp; cp = cp->next)
	fwrite (cp->pool, POOL_SIZE - cp->left, sizeof (u_char), aux);
    }
  fclose (aux);
  return COMPALLOK;
}


void 
estimate_compressed_aux_dict (void)
{
  int i;
  u_long aux_compressed = 0, total_uncomp = 0;
  for (i = 0; i <= 1; i++)
    {
      int j;
      long chars[256], fchars[256];
      long lens[16], flens[16];
      char_pool *cp;
      bzero ((char *) chars, sizeof (chars));
      bzero ((char *) lens, sizeof (lens));
      for (cp = nht[i].first_pool; cp; cp = cp->next)
	{
	  u_char *buf = cp->pool;
	  while (buf != cp->ptr)
	    {
	      int len = *buf++;
	      lens[len]++;
	      total_uncomp += len + 4;
	      for (; len; len--)
		chars[*buf++]++;
	    }
	}
      for (j = 0; j < 256; j++)
	if (!chars[j] && INAWORD (j) == i)
	  fchars[j] = 1;
	else
	  fchars[j] = chars[j];
      for (j = 0; j < 16; j++)
	if (!lens[j])
	  flens[j] = 1;
	else
	  flens[j] = lens[j];

      aux_compressed += (Calculate_Huffman_Size (16, flens, lens) +
			 Calculate_Huffman_Size (256, fchars, chars)) / 8;

    }

  Message ("Aux dictionary (Uncompressed) %.2f Mb ( %u bytes   %0.3f %%)",
	   total_uncomp / 1024.0 / 1024, total_uncomp,
	   (total_uncomp * 100.0) / bytes_processed);
  Message ("Aux dictionary (Compressed)   %.2f Mb ( %.0f bytes   %0.3f %%)",
	   aux_compressed / 1024.0 / 1024, aux_compressed * 1.0,
	   (aux_compressed * 100.0) / bytes_processed);
}






int 
done_text_2 (char *FileName)
{
  if (Comp_Stats)
    fprintf (Comp_Stats, "%ld %ld %ld %f\n", stats_in_tot_bytes,
	     stats_in_bytes, stats_out_bytes,
	     (double) stats_out_bytes / (double) stats_in_bytes);

  fwrite (&text_length, sizeof (text_length), 1, text_idx);
  if (fseek (text_idx, sizeof (u_long), SEEK_SET) == -1 ||
      fwrite (&cth, sizeof (cth), 1, text_idx) != 1)
    return COMPERROR;
  fclose (text_idx);

  if (fseek (text, sizeof (u_long), SEEK_SET) == -1 ||
      fwrite (&cth, sizeof (cth), 1, text) != 1)
    return COMPERROR;
  fclose (text);

  Message ("Compressed Text %.2f Mb ( %u bytes   %0.3f %%)",
	   text_length / 1024.0 / 1024.0, text_length,
	   (text_length * 100.0) / bytes_processed);
  Message ("Words portion of the dictionary %.2f Mb ( %.0f bytes   %0.3f %%)",
	   Words_disk / 1024.0 / 1024, Words_disk * 1.0,
	   (Words_disk * 100.0) / bytes_processed);

  if (cdh.dict_type != MG_COMPLETE_DICTIONARY &&
      (cdh.novel_method == MG_NOVEL_BINARY ||
       cdh.novel_method == MG_NOVEL_DELTA ||
       cdh.novel_method == MG_NOVEL_HYBRID ||
       cdh.novel_method == MG_NOVEL_HYBRID_MTF))
    {
      if (write_aux_dict (FileName) == COMPERROR)
	return COMPERROR;
      estimate_compressed_aux_dict ();
    }
  else
    {
      if (cdh.dict_type != MG_COMPLETE_DICTIONARY)
	Message ("Huffman info for chars in dictionary %.2f Mb"
		 " ( %u bytes   %0.3f %%)",
		 Chars_disk / 1024.0 / 1024, Chars_disk,
		 (Chars_disk * 100.0) / bytes_processed);
      unlink (make_name (FileName, TEXT_DICT_AUX_SUFFIX, NULL));
    }

  return (COMPALLOK);
}
