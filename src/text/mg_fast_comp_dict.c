/**************************************************************************
 *
 * mg_fast_comp_dict.c -- Program to generate a fast compression dictionary
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
 * $Id: mg_fast_comp_dict.c,v 1.3 1994/10/20 03:56:55 tes Exp $
 *
 **************************************************************************/


/*
   $Log: mg_fast_comp_dict.c,v $
   * Revision 1.3  1994/10/20  03:56:55  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:47  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_fast_comp_dict.c,v 1.3 1994/10/20 03:56:55 tes Exp $";

#include "sysfuncs.h"

#include "filestats.h"
#include "huffman.h"
#include "timing.h"
#include "messages.h"
#include "memlib.h"

#include "local_strings.h"
#include "mg.h"
#include "text.h"
#include "invf.h"
#include "lists.h"
#include "backend.h"
#include "mg_files.h"
#include "locallib.h"
#include "words.h"



#define ALIGN_SIZE(p,t) (((p) + (sizeof(t)-1)) & (~(sizeof(t)-1)))

#define WORDNO(p, base) ((((char*)(p))-((char*)(base)))/sizeof(u_char*))
#define FIXUP(p) (fixup[WORDNO(p,buffer)/8] |= (1<<(WORDNO(p, buffer) & 7)))

#define IS_FIXUP(p) ((fixup[WORDNO(p,buffer)/8] & (1<<(WORDNO(p, buffer) & 7))) != 0)

#define FIXUP_VALS(vals) do {						\
	int i;								\
	for (i=0; i < MAX_HUFFCODE_LEN+1; i++)				\
	  FIXUP(&vals[i]);						\
      } while(0)



static u_long mem_for_comp_dict (char *filename);
static void load_comp_dict (char *filename);
static void save_fast_dict (char *filename);
static void unfixup_buffer (void);


static void *buffer;
static void *cur;
static u_char *fixup;
static u_long mem, fixup_mem;

int 
main (int argc, char **argv)
{
  ProgTime StartTime;
  int ch;
  char *filename = "";
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "f:d:h")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	filename = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-f input_file] [-d data directory] [-h]\n",
		 argv[0]);
	exit (1);
      }
  GetTime (&StartTime);

  mem = mem_for_comp_dict (filename);

  fixup_mem = (ALIGN_SIZE (mem, u_char *) / sizeof (u_char *) + 7) / 8;

  cur = buffer = Xmalloc (mem);
  bzero (buffer, mem);
  fixup = Xmalloc (fixup_mem);
  bzero (fixup, fixup_mem);

  Message ("Estimated memory for fast_dict %u", mem);
  Message ("Estimated memory for fixups %u", fixup_mem);

  load_comp_dict (filename);

  Message ("Actual memory for fast_dict %u", (char *) cur - (char *) buffer);

  if ((u_long) cur > (u_long) buffer + mem)
    FatalError (1, "The buffer was not big enough for the dictionary");

  unfixup_buffer ();

  save_fast_dict (filename);

  Message ("%s", ElapsedTime (&StartTime, NULL));
  exit (0);
}



static void 
unfixup_buffer ()
{
  u_long *p;
  for (p = buffer; (u_long) p < (u_long) cur; p++)
    {
      if (IS_FIXUP (p))
	*p = *p - (u_long) buffer;
    }
}




static u_long 
mem_for_aux_dict (compression_dict_header * cdh, char *filename)
{
  int i;
  u_long mem = sizeof (auxiliary_dict);
  FILE *aux;

  aux = open_file (filename, TEXT_DICT_AUX_SUFFIX, "r",
		   MAGIC_AUX_DICT, MG_ABORT);

  for (i = 0; i <= 1; i++)
    {
      aux_frags_header afh;
      fread (&afh, sizeof (afh), 1, aux);
      mem += afh.num_frags * sizeof (u_char *);
      mem = ALIGN_SIZE (mem + afh.mem_for_frags, u_char *);
      fseek (aux, afh.mem_for_frags, SEEK_CUR);
    }
  fclose (aux);

  return mem;
}



static u_long 
mem_for_words (FILE * dict, compression_dict_header * cdh,
	       comp_frags_header * cfh)
{
  u_long mem = 0;
  u_long i, lookback;
  int ptrs_reqd = 0;
  int mem_reqd = 0;

  lookback = cdh->lookback;

  for (i = cfh->hd.mincodelen; i <= cfh->hd.maxcodelen; i++)
    {
      ptrs_reqd += (cfh->hd.lencount[i] + ((1 << lookback) - 1)) >> lookback;
      mem_reqd += cfh->huff_words_size[i];
    }

  mem += ptrs_reqd * sizeof (u_char *);
  mem += (MAX_HUFFCODE_LEN + 1) * sizeof (u_char **);
  mem += mem_reqd;

  for (i = 0; i < cfh->hd.num_codes; i++)
    {
      register int val;
      for (val = getc (dict) & 0xf; val; val--)
	getc (dict);
    }
  return ALIGN_SIZE (mem, u_char *);
}




static u_long 
mem_for_comp_dict (char *filename)
{
  u_long mem = sizeof (compression_dict);
  compression_dict_header cdh;
  comp_frags_header cfh;
  FILE *dict;
  int which;

  dict = open_file (filename, TEXT_DICT_SUFFIX, "r",
		    MAGIC_DICT, MG_ABORT);

  fread (&cdh, sizeof (cdh), 1, dict);

  for (which = 0; which < 2; which++)
    switch (cdh.dict_type)
      {
      case MG_COMPLETE_DICTIONARY:
	{
	  mem += sizeof (comp_frags_header);
	  Read_cfh (dict, &cfh, NULL, NULL);

	  /* Don't need to count the space for the clens of the huffman data */

	  mem += mem_for_words (dict, &cdh, &cfh);
	  if (cfh.hd.clens)
	    Xfree (cfh.hd.clens);
	}
	break;
      case MG_PARTIAL_DICTIONARY:
	{
	  huff_data hd;
	  if (cdh.num_words[which])
	    {
	      mem += sizeof (comp_frags_header);
	      Read_cfh (dict, &cfh, NULL, NULL);

	      /* Don't need to count the space for the clens of the
	         huffman data */

	      mem += mem_for_words (dict, &cdh, &cfh);
	      if (cfh.hd.clens)
		Xfree (cfh.hd.clens);

	    }

	  mem += sizeof (hd);
	  Read_Huffman_Data (dict, &hd, NULL, NULL);
	  if (hd.clens)
	    Xfree (hd.clens);
	  mem += hd.num_codes * sizeof (unsigned long);
	  mem += (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *);

	  mem += sizeof (hd);
	  Read_Huffman_Data (dict, &hd, NULL, NULL);
	  if (hd.clens)
	    Xfree (hd.clens);
	  mem += hd.num_codes * sizeof (unsigned long);
	  mem += (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *);
	}
	break;
      case MG_SEED_DICTIONARY:
	{
	  huff_data hd;
	  if (cdh.num_words[which])
	    {
	      mem += sizeof (comp_frags_header);
	      Read_cfh (dict, &cfh, NULL, NULL);

	      /* Don't need to count the space for the clens of the
	         huffman data */

	      mem += mem_for_words (dict, &cdh, &cfh);
	      if (cfh.hd.clens)
		Xfree (cfh.hd.clens);

	    }
	  switch (cdh.novel_method)
	    {
	    case MG_NOVEL_HUFFMAN_CHARS:
	      mem += sizeof (hd);
	      Read_Huffman_Data (dict, &hd, NULL, NULL);
	      if (hd.clens)
		Xfree (hd.clens);
	      mem += hd.num_codes * sizeof (unsigned long);
	      mem += (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *);

	      mem += sizeof (hd);
	      Read_Huffman_Data (dict, &hd, NULL, NULL);
	      if (hd.clens)
		Xfree (hd.clens);
	      mem += hd.num_codes * sizeof (unsigned long);
	      mem += (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *);
	      break;
	    case MG_NOVEL_BINARY:
	      break;
	    case MG_NOVEL_DELTA:
	      break;
	    case MG_NOVEL_HYBRID:
	      break;
	    case MG_NOVEL_HYBRID_MTF:
	      break;
	    }
	  break;
	}
      }
  fclose (dict);

  if (cdh.novel_method == MG_NOVEL_BINARY ||
      cdh.novel_method == MG_NOVEL_DELTA ||
      cdh.novel_method == MG_NOVEL_HYBRID ||
      cdh.novel_method == MG_NOVEL_HYBRID_MTF)
    mem += mem_for_aux_dict (&cdh, filename);

  return ALIGN_SIZE (mem, u_char *);
}



