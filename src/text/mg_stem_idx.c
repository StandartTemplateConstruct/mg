/**************************************************************************
 *
 * mg_stem_idx.c -- Memory efficient stem index builder
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

#include "memlib.h"
#include "messages.h"
#include "filestats.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg_files.h"
#include "invf.h"
#include "mg.h"
#include "locallib.h"
#include "backend.h"     /* For struct stemmed_dict */
#include "words.h"
#include "stemmer.h"
#include "hash.h"
#include "local_strings.h"

typedef struct PosEntry
{
  unsigned int num_cases;
  unsigned int blk;
  unsigned short blk_index;
  unsigned short offset;
}
PosEntry;


typedef struct PosList
{
  unsigned int list_size;
  unsigned int num_entries;
  PosEntry PE[1];
}
PosList;


typedef struct idx_hash_rec
{
  u_char *word;
  PosList *PL;
}
idx_hash_rec;


#define POOL_SIZE 1024*1024
#define HASH_POOL_SIZE 8192
#define INITIAL_HASH_SIZE 7927


static unsigned long MaxMemInUse = 0;
static unsigned long MemInUse = 0;

static idx_hash_rec **IdxHashTable;
static unsigned long IdxHashSize;
static unsigned long IdxHashUsed;
static u_char *IdxPool;
static int IdxPoolLeft;

static idx_hash_rec *idx_hr_pool;
static int idx_hr_PoolLeft;

static idx_hash_rec **idx_first_occr;
static int idx_max_first_occr;

int block_size = 1024 * 4;

int force = 0;

static long lookback = 4;

static void 
ChangeMem (int Change)
{
  MemInUse += Change;
  if (MemInUse > MaxMemInUse)
    MaxMemInUse = MemInUse;
}


/* =========================================================================
 * Function: MakePosList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */
PosList *
MakePosList (int n)
{
  PosList *pl;
  int list_size = (n == 0 ? 1 : n);	/* always allocate at least one node */

  pl = Xmalloc (sizeof (PosList) + (list_size - 1) * sizeof (PosEntry));
  if (!pl)
    FatalError (1, "Unable to allocate term list");
  ChangeMem (sizeof (PosList) + (list_size - 1) * sizeof (PosEntry));

  pl->num_entries = n;
  pl->list_size = list_size;

  return pl;
}

/* =========================================================================
 * Function: ResizePosList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */

#define GROWTH_FACTOR 2
#define MIN_SIZE 2

static void
ResizePosList (PosList ** pos_list)
{
  PosList *pl = *pos_list;

  ChangeMem (-(sizeof (PosList) + (pl->list_size - 1) * sizeof (PosEntry)));
  if (pl->num_entries > pl->list_size)
    {
      if (pl->list_size)
	pl->list_size *= GROWTH_FACTOR;
      else
	pl->list_size = MIN_SIZE;
    }
  pl = Xrealloc (pl, sizeof (PosList) + (pl->list_size - 1) * sizeof (PosEntry));

  if (!pl)
    FatalError (1, "Unable to resize pos list");
  ChangeMem (sizeof (PosList) + (pl->list_size - 1) * sizeof (PosEntry));

  *pos_list = pl;
}


/* =========================================================================
 * Function: FreePosList
 * Description:
 * Input:
 * Output:
 * ========================================================================= */

void
FreePosList (PosList ** the_pl)
{
  PosList *pl = *the_pl;

  ChangeMem (-(sizeof(PosList) + sizeof (PosEntry) * (pl->list_size - 1)));
  Xfree (pl);

  *the_pl = NULL;
}


/* =========================================================================
 * Function: ResetPosList
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

void
ResetPosList (PosList ** pl)
{
  if (*pl)
    FreePosList (pl);
  *pl = MakePosList (0);
}

/* =========================================================================
 * Function: AddPosEntry
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

int
AddPosEntry (PosList ** pos_list, PosEntry * pe)
{
  PosList *pl = *pos_list;

  pl->num_entries++;
  ResizePosList (pos_list);
  pl = *pos_list;

  /* copy the structure contents */
  bcopy ((char *) pe, (char *) &(pl->PE[pl->num_entries - 1]), sizeof (PosEntry));

  return pl->num_entries - 1;
}


