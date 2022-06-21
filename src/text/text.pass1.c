/**************************************************************************
 *
 * text.pass1.c -- Text compression (Pass 1)
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
 * $Id: text.pass1.c,v 1.4 1994/11/25 03:47:47 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "huffman.h"


#include "mg_files.h"
#include "mg.h"
#include "build.h"
#include "locallib.h"
#include "words.h"
#include "text.h"
#include "hash.h"
#include "local_strings.h"


/*
   $Log: text.pass1.c,v $
   * Revision 1.4  1994/11/25  03:47:47  tes
   * Committing files before adding the merge stuff.
   *
   * Revision 1.3  1994/10/20  03:57:09  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:13  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: text.pass1.c,v 1.4 1994/11/25 03:47:47 tes Exp $";


#define POOL_SIZE 1024*1024
#define INITIAL_HASH_SIZE 7927







typedef struct hash_rec
  {
    unsigned long wcnt;		/* word frequency */
    unsigned long occurance_num;
    u_char *word;
  }
hash_rec;

typedef struct dict_data
  {
    hash_rec *HashTable;
    unsigned long HashSize;
    unsigned long HashUsed;
    unsigned long wordnum;
    unsigned long words_read;
    unsigned long bytes_diff;
    huff_data hd;
  }
dict_data;



static unsigned long LongestDoc = 0;
static unsigned long occurance_num = 0;
static dict_data DictData[2];

static u_char *Pool;
static int PoolLeft;
static unsigned long inputbytes = 0;
static unsigned long MaxMemInUse = 0;
static unsigned long MemInUse = 0;
static compression_stats_header csh =
{0, 0};


static void 
ChangeMem (int Change)
{
  MemInUse += Change;
  if (MemInUse > MaxMemInUse)
    MaxMemInUse = MemInUse;
}




int 
init_text_1 (char *FileName)
{
  int which;

  if (!(Pool = Xmalloc (POOL_SIZE)))
    {
      Message ("Unable to allocate memory for pool");
      return (COMPERROR);
    }
  PoolLeft = POOL_SIZE;
  ChangeMem (POOL_SIZE);

  for (which = 1; which >= 0; which--)
    {
      u_char *word;
      hash_rec *ent;
      dict_data *dd = &DictData[which];

      dd->wordnum = 0;
      dd->words_read = 0;
      dd->bytes_diff = 0;
      dd->HashSize = INITIAL_HASH_SIZE;
      dd->HashUsed = 0;

      if (!(dd->HashTable = Xmalloc (sizeof (hash_rec) * dd->HashSize)))
	{
	  Message ("Unable to allocate memory for table");
	  return (COMPERROR);
	}
      ChangeMem (sizeof (hash_rec) * dd->HashSize);
      bzero ((char *) (dd->HashTable), sizeof (hash_rec) * dd->HashSize);

      word = Pool;
      *Pool++ = '\0';
      PoolLeft--;
      {
	register u_char *wptr;
	register int hsize = dd->HashSize;
	register unsigned long hashval, step;

	HASH (hashval, step, word, hsize);
	wptr = (dd->HashTable + hashval)->word;
	while (wptr)
	  {
	    hashval += step;
	    if (hashval >= hsize)
	      hashval -= hsize;
	    wptr = (dd->HashTable + hashval)->word;
	  }
	ent = dd->HashTable + hashval;
      }
      ent->wcnt = 1;
      ent->word = word;
      dd->HashUsed = 1;
    }
  return (COMPALLOK);
}