void *
getmem (u_long size, int align)
{
  void *res;
  cur = (void *) (((u_long) cur + (align - 1)) & (~(align - 1)));
  res = cur;
  cur = (char *) cur + size;
  if ((long) cur > (long) buffer + mem)
    FatalError (1, "The buffer was not big enough for the dictionary");
  return res;
}











static auxiliary_dict *
LoadAuxDict (compression_dict_header * cdh,
	     char *filename)
{
  auxiliary_dict *ad;
  int i;
  FILE *aux;

  aux = open_file (filename, TEXT_DICT_AUX_SUFFIX, "r",
		   MAGIC_AUX_DICT, MG_ABORT);

  ad = getmem (sizeof (auxiliary_dict), sizeof (u_char *));

  for (i = 0; i <= 1; i++)
    {
      int j;
      u_char *pos;

      fread (&ad->afh[i], sizeof (aux_frags_header), 1, aux);


      ad->word_data[i] = getmem (ad->afh[i].mem_for_frags, 1);
      FIXUP (&ad->word_data[i]);

      ad->words[i] = getmem (ad->afh[i].num_frags * sizeof (u_char *),
			     sizeof (u_char *));
      FIXUP (&ad->words[i]);

      fread (ad->word_data[i], ad->afh[i].mem_for_frags, sizeof (u_char), aux);

      pos = ad->word_data[i];
      for (j = 0; j < ad->afh[i].num_frags; j++)
	{
	  ad->words[i][j] = pos;
	  FIXUP (&ad->words[i][j]);
	  pos += *pos + 1;
	}
      if (cdh->novel_method == MG_NOVEL_HYBRID ||
	  cdh->novel_method == MG_NOVEL_HYBRID_MTF)
	{
	  int num;
	  num = 1;
	  ad->blk_start[i][0] = 0;
	  ad->blk_end[i][0] = cdh->num_words[i] - 1;
	  while (num < 33)
	    {
	      ad->blk_start[i][num] = ad->blk_end[i][num - 1] + 1;
	      ad->blk_end[i][num] = ad->blk_start[i][num] +
		(ad->blk_end[i][num - 1] - ad->blk_start[i][num - 1]) * 2;
	      num++;
	    }
	}
    }
  fclose (aux);
  return (ad);
}