/* Modified from stem_search.c */
stemmed_dict *
ReadStemDictBlk (File * stem_file)
{
  unsigned long i;
  stemmed_dict *sd;
  u_char *buffer;

  if (!(sd = Xmalloc (sizeof (stemmed_dict))))
    FatalError (1, "Could not allocate memory for stemmed dict");


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
  NTOHUL(sd->sdh.stemmer_num);
  NTOHUL(sd->sdh.stem_method);
  NTOHUL(sd->sdh.indexed);

  if (!(buffer = Xmalloc (sd->sdh.index_chars)))
    {
      Xfree (sd);
      FatalError (1, "Could not allocate memory for stemmed dict");
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.index_chars;

  if (!(sd->index = Xmalloc (sd->sdh.num_blocks * sizeof (*sd->index))))
    {
      Xfree (sd);
      Xfree (buffer);
      FatalError (1, "Could not allocate memory for stemmed dict");
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.num_blocks * sizeof (*sd->index);

  if (!(sd->pos = Xmalloc (sd->sdh.num_blocks * sizeof (*sd->pos))))
    {
      Xfree (sd);
      Xfree (buffer);
      Xfree (sd->index);
      FatalError (1, "Could not allocate memory for stemmed dict");
      return (NULL);
    };
  sd->MemForStemDict += sd->sdh.num_blocks * sizeof (*sd->pos);

  if (!(sd->buffer = Xmalloc (sd->sdh.block_size * sizeof (*sd->buffer))))
    {
      Xfree (sd);
      Xfree (buffer);
      Xfree (sd->index);
      Xfree (sd->buffer);
      FatalError (1, "Could not allocate memory for stemmed dict");
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
      NTOHUL(sd->pos[i]);  /* [RPAP - Jan 97: Endian Ordering] */
    }

  return sd;
}


void
mg_init_process ()
{
  /* Allocate memory for idx hash table */

  if (!(IdxPool = Xmalloc (POOL_SIZE)))
    FatalError (1, "Unable to allocate memory for idx pool");
  IdxPoolLeft = POOL_SIZE;
  ChangeMem (POOL_SIZE);

  if (!(idx_hr_pool = Xmalloc (HASH_POOL_SIZE * sizeof (idx_hash_rec))))
    FatalError (1, "Unable to allocate memory for idx pool");
  idx_hr_PoolLeft = HASH_POOL_SIZE;
  ChangeMem (HASH_POOL_SIZE * sizeof (idx_hash_rec));

  IdxHashSize = INITIAL_HASH_SIZE;
  IdxHashUsed = 0;
  if (!(IdxHashTable = Xmalloc (sizeof (idx_hash_rec *) * IdxHashSize)))
    FatalError (1, "Unable to allocate memory for idx table");
  ChangeMem (sizeof (idx_hash_rec *) * IdxHashSize);
  bzero ((char *) IdxHashTable, sizeof (idx_hash_rec *) * IdxHashSize);
  idx_max_first_occr = 8192;
  if (!(idx_first_occr = Xmalloc (sizeof (idx_hash_rec *) * idx_max_first_occr)))
    FatalError (1, "Unable to allocate memory for idx_first_occur");
  ChangeMem (sizeof (idx_hash_rec *) * idx_max_first_occr);
}


void
PackIdxHashTable (void)
{
  int s, d;
  for (s = d = 0; s < IdxHashSize; s++)
    if (IdxHashTable[s])
      IdxHashTable[d++] = IdxHashTable[s];
  ChangeMem (-sizeof (idx_hash_rec *) * IdxHashSize);
  ChangeMem (sizeof (idx_hash_rec *) * IdxHashUsed);
  if (!(IdxHashTable = Xrealloc (IdxHashTable, sizeof (idx_hash_rec *) * IdxHashUsed)))
    FatalError (1, "Out of memory");
  IdxHashSize = IdxHashUsed;
}


void
process_stem_dict (stemmed_dict * sd, int stem_method)
{
  int block;
  short blk_index = -1;
  int wordnum = -1;
  PosEntry *prevPE = NULL;
  idx_hash_rec *prevIdx = NULL;
  u_char word[MAXSTEMLEN + 1];
  u_char prev[MAXSTEMLEN + 1];

  /* For each block in stem dict... */
  for (block = 0; block < sd->sdh.num_blocks; block++)
    {
      register unsigned int res;
      int num_indexes;
      unsigned long *first_word, *last_invf_len;
      unsigned short *num_words;
      u_char *base;
      unsigned short *index;

      /* Read block into buffer */
      Fseek (sd->stem_file, sd->pos[block] + sd->sdh.blocks_start, 0);
      Fread (sd->buffer, sd->sdh.block_size, sizeof (u_char), sd->stem_file);
      sd->active = sd->pos[block];

      /* Move through block header */
      first_word = (unsigned long *) (sd->buffer);
      NTOHUL(*first_word);  /* [RPAP - Jan 97: Endian Ordering] */
      last_invf_len = (unsigned long *) (first_word + 1);
      NTOHUL(*last_invf_len);  /* [RPAP - Jan 97: Endian Ordering] */
      num_words = (unsigned short *) (last_invf_len + 1);
      NTOHUS(*num_words);  /* [RPAP - Jan 97: Endian Ordering] */
      index = num_words + 1;
      num_indexes = ((*num_words - 1) / sd->sdh.lookback) + 1;

      {
	/* [RPAP - Jan 97: Endian Ordering] */
	int i;
	for (i = 0; i < num_indexes; i++)
	  NTOHUS(index[i]);
      }

      base = (u_char *) (index + num_indexes);
      blk_index = -1;

      /* For each word in block... */
      for (res = 0; res < *num_words; res++)
	{
	  unsigned copy, suff;
	  register idx_hash_rec *idx_ent = 0;

	  /* Update blk_index */
	  if (!(res % sd->sdh.lookback))
	    blk_index++;

	  copy = *base++;
	  suff = *base++;
	  bcopy ((char *) base, (char *) (prev + copy + 1), suff);
	  base += suff;
	  *prev = copy + suff;

	  /* Skip irrelevant word info */
	  base += sizeof (u_long);
	  base += sizeof (u_long);
	  base += sizeof (u_long);

	  /* Stem word */
	  bcopy ((char *) prev, (char *) word, *prev + 1);
	  stemmer (stem_method, sd->sdh.stemmer_num, word);

	  /* Check if word follows straight on from previous word */
	  if (prevIdx && !compare (word, prevIdx->word))
	    prevPE->num_cases++;

	  else 
	    {
	      /* Search the idx hash table for word */
	      register unsigned long hashval, step;
	      register int hsize = IdxHashSize;
	      HASH (hashval, step, word, hsize);
	      for (;;)
		{
		  register u_char *s1;
		  register u_char *s2;
		  register int len;
		  idx_ent = IdxHashTable[hashval];
		  if (!idx_ent)
		    {
		      /* Create a next entry in the hash table */
		      int len = *word + 1;
		      if (!idx_hr_PoolLeft)
			{
			  if (!(idx_hr_pool = Xmalloc (HASH_POOL_SIZE *
						       sizeof (idx_hash_rec))))
			    FatalError (1, "Unable to allocate memory for pool");
			  idx_hr_PoolLeft = HASH_POOL_SIZE;
			  ChangeMem (HASH_POOL_SIZE * sizeof (idx_hash_rec));
			}
		      idx_ent = idx_hr_pool++;
		      idx_hr_PoolLeft--;
		      if (len > IdxPoolLeft)
			{
			  if (!(IdxPool = Xmalloc (POOL_SIZE)))
			    FatalError (1, "Unable to allocate memory for pool");
			  IdxPoolLeft = POOL_SIZE;
			  ChangeMem (POOL_SIZE);
			}
		      wordnum++;

		      idx_ent->word = IdxPool;
		      idx_ent->PL = MakePosList (0);
		      {
			PosEntry PE;
			PE.num_cases = 1;
			PE.blk = block;
			PE.blk_index = (unsigned short) blk_index;
			PE.offset = res % sd->sdh.lookback;
			AddPosEntry (&(idx_ent->PL), &PE);
		      }
		      prevIdx = idx_ent;
		      prevPE = &(idx_ent->PL->PE[idx_ent->PL->num_entries - 1]);

		      bcopy ((char *) word, (char *) IdxPool, len);
		      IdxPool += len;
		      IdxPoolLeft -= len;
		      if (IdxHashUsed == idx_max_first_occr - 1)
			{
			  ChangeMem (-sizeof (idx_hash_rec *) * idx_max_first_occr);
			  idx_max_first_occr *= 2;
			  if (!(idx_first_occr = Xrealloc (idx_first_occr, sizeof (idx_hash_rec *) *
							   idx_max_first_occr)))
			    FatalError (1, "Unable to allocate memory for idx_first_occr");
			  ChangeMem (sizeof (idx_hash_rec *) * idx_max_first_occr);
			}
		      idx_first_occr[IdxHashUsed] = idx_ent;
		      IdxHashUsed++;
		      IdxHashTable[hashval] = idx_ent;
		      break;
		    } /* if (!idx_ent) */
		  
		  /* Compare the words */
		  s1 = word;
		  s2 = idx_ent->word;
		  len = *s1 + 1;
		  for (; len; len--)
		    if (*s1++ != *s2++)
		      break;
		  
		  if (len)
		    {
		      /* Entry is not the right one - move to next hash index */
		      hashval = (hashval + step);
		      if (hashval >= hsize)
			hashval -= hsize;
		    }
		  else
		    {
		      /* Entry is correct - added PosEntry to word */
		      PosEntry PE;
		      PE.num_cases = 1;
		      PE.blk = block;
		      PE.blk_index = (unsigned short) blk_index;
		      PE.offset = res % sd->sdh.lookback;
		      AddPosEntry (&(idx_ent->PL), &PE);
		      prevIdx = idx_ent;
		      prevPE = &(idx_ent->PL->PE[idx_ent->PL->num_entries - 1]);
		      break;
		    }
		} /* for (;;) */
	    } /* else (the word does not follow on from previous one) */

	  if (IdxHashUsed >= IdxHashSize >> 1)
	    {
	      idx_hash_rec **ht;
	      unsigned long size;
	      unsigned long i;
	      size = prime (IdxHashSize * 2);
	      if (!(ht = Xmalloc (sizeof (idx_hash_rec *) * size)))
		FatalError (1, "Unable to allocate memory for idx table");
	      bzero ((char *) ht, sizeof (idx_hash_rec *) * size);
	      ChangeMem (sizeof (idx_hash_rec *) * size);

	      for (i = 0; i < IdxHashSize; i++)
		if (IdxHashTable[i])
		  {
		    register u_char *wptr;
		    idx_hash_rec *ent;
		    register unsigned long hashval, step;
		    
		    wptr = IdxHashTable[i]->word;
		    HASH (hashval, step, wptr, size);
		    ent = ht[hashval];
		    while (ent)
		      {
			hashval += step;
			if (hashval >= size)
			  hashval -= size;
			ent = ht[hashval];
		      }
		    ht[hashval] = IdxHashTable[i];
		  }
	      Xfree (IdxHashTable);
	      ChangeMem (-sizeof (idx_hash_rec *) * IdxHashSize);
	      IdxHashTable = ht;
	      IdxHashSize = size;
	    }
	  
	} /* end for each word */

    } /* end for each block */

}
 
static int 
idx_comp (const void *A, const void *B)
{
  u_char *s1 = (*((idx_hash_rec **) A))->word;
  u_char *s2 = (*((idx_hash_rec **) B))->word;
  return (casecompare (s1, s2));
}


void
save_idx (char * filename, int stem_method)
{
  char *FName;
  unsigned long i, j, pos, First_word, num;
  struct stem_idx_header sih;
  u_char *buffer, *last_word = NULL;
  unsigned short *pointers;
  int buf_in_use;
  unsigned short ptrs_in_use, word_num;
  FILE *idbi = NULL, *tmp = NULL;

  FName = make_name (filename, ".tmp", NULL);
  if (!(tmp = fopen (FName, "w+b")))
    FatalError (1, "Unable to open \"%s\".\n", FName);

  /* Delete the file now */
  unlink (FName);

  /* Create appropriate stem index file */
  switch (stem_method)
    {
    case (1):
      {
	idbi = create_file (filename, INVF_DICT_BLOCKED_1_SUFFIX, "wb", MAGIC_STEM_1,
			    MG_ABORT);
	break;
      }

    case (2):
      {
	idbi = create_file (filename, INVF_DICT_BLOCKED_2_SUFFIX, "wb", MAGIC_STEM_2,
			    MG_ABORT);
	break;
      }
    case (3):
      {
	idbi = create_file (filename, INVF_DICT_BLOCKED_3_SUFFIX, "wb", MAGIC_STEM_3,
			    MG_ABORT);
	break;
      }
    }

  if (!idbi)
    FatalError (1, "Could NOT create .invf.blocked.%d file", stem_method);

  PackIdxHashTable();
  qsort (IdxHashTable, IdxHashUsed, sizeof (idx_hash_rec *), idx_comp);

  sih.lookback = lookback;
  sih.block_size = block_size;
  sih.num_blocks = 0;
  sih.blocks_start = 0;
  sih.index_chars = 0;
  sih.num_of_words = IdxHashUsed;

  fwrite ((char *) &sih, sizeof (sih), 1, idbi);

  if (!(buffer = Xmalloc (block_size + 512)))
    FatalError (1, "Unable to allocate memory for \"buffer\"\n");
  if (!(pointers = Xmalloc (block_size + 512)))
    FatalError (1, "Unable to allocate memory for \"buffer\"\n");

  buf_in_use = 0;
  pos = 0;
  word_num = 0;
  ptrs_in_use = 0;
  First_word = 0;

  /* For each word in the hashtable... */
  for (i = 0; i < IdxHashUsed; i++)
    {
      register unsigned long extra, copy, suff;
      register struct idx_hash_rec *ent = IdxHashTable[i];

      /* build a new word on top of prev */
      if (last_word != NULL)
	copy = prefixlen (last_word, ent->word);
      else
	copy = 0;
      suff = *(ent->word) - copy;
      last_word = ent->word;

      if (word_num % sih.lookback == 0)
	/* Will need copy chars to add + a pointer in index */
	extra = copy + sizeof (*pointers);
      else
	extra = 0;
      if ((ptrs_in_use + 1) * sizeof (*pointers) + sizeof (ptrs_in_use) + extra +
	  buf_in_use + sizeof (First_word) + suff + 1 + sizeof (ent->PL->num_entries) + 
	  ent->PL->num_entries * sizeof (PosEntry) > block_size)
	{
	  /* Dump buffer to tmp file */
	  int chunk;
	  HTONUL(First_word);  /* [RPAP - Jan 97: Endian Ordering] */
	  HTONUS(word_num);  /* [RPAP - Jan 97: Endian Ordering] */
	  fwrite (&First_word, sizeof (First_word), 1, tmp);
	  fwrite (&word_num, sizeof (word_num), 1, tmp);
	  fwrite (pointers, sizeof (*pointers), ptrs_in_use, tmp);
	  fwrite (buffer, sizeof (u_char), buf_in_use, tmp);
	  bzero ((char *) buffer, block_size);
	  chunk = buf_in_use + ptrs_in_use * sizeof (*pointers) +
	    sizeof (ptrs_in_use) + sizeof (First_word);
	  if (force && chunk < block_size)
	    {
	      fwrite (buffer, sizeof (u_char), block_size - chunk, tmp);
	      chunk = block_size;
	    }

	  pos += chunk;

	  buf_in_use = 0;
	  word_num = 0;
	  ptrs_in_use = 0;
	  sih.num_blocks++;

	  /* Check that entry will fit into new block */
	  if (sizeof (*pointers) + sizeof (ptrs_in_use) + extra + sizeof (First_word) +
	      suff + 1 + sizeof (ent->PL->num_entries) + 
	      ent->PL->num_entries * sizeof (PosEntry) > block_size)
	    FatalError (1, "Block size to small");

	} /* if size > block_size */

      if (word_num % sih.lookback == 0)
	{
	  HTONUS2(buf_in_use, pointers[ptrs_in_use++]);  /* [RPAP - Jan 97: Endian Ordering] */
	  suff += copy;
	  copy = 0;
	}

      /* Output Word information */
      buffer[buf_in_use++] = copy;
      buffer[buf_in_use++] = suff;
      bcopy ((char *) (ent->word + copy + 1), (char *) (buffer + buf_in_use), suff);
      buf_in_use += suff;
      HTONUI(ent->PL->num_entries);  /* [RPAP - Jan 97: Endian Ordering] */
      bcopy ((char *) &(ent->PL->num_entries), (char *) (buffer + buf_in_use), sizeof (ent->PL->num_entries));
      NTOHUI(ent->PL->num_entries);  /* [RPAP - Jan 97: Endian Ordering] */
      buf_in_use += sizeof (ent->PL->num_entries);

      for (j = 0; j < ent->PL->num_entries; j++)
	{
	  register PosEntry *pe = &(ent->PL->PE[j]);
	  HTONUI(pe->num_cases);  /* [RPAP - Jan 97: Endian Ordering] */
	  bcopy ((char *) &(pe->num_cases), (char *) (buffer + buf_in_use), sizeof (pe->num_cases));
	  buf_in_use += sizeof (pe->num_cases);
	  HTONUI(pe->blk);  /* [RPAP - Jan 97: Endian Ordering] */
	  bcopy ((char *) &(pe->blk), (char *) (buffer + buf_in_use), sizeof (pe->blk));
	  buf_in_use += sizeof (pe->blk);
	  HTONUS(pe->blk_index);  /* [RPAP - Jan 97: Endian Ordering] */
	  bcopy ((char *) &(pe->blk_index), (char *) (buffer + buf_in_use), sizeof (pe->blk_index));
	  buf_in_use += sizeof (pe->blk_index);
	  HTONUS(pe->offset);  /* [RPAP - Jan 97: Endian Ordering] */
	  bcopy ((char *) &(pe->offset), (char *) (buffer + buf_in_use), sizeof (pe->offset));
	  buf_in_use += sizeof (pe->offset);
	}

      if (buf_in_use + ptrs_in_use * sizeof (*pointers) +
	  sizeof (ptrs_in_use) > block_size)
	FatalError (1, "Fatal Internal Error # 64209258\n");

      if (word_num == 0) /* the first word in each block - word_num set to 0 when buffer written to tmp file */
	{
	  /* Write word to main index */
	  fwrite (ent->word, sizeof (u_char), *(ent->word) + 1, idbi);
	  HTONUL(pos);  /* [RPAP - Jan 97: Endian Ordering] */
	  fwrite (&pos, sizeof (pos), 1, idbi);
	  NTOHUL(pos);  /* [RPAP - Jan 97: Endian Ordering] */
	  sih.index_chars += *(ent->word) + 1;
	  First_word = i;
	}
      word_num++;
    } /* end for each word */ /* for each hash entry */

  if (buf_in_use)
    {
      /* Write last buffer to tmp file */
      int chunk;

      /* [RPAP - Jan 97: Endian Ordering] */
      HTONUL(First_word);
      HTONUS(word_num);

      fwrite (&First_word, sizeof (First_word), 1, tmp);
      fwrite (&word_num, sizeof (word_num), 1, tmp);
      fwrite (pointers, sizeof (*pointers), ptrs_in_use, tmp);
      fwrite (buffer, sizeof (u_char), buf_in_use, tmp);
      bzero ((char *) buffer, block_size);
      chunk = buf_in_use + ptrs_in_use * sizeof (*pointers) +
	sizeof (ptrs_in_use) + sizeof (First_word);
      if (force && chunk < block_size)
	{
	  fwrite (buffer, sizeof (u_char), block_size - chunk, tmp);
	  chunk = block_size;
	}

      sih.num_blocks++;
    }

  rewind (tmp);
  sih.blocks_start = sih.index_chars + sizeof (u_long) + sizeof (sih) +
    sih.num_blocks * sizeof (pos);
  if (force)
    {
      int amount;
      amount = sih.blocks_start % block_size;
      if (amount != 0)
	{
	  bzero ((char *) buffer, block_size);
	  fwrite (buffer, sizeof (u_char), block_size - amount, idbi);
	  sih.blocks_start += block_size - amount;
	}
    }

  while ((num = fread (buffer, sizeof (u_char), block_size, tmp)) != 0)
    fwrite (buffer, sizeof (u_char), num, idbi);
  fclose (tmp);

  /* skip over the magic number */
  fseek (idbi, sizeof (u_long), 0);

  /* [RPAP - Jan 97: Endian Ordering] */
  HTONUL(sih.lookback);
  HTONUL(sih.block_size);
  HTONUL(sih.num_blocks);
  HTONUL(sih.blocks_start);
  HTONUL(sih.index_chars);
  HTONUL(sih.num_of_words);

  fwrite (&sih, sizeof (sih), 1, idbi);
  fclose (idbi);

#ifndef SILENT
  Message ("Stem %d:\n", stem_method);
  Message ("     Block size   : %10d\n", block_size);
  Message ("     Num_blocks   : %10d\n", NTOHUL(sih.num_blocks));  /* [RPAP - Jan 97: Endian Ordering] */
  Message ("     Max mem used : %10.1f Mb\n", (double) MaxMemInUse / 1024.0 / 1024.0);
  Message ("     Num_of_words : %10d\n", NTOHUL(sih.num_of_words));  /* [RPAP - Jan 97: Endian Ordering] */
#endif
}


void
UpdateStemDict (char * filename, int stem_method)
{
  FILE *idb;
  struct stem_dict_header sdh;

  if (!(idb = open_file (filename, INVF_DICT_BLOCKED_SUFFIX, "r+b",
			 MAGIC_STEM, MG_CONTINUE)))
    FatalError (1, "Could not update stemmed dict");

  fread ((char *) &sdh, sizeof (sdh), 1, idb);
  NTOHUL(sdh.indexed);  /* [RPAP - Jan 97: Endian Ordering] */
  sdh.indexed |= 1 << (stem_method - 1);
  HTONUL(sdh.indexed);
  fseek (idb, sizeof (u_long), 0);
  fwrite ((char *) &sdh, sizeof (sdh), 1, idb);
  fclose (idb);
}



/* Main */
int main (int argc, char **argv)
{
  File *idb;    /* File to .invf.dict.blocked */
  char *filename = "";
  stemmed_dict *sd;   /* Stemmed dictionary */
  int ch;
  char path[512];
  int stem_method = 0;

  msg_prefix = argv[0];
  opterr = 0;
  while ((ch = getopt (argc, argv, "f:d:b:hFs:")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	filename = optarg;
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
      case 's':
	stem_method = atoi (optarg);
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-d directory] "
		 "[-b num] [-F] [-h] -s 1|2|3 -f name\n", argv[0]);
	exit (1);
      }

  if (stem_method < 1 || stem_method > 3)
    FatalError (1, "Stem method must be 1, 2 or 3");

  /* Open required stem dict file */
  sprintf (path, FILE_NAME_FORMAT, get_basepath (), filename, INVF_DICT_BLOCKED_SUFFIX);
  if (!(idb = Fopen (path, "rb", MAGIC_STEM)))
    FatalError (1, "Unable to open \"%s\"", path);

  /* Read in idb header and index to blocks */
  if (!(sd = ReadStemDictBlk (idb)))
    FatalError (1, "Could not read stemmed dictionary");

  /* Process stemmed dictionary */
  mg_init_process ();
  process_stem_dict (sd, stem_method);
  save_idx (filename, stem_method);
 
  /* Close stemmed dict */
  Fclose (idb);

  /* Update stemmed dict */
  UpdateStemDict (filename, stem_method);

  return 0;
}