int 
process_text_1 (u_char * s_in, int l_in)
{
  int which;
  u_char *end = s_in + l_in - 1;

  if (l_in > LongestDoc)
    LongestDoc = l_in;

  csh.num_docs++;
  csh.num_bytes += l_in;

  which = INAWORD (*s_in);
  /*
   ** Alternately parse off words and non-words from the input
   ** stream beginning with a non-word. Each token is then
   ** inserted into the set if it does not exist or has it's
   ** frequency count incremented if it does.
   */
  for (; s_in <= end; which = !which)
    {
      u_char Word[MAXWORDLEN + 1];
      dict_data *dd = &DictData[which];

      /* First parse a word or non-word out of the string */
      if (which)
	PARSE_WORD (Word, s_in, end);
      else
	PARSE_NON_WORD (Word, s_in, end);

      dd->wordnum++;
      inputbytes += *Word;
      dd->words_read++;

      /* Search the hash table for Word */
      {
	register unsigned long hashval, step;
	register int hsize = dd->HashSize;
	HASH (hashval, step, Word, hsize);
	for (;;)
	  {
	    register u_char *s1;
	    register u_char *s2;
	    register int len;
	    register hash_rec *ent;
	    ent = dd->HashTable + hashval;
	    if (!ent->word)
	      {
		int len = *Word + 1;
		if (len > PoolLeft)
		  {
		    if (!(Pool = Xmalloc (POOL_SIZE)))
		      {
			Message ("Unable to allocate memory for pool");
			return (COMPERROR);
		      }
		    PoolLeft = POOL_SIZE;
		    ChangeMem (POOL_SIZE);
		  }
		ent->occurance_num = occurance_num++;
		ent->wcnt = 1;
		ent->word = Pool;
		memcpy (Pool, Word, len);
		Pool += len;
		PoolLeft -= len;
		dd->HashUsed++;
		dd->bytes_diff += Word[0];
		break;
	      }

	    /* Compare the words */
	    s1 = Word;
	    s2 = ent->word;
	    len = *s1 + 1;
	    for (; len; len--)
	      if (*s1++ != *s2++)
		break;

	    if (len)
	      {
		hashval = (hashval + step);
		if (hashval >= hsize)
		  hashval -= hsize;
	      }
	    else
	      {
		ent->wcnt++;
		break;
	      }
	  }
      }


      if (dd->HashUsed >= dd->HashSize >> 1)
	{
	  hash_rec *ht;
	  unsigned long size;
	  unsigned long i;
	  size = prime (dd->HashSize * 2);
	  if (!(ht = Xmalloc (sizeof (hash_rec) * size)))
	    {
	      Message ("Unable to allocate memory for table");
	      return (COMPERROR);
	    }
	  ChangeMem (sizeof (hash_rec) * size);
	  bzero ((char *) ht, sizeof (hash_rec) * size);

	  for (i = 0; i < dd->HashSize; i++)
	    if (dd->HashTable[i].word)
	      {
		register u_char *wptr;
		register unsigned long hashval, step;

		wptr = dd->HashTable[i].word;
		HASH (hashval, step, wptr, size);
		wptr = (ht + hashval)->word;
		while (wptr)
		  {
		    hashval += step;
		    if (hashval >= size)
		      hashval -= size;
		    wptr = (ht + hashval)->word;
		  }
		ht[hashval] = dd->HashTable[i];
	      }
	  Xfree (dd->HashTable);
	  ChangeMem (-sizeof (hash_rec) * dd->HashSize);
	  dd->HashTable = ht;
	  dd->HashSize = size;


	}
    }
  return (COMPALLOK);
}				/* encode */



static int 
PackHashTable (dict_data * dd)
{
  int s, d;
  for (s = d = 0; s < dd->HashSize; s++)
    if (dd->HashTable[s].word)
      dd->HashTable[d++] = dd->HashTable[s];
  ChangeMem (-sizeof (hash_rec) * dd->HashSize);
  ChangeMem (sizeof (hash_rec) * dd->HashUsed);
  if (!(dd->HashTable = Xrealloc (dd->HashTable,
				  sizeof (hash_rec) * dd->HashUsed)))
    {
      Message ("Out of memory");
      return COMPERROR;
    }
  dd->HashSize = dd->HashUsed;
  return COMPALLOK;
}





static int 
ent_comp (const void *s1, const void *s2)
{
  return compare (((hash_rec *) s1)->word, ((hash_rec *) s2)->word);
}



static void 
WriteHashTable (FILE * fp, dict_data * dd)
{
  frags_stats_header fsh;
  u_long j = 0;
  u_char *curr;

  if (PackHashTable (dd) == COMPERROR)
    return;

  qsort (dd->HashTable, dd->HashUsed, sizeof (hash_rec), ent_comp);

  fsh.num_frags = dd->HashSize;
  fsh.mem_for_frags = dd->HashSize;
  for (j = 0; j < dd->HashSize; j++)
    fsh.mem_for_frags += dd->HashTable[j].word[0];

  fwrite (&fsh, sizeof (fsh), 1, fp);

  for (j = 0; j < dd->HashSize; j++)
    {
      curr = dd->HashTable[j].word;
      fwrite (&dd->HashTable[j].wcnt, sizeof (dd->HashTable[j].wcnt), 1, fp);
      fwrite (&dd->HashTable[j].occurance_num,
	      sizeof (dd->HashTable[j].occurance_num), 1, fp);
      fwrite (curr, sizeof (u_char), curr[0] + 1, fp);
    }
}


int 
done_text_1 (char *file_name)
{
  char *temp_str;
  FILE *fp;

  if (!(fp = create_file (file_name, TEXT_STATS_DICT_SUFFIX, "w",
			  MAGIC_STATS_DICT, MG_MESSAGE)))
    return COMPERROR;

  temp_str = msg_prefix;
  msg_prefix = "text.pass1";

  fwrite (&csh, sizeof (csh), 1, fp);
  WriteHashTable (fp, &DictData[0]);
  WriteHashTable (fp, &DictData[1]);
  msg_prefix = temp_str;
  return COMPALLOK;
}				/* done_encode */