static u_char ***
ReadInWords (FILE * dict, compression_dict * cd,
	     comp_frags_header * cfh, u_char ** escape)
{
  int i, lookback;
  int ptrs_reqd = 0;
  int mem_reqd = 0;
  int num_set[MAX_HUFFCODE_LEN + 1];
  u_char *next_word[MAX_HUFFCODE_LEN + 1];
  u_char **vals;
  u_char ***values;
  u_char word[MAXWORDLEN + 1];
  u_char last_word[MAX_HUFFCODE_LEN + 1][MAXWORDLEN + 1];

  lookback = cd->cdh.lookback;

  for (i = cfh->hd.mincodelen; i <= cfh->hd.maxcodelen; i++)
    {
      ptrs_reqd += (cfh->hd.lencount[i] + ((1 << lookback) - 1)) >> lookback;
      mem_reqd += cfh->huff_words_size[i];
    }

  vals = getmem (ptrs_reqd * sizeof (*vals), sizeof (u_char *));

  values = getmem ((MAX_HUFFCODE_LEN + 1) * sizeof (u_char **), sizeof (u_char **));

  next_word[0] = getmem (mem_reqd, sizeof (char));

  cd->MemForCompDict += ptrs_reqd * sizeof (*vals) +
    (MAX_HUFFCODE_LEN + 1) * sizeof (u_char **) +
    mem_reqd;

  values[0] = vals;
  FIXUP (&values[0]);
  values[0][0] = next_word[0];
  FIXUP (&values[0][0]);
  for (i = 1; i <= cfh->hd.maxcodelen; i++)
    {
      int next_start = (values[i - 1] - vals) +
      ((cfh->hd.lencount[i - 1] + ((1 << lookback) - 1)) >> lookback);
      values[i] = &vals[next_start];
      FIXUP (&values[i]);
      next_word[i] = next_word[i - 1] + cfh->huff_words_size[i - 1];
      values[i][0] = next_word[i];
      FIXUP (&values[i][0]);
    }

  bzero ((char *) num_set, sizeof (num_set));

  for (i = 0; i < cfh->hd.num_codes; i++)
    {
      register int val, copy;
      register int len = cfh->hd.clens[i];
      val = getc (dict);
      copy = (val >> 4) & 0xf;
      val &= 0xf;

      fread (word + copy + 1, sizeof (u_char), val, dict);
      *word = val + copy;

      if ((num_set[len] & ((1 << lookback) - 1)) == 0)
	{
	  values[len][num_set[len] >> lookback] = next_word[len];
	  FIXUP (&values[len][num_set[len] >> lookback]);
	  memcpy (next_word[len], word, *word + 1);
	  if (escape && i == cfh->hd.num_codes - 1)
	    {
	      *escape = next_word[len];
	      FIXUP (escape);
	    }
	  next_word[len] += *word + 1;
	}
      else
	{
	  copy = prefixlen (last_word[len], word);
	  memcpy (next_word[len] + 1, word + copy + 1, *word - copy);
	  *next_word[len] = (copy << 4) + (*word - copy);
	  if (escape && i == cfh->hd.num_codes - 1)
	    {
	      *escape = next_word[len];
	      FIXUP (escape);
	    }
	  next_word[len] += (*word - copy) + 1;
	}
      memcpy (last_word[len], word, *word + 1);
      num_set[len]++;
    }
  if (cfh->hd.clens)
    Xfree (cfh->hd.clens);
  cfh->hd.clens = NULL;
  return values;
}


static unsigned long **
Generate_Fast_Huffman_Vals (huff_data * data, u_long * mem)
{
  int i;
  unsigned long *fcode[MAX_HUFFCODE_LEN + 1];
  unsigned long **values;
  unsigned long *vals;

  if (!data)
    return (NULL);
  vals = getmem (data->num_codes * sizeof (*vals), sizeof (long *));
  values = getmem ((MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *),
		   sizeof (long *));

  bzero ((char *) values, (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *));

  if (mem)
    *mem += data->num_codes * sizeof (*vals) +
      (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *);

  fcode[0] = values[0] = &vals[0];
  FIXUP (&values[0]);
  for (i = 1; i <= data->maxcodelen; i++)
    {
      fcode[i] = values[i] = &vals[(values[i - 1] - vals) + data->lencount[i - 1]];
      FIXUP (&values[i]);
    }

  for (i = 0; i < data->num_codes; i++)
    if (data->clens[i])
      *fcode[(int) (data->clens[i])]++ = i;
  return (values);
}



static void 
load_comp_dict (char *filename)
{
  FILE *dict;
  int which;
  compression_dict *cd;

  cd = getmem (sizeof (compression_dict), sizeof (u_char *));
  cd->fast_loaded = 1;

  dict = open_file (filename, TEXT_DICT_SUFFIX, "r",
		    MAGIC_DICT, MG_ABORT);

  Read_cdh (dict, &cd->cdh, NULL, NULL);

  for (which = 0; which < 2; which++)
    switch (cd->cdh.dict_type)
      {
      case MG_COMPLETE_DICTIONARY:
	{
	  cd->cfh[which] = getmem (sizeof (*cd->cfh[which]), sizeof (u_char *));
	  cd->MemForCompDict += sizeof (*cd->cfh[which]);
	  Read_cfh (dict, cd->cfh[which], &cd->MemForCompDict, NULL);

	  cd->values[which] = ReadInWords (dict, cd, cd->cfh[which], NULL);
	  FIXUP (&cd->cfh[which]);
	  FIXUP (&cd->values[which]);
	  cd->escape[which] = NULL;
	}
	break;
      case MG_PARTIAL_DICTIONARY:
	{
	  huff_data *hd;
	  u_long **vals;
	  if (cd->cdh.num_words[which])
	    {
	      cd->cfh[which] = getmem (sizeof (*cd->cfh[which]), sizeof (u_char *));
	      cd->MemForCompDict += sizeof (*cd->cfh[which]);
	      Read_cfh (dict, cd->cfh[which], &cd->MemForCompDict, NULL);
	      cd->values[which] = ReadInWords (dict, cd, cd->cfh[which],
					       &cd->escape[which]);
	      FIXUP (&cd->cfh[which]);
	      FIXUP (&cd->values[which]);
	      FIXUP (&cd->escape[which]);
	    }

	  hd = getmem (sizeof (huff_data), sizeof (char *));
	  cd->MemForCompDict += sizeof (huff_data);
	  Read_Huffman_Data (dict, hd, &cd->MemForCompDict, NULL);
	  vals = Generate_Fast_Huffman_Vals (hd, &cd->MemForCompDict);
	  cd->chars_huff[which] = hd;
	  FIXUP (&cd->chars_huff[which]);
	  cd->chars_vals[which] = vals;
	  FIXUP (&cd->chars_vals[which]);
	  if (hd->clens)
	    Xfree (hd->clens);
	  hd->clens = NULL;

	  hd = getmem (sizeof (huff_data), sizeof (char *));
	  cd->MemForCompDict += sizeof (huff_data);
	  Read_Huffman_Data (dict, hd, &cd->MemForCompDict, NULL);
	  vals = Generate_Fast_Huffman_Vals (hd, &cd->MemForCompDict);
	  cd->lens_huff[which] = hd;
	  FIXUP (&cd->lens_huff[which]);
	  cd->lens_vals[which] = vals;
	  FIXUP (&cd->lens_vals[which]);
	  if (hd->clens)
	    Xfree (hd->clens);
	  hd->clens = NULL;
	}
	break;
      case MG_SEED_DICTIONARY:
	{
	  huff_data *hd;
	  u_long **vals;
	  if (cd->cdh.num_words[which])
	    {
	      cd->cfh[which] = getmem (sizeof (*cd->cfh[which]), sizeof (u_char *));
	      cd->MemForCompDict += sizeof (*cd->cfh[which]);
	      Read_cfh (dict, cd->cfh[which], &cd->MemForCompDict, NULL);
	      cd->values[which] = ReadInWords (dict, cd, cd->cfh[which],
					       &cd->escape[which]);
	      FIXUP (&cd->cfh[which]);
	      FIXUP (&cd->values[which]);
	      FIXUP (&cd->escape[which]);
	    }
	  switch (cd->cdh.novel_method)
	    {
	    case MG_NOVEL_HUFFMAN_CHARS:
	      hd = getmem (sizeof (huff_data), sizeof (char *));
	      cd->MemForCompDict += sizeof (huff_data);
	      Read_Huffman_Data (dict, hd, &cd->MemForCompDict, NULL);
	      vals = Generate_Fast_Huffman_Vals (hd, &cd->MemForCompDict);
	      cd->chars_huff[which] = hd;
	      FIXUP (&cd->chars_huff[which]);
	      cd->chars_vals[which] = vals;
	      FIXUP (&cd->chars_vals[which]);
	      if (hd->clens)
		Xfree (hd->clens);
	      hd->clens = NULL;

	      hd = getmem (sizeof (huff_data), sizeof (char *));
	      cd->MemForCompDict += sizeof (huff_data);
	      Read_Huffman_Data (dict, hd, &cd->MemForCompDict, NULL);
	      vals = Generate_Fast_Huffman_Vals (hd, &cd->MemForCompDict);
	      cd->lens_huff[which] = hd;
	      FIXUP (&cd->lens_huff[which]);
	      cd->lens_vals[which] = vals;
	      FIXUP (&cd->lens_vals[which]);
	      if (hd->clens)
		Xfree (hd->clens);
	      hd->clens = NULL;
	      break;
	    case MG_NOVEL_BINARY:
	      break;
	    case MG_NOVEL_DELTA:
	      break;
	    case MG_NOVEL_HYBRID:
	      break;
	    case MG_NOVEL_HYBRID_MTF:
	      break;
	    }
	  break;
	}
      }
  fclose (dict);


  if (cd->cdh.novel_method == MG_NOVEL_BINARY ||
      cd->cdh.novel_method == MG_NOVEL_DELTA ||
      cd->cdh.novel_method == MG_NOVEL_HYBRID ||
      cd->cdh.novel_method == MG_NOVEL_HYBRID_MTF)
    {
      cd->ad = LoadAuxDict (&cd->cdh, filename);
      FIXUP (&cd->ad);
    }

}


static void 
save_fast_dict (char *filename)
{
  FILE *fdict;

  fdict = create_file (filename, TEXT_DICT_FAST_SUFFIX, "w",
		       MAGIC_FAST_DICT, MG_ABORT);

  fwrite (&mem, sizeof (mem), 1, fdict);
  fwrite (&fixup_mem, sizeof (fixup_mem), 1, fdict);
  fwrite (buffer, sizeof (u_char), mem, fdict);
  fwrite (fixup, sizeof (u_char), fixup_mem, fdict);

  fclose (fdict);
}
