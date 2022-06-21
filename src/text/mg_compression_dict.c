/**************************************************************************
 *
 * mg_compression_dict.c -- Routines for reading the compression dictionary
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
 * $Id: mg_compression_dict.c,v 1.3 1994/10/20 03:56:54 tes Exp $
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

#define MAXBITS (sizeof(unsigned long) * 8)

/*
   $Log: mg_compression_dict.c,v $
   * Revision 1.3  1994/10/20  03:56:54  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:45  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_compression_dict.c,v 1.3 1994/10/20 03:56:54 tes Exp $";

/* #define DEBUG1 */

/* #define DEBUG */

#define is_power_of_two(a) ((a) != 0 && (((a) & ((a)-1)) == 0))

#define MAX_RECALCULATIONS 100

typedef struct WordData
  {
    u_long freq, occur_num;
    float saving;
    float char_bit_cost;
    u_long num_trans;
    u_char *word;
  }
WordData;

static WordData *Words[2];
static u_long Num[2];
static u_long chars[2];

#define KIND(p) (((p) >= Words[0] && (p) < Words[0] + Num[0]) ? 0 : 1)
#define IsWord(p) (((p) >= Words[1] && (p) < Words[1] + Num[1]) ? 1 : 0)
#define IsNonWord(p) (((p) >= Words[0] && (p) < Words[0] + Num[0]) ? 1 : 0)


typedef struct DictData
  {
    WordData **wd;
    u_long num_wds;
    u_long chars;
  }
DictData;

typedef DictData DictInfo[2];

static DictInfo keep, discard, all;
static compression_stats_header csh;

static char *file_name = "";
static u_long novel_method = MG_NOVEL_HUFFMAN_CHARS;


static void ReadInWords (char *);
static void Select_on (int k, heap_comp hc);
static void Method3 (int k);
static void Select_all (void);
static u_long WriteOutWords (char *, u_long, int);
static int DecFreqIncWL (void *a, void *b);
static int OccuranceOrder (void *a, void *b);



int 
main (int argc, char **argv)
{
  ProgTime StartTime;
  int ch;
  char type = 'C';
  char mode = '0';
  int lookback = 2;
  double k = 0;
  u_long mem_reqd;
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "0123CPSf:d:l:hk:HBDYM")) != -1)
    switch (ch)
      {
      case 'H':
	novel_method = MG_NOVEL_HUFFMAN_CHARS;
	break;
      case 'B':
	novel_method = MG_NOVEL_BINARY;
	break;
      case 'D':
	novel_method = MG_NOVEL_DELTA;
	break;
      case 'Y':
	novel_method = MG_NOVEL_HYBRID;
	break;
      case 'M':
	novel_method = MG_NOVEL_HYBRID_MTF;
	break;
      case 'f':		/* input file */
	file_name = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'C':
      case 'P':
      case 'S':
	type = ch;
	break;
      case '0':
      case '1':
      case '2':
      case '3':
	mode = ch;
	break;
      case 'l':
	lookback = atoi (optarg);
	if (!is_power_of_two (lookback))
	  FatalError (1, "The lookback value must be a power of 2");
	lookback = floorlog_2 (lookback);
	break;
      case 'k':
	k = atof (optarg) * 1024;
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-l lookback] [-f input_file]"
	"[-d data directory] [-h] [-0|-1|-2|-3] [-C|-P|-S] [-k mem (Kb)]\n",
		 argv[0]);
	exit (1);
      }
  GetTime (&StartTime);

  ReadInWords (file_name);

  if (type == 'C')
    {
      Select_all ();
      mem_reqd = WriteOutWords (file_name, MG_COMPLETE_DICTIONARY, lookback);
    }
  else
    {
      switch (mode)
	{
	case '0':
	  Select_all ();
	  break;
	case '1':
	  Message ("Dictionary limit of %.2f Kb", k / 1024);
	  Select_on ((int) k, OccuranceOrder);
	  break;
	case '2':
	  Message ("Dictionary limit of %.2f Kb", k / 1024);
	  Select_on ((int) k, DecFreqIncWL);
	  break;
	case '3':
	  Message ("Dictionary limit of %.2f Kb", k / 1024);
	  Method3 ((int) k);
	  break;
	}
      if (type == 'P')
	{
	  mem_reqd = WriteOutWords (file_name, MG_PARTIAL_DICTIONARY, lookback);
	}
      else
	{
	  mem_reqd = WriteOutWords (file_name, MG_SEED_DICTIONARY, lookback);
	}
    }

  Message ("Num words           : %8u -> %8u\n", Num[1], keep[1].num_wds);
  Message ("Num non-words       : %8u -> %8u\n", Num[0], keep[0].num_wds);
  Message ("Chars of words      : %8u -> %8u\n", chars[1], keep[1].chars);
  Message ("Chars of non-words  : %8u -> %8u\n", chars[0], keep[0].chars);
  Message ("Mem usage           : %8u -> %8u\n",
	   (Num[0] + Num[1]) * sizeof (char *) + chars[0] + chars[1],
	     (keep[0].num_wds + keep[1].num_wds) * sizeof (char *) +
	   keep[0].chars + keep[1].chars);
  Message ("Actual mem required : %8u\n", mem_reqd);

  Message ("%s", ElapsedTime (&StartTime, NULL));
  exit (0);
}




static void 
ReadInWords (char *filename)
{
  FILE *f;
  unsigned long i;
  f = open_file (filename, TEXT_STATS_DICT_SUFFIX, "r",
		 MAGIC_STATS_DICT, MG_ABORT);

  fread (&csh, sizeof (csh), 1, f);

  for (i = 0; i < 2; i++)
    {
      frags_stats_header fsh;
      char *buf;
      WordData *wd;
      int j;
      chars[i] = 0;

      fread (&fsh, sizeof (fsh), 1, f);
      Num[i] = all[i].num_wds = fsh.num_frags;

      /* The +1 on the following line is to leave room for the esc code. */
      all[i].wd = Xmalloc (sizeof (WordData *) * Num[i] + 1);

      buf = Xmalloc (fsh.mem_for_frags);
      wd = Words[i] = Xmalloc (sizeof (WordData) * Num[i]);
      for (j = 0; j < Num[i]; j++, wd++)
	{
	  int len;
	  fread (&wd->freq, sizeof (wd->freq), 1, f);
	  fread (&wd->occur_num, sizeof (wd->occur_num), 1, f);
	  len = fgetc (f);
	  wd->word = (u_char *) buf;
	  *buf++ = len;
	  fread (buf, len, 1, f);
	  buf += len;
	  all[i].wd[j] = wd;
	}
      chars[i] = fsh.mem_for_frags - fsh.num_frags;
    }
  fclose (f);
}


static void 
Alloc_keep_discard (void)
{
  keep[0].num_wds = 0;
  keep[0].wd = Xmalloc ((Num[0] + 1) * sizeof (WordData *));
  keep[1].num_wds = 0;
  keep[1].wd = Xmalloc ((Num[1] + 1) * sizeof (WordData *));
  discard[0].num_wds = 0;
  discard[0].wd = Xmalloc ((Num[0] + 1) * sizeof (WordData *));
  discard[1].num_wds = 0;
  discard[1].wd = Xmalloc ((Num[1] + 1) * sizeof (WordData *));
}


static int 
sort_comp (const void *a, const void *b)
{
  WordData *aa = *(WordData **) a;
  WordData *bb = *(WordData **) b;
  return compare (aa->word, bb->word);
}



static void 
SortAndCount_DictData (DictData * dd)
{
  int i;
  WordData **wd;
  qsort (dd->wd, dd->num_wds, sizeof (WordData *), sort_comp);
  dd->chars = 0;
  wd = dd->wd;
  for (i = 0; i < dd->num_wds; i++, wd++)
    dd->chars += (*wd)->word[0];
}


static void 
Select_all (void)
{
  int i;
  Alloc_keep_discard ();
  keep[0].num_wds = Num[0];
  for (i = 0; i < Num[0]; i++)
    keep[0].wd[i] = Words[0] + i;
  keep[1].num_wds = Num[1];
  for (i = 0; i < Num[1]; i++)
    keep[1].wd[i] = Words[1] + i;
  SortAndCount_DictData (&keep[0]);
  SortAndCount_DictData (&keep[1]);
}




static int 
DecFreqIncWL (void *a, void *b)
{
  WordData *aa = *(WordData **) a;
  WordData *bb = *(WordData **) b;
  if (aa->freq > bb->freq)
    return -1;
  else if (aa->freq < bb->freq)
    return 1;
  else
    return bb->word[0] - aa->word[0];
}


static int 
OccuranceOrder (void *a, void *b)
{
  WordData *aa = *(WordData **) a;
  WordData *bb = *(WordData **) b;
  if (aa->occur_num > bb->occur_num)
    return 1;
  else if (aa->occur_num < bb->occur_num)
    return -1;
  else
    return 0;
}


static void 
Select_on (int k, heap_comp hc)
{
  int i, num, mem;
  WordData **wd;

  Alloc_keep_discard ();

  num = Num[0] + Num[1];
  wd = Xmalloc (num * sizeof (WordData *));
  for (i = 0; i < Num[0]; i++)
    wd[i] = Words[0] + i;
  for (i = 0; i < Num[1]; i++)
    wd[i + Num[0]] = Words[1] + i;

  heap_build (wd, sizeof (*wd), num, hc);

  mem = 0;
  while (k > mem && num)
    {
      int idx;
      WordData *word = wd[0];
#ifdef DEBUG
      fprintf (stderr, "%4d:%4d:%8d :%8d :%8d : \"%s\"\n",
	       keep[0].num_wds, keep[1].num_wds,
	       mem, word->freq, word->occur_num, word2str (word->word));
#endif
      mem += sizeof (u_char *) + word->word[0];
      heap_deletehead (wd, sizeof (*wd), &num, hc);
      idx = KIND (word);
      keep[idx].wd[keep[idx].num_wds++] = word;
    }

  for (i = 0; i < num; i++)
    {
      WordData *word = wd[i];
      int idx = KIND (word);
      discard[idx].wd[discard[idx].num_wds++] = word;
    }
  SortAndCount_DictData (&keep[0]);
  SortAndCount_DictData (&keep[1]);
  SortAndCount_DictData (&discard[0]);
  SortAndCount_DictData (&discard[1]);

  assert (keep[0].num_wds + discard[0].num_wds == Num[0]);
  assert (keep[1].num_wds + discard[1].num_wds == Num[1]);

}



static int 
BigSaving (void *a, void *b)
{
  WordData *aa = *(WordData **) a;
  WordData *bb = *(WordData **) b;
  return (aa->saving > bb->saving) ? -1 : (aa->saving < bb->saving);
}

static int 
SmallSaving (void *a, void *b)
{
  WordData *aa = *(WordData **) a;
  WordData *bb = *(WordData **) b;
  return (aa->saving < bb->saving) ? -1 : (aa->saving > bb->saving);
}


static void 
CalcCharCounts (WordData ** wd, int num,
		char *char_lens[2], char *len_lens[2],
		u_long escape[2])
{
  long char_freqs[2][256];
  long len_freqs[2][16];
  huff_data hd;
  int i;
  escape[0] = 0;
  escape[1] = 0;
  bzero ((char *) char_freqs, sizeof (char_freqs));
  bzero ((char *) len_freqs, sizeof (len_freqs));
  for (i = 0; i < num; i++, wd++)
    {
      u_long freq = (*wd)->freq;
      u_char *buf = (*wd)->word;
      int len = *buf++;
      int idx = KIND (*wd);
      len_freqs[idx][len] += freq;
      escape[idx] += freq;
      for (; len; len--, buf++)
	char_freqs[idx][(u_long) (*buf)] += freq;
    }
  Generate_Huffman_Data (256, char_freqs[0], &hd, NULL);
  char_lens[0] = hd.clens;
  Generate_Huffman_Data (256, char_freqs[1], &hd, NULL);
  char_lens[1] = hd.clens;

  Generate_Huffman_Data (16, len_freqs[0], &hd, NULL);
  len_lens[0] = hd.clens;
  Generate_Huffman_Data (16, len_freqs[1], &hd, NULL);
  len_lens[1] = hd.clens;
}









void 
CalcBitCost (WordData ** discard_word, int discard_num,
	     WordData ** keep_word, int keep_num, double freqs_trans[2],
	     u_long escape[2], int num_trans)
{
  char *char_lens[2];
  char *len_lens[2];
  double esc[2];
  int j;
  CalcCharCounts (discard_word, discard_num, char_lens, len_lens, escape);
  esc[0] = -log2 (escape[0] / (freqs_trans[0] + escape[0]));
  esc[1] = -log2 (escape[1] / (freqs_trans[1] + escape[1]));
  for (j = 0; j < discard_num; j++, discard_word++)
    {
      float cbc, wbc;
      u_char *buf = (*discard_word)->word;
      int len = *buf++;
      u_long freq = (*discard_word)->freq;
      int idx = KIND (*discard_word);

      cbc = len_lens[idx][len];
      for (; len; len--, buf++)
	cbc += char_lens[idx][(u_long) (*buf)];

      (*discard_word)->char_bit_cost = (cbc + esc[idx]) * freq;

      wbc = -log2 (freq / (freqs_trans[idx] + escape[idx])) * freq;

      (*discard_word)->saving = ((*discard_word)->char_bit_cost - wbc) /
	(sizeof (char *) + (*discard_word)->word[0]);

      (*discard_word)->num_trans = num_trans;
    }
  for (j = 0; j < keep_num; j++, keep_word++)
    {
      float cbc, wbc;
      u_char *buf = (*keep_word)->word;
      int len = *buf++;
      u_long freq = (*keep_word)->freq;
      int idx = KIND (*keep_word);

      cbc = len_lens[idx][len];
      for (; len; len--, buf++)
	cbc += char_lens[idx][(u_long) (*buf)];

      (*keep_word)->char_bit_cost = (cbc + esc[idx]) * freq;

      wbc = -log2 (freq / (freqs_trans[idx] + escape[idx])) * freq;

      (*keep_word)->saving = ((*keep_word)->char_bit_cost - wbc) /
	(sizeof (char *) + (*keep_word)->word[0]);

      (*keep_word)->num_trans = num_trans;
    }
  Xfree (char_lens[0]);
  Xfree (char_lens[1]);
  Xfree (len_lens[0]);
  Xfree (len_lens[1]);
}



static void 
Method3 (int k)
{
  int i, keep_num, discard_num, mem, num_trans, recalc_reqd;
  int keep_to_discard = 0;
  int discard_to_keep = 0;
  int recalcs = 0;
  double freqs_trans[2], total;
  u_long escape[2];
  WordData **keep_heap, **discard_heap;

  freqs_trans[0] = freqs_trans[1] = 0;
  num_trans = 0;

  discard_num = Num[0] + Num[1];
  discard_heap = Xmalloc (discard_num * sizeof (WordData *));

  keep_num = 0;
  keep_heap = Xmalloc (discard_num * sizeof (WordData *));


  for (i = 0; i < Num[0]; i++)
    discard_heap[i] = Words[0] + i;
  for (i = 0; i < Num[1]; i++)
    discard_heap[i + Num[0]] = Words[1] + i;

  heap_build (discard_heap, sizeof (*discard_heap), discard_num, DecFreqIncWL);

  mem = 0;
  while (k > mem && discard_num)
    {
      WordData *word = discard_heap[0];
      mem += sizeof (u_char *) + word->word[0];
      heap_deletehead (discard_heap, sizeof (word), &discard_num, DecFreqIncWL);
      keep_heap[keep_num++] = word;
      freqs_trans[KIND (word)] += word->freq;
      num_trans++;
    }

  CalcBitCost (discard_heap, discard_num, keep_heap, keep_num,
	       freqs_trans, escape, num_trans);
  heap_build (discard_heap, sizeof (*discard_heap), discard_num, BigSaving);
  heap_build (keep_heap, sizeof (*keep_heap), keep_num, SmallSaving);

  total = 0;
  recalc_reqd = 0;
  while (keep_num && discard_num)
    {
      if ((keep_num && keep_heap[0]->num_trans < num_trans) ||
	  (discard_num && discard_heap[0]->num_trans < num_trans))
	{
	  if (keep_num && keep_heap[0]->num_trans < num_trans)
	    {
	      WordData *word = keep_heap[0];
	      int idx = KIND (word);
	      float wbc;
#ifdef DEBUG1
	      fprintf (stderr, "KEEP \"%s\" %.2f ->", word2str (word->word),
		       word->saving);
#endif
	      wbc = -log2 (word->freq / (freqs_trans[idx] + escape[idx])) *
		word->freq;
	      word->saving = (word->char_bit_cost - wbc) /
		(sizeof (char *) + word->word[0]);
#ifdef DEBUG1
	      fprintf (stderr, " %.2f\n", word->saving);
#endif
	      word->num_trans = num_trans;
	      heap_changedhead (keep_heap, sizeof (word), keep_num, SmallSaving);
	    }

	  if (discard_num && discard_heap[0]->num_trans < num_trans)
	    {
	      WordData *word = discard_heap[0];
	      int idx = KIND (word);
	      float wbc;
#ifdef DEBUG1
	      fprintf (stderr, "DISCARD \"%s\" %.2f ->", word2str (word->word),
		       word->saving);
#endif
	      wbc = -log2 (word->freq / (freqs_trans[idx] + escape[idx])) *
		word->freq;
	      word->saving = (word->char_bit_cost - wbc) /
		(sizeof (char *) + word->word[0]);
#ifdef DEBUG1
	      fprintf (stderr, " %.2f\n", word->saving);
#endif
	      word->num_trans = num_trans;
	      heap_changedhead (discard_heap, sizeof (word),
				discard_num, BigSaving);
	    }
	}
      else if (keep_heap[0]->saving < discard_heap[0]->saving)
	{
	  assert (keep_num && discard_num);
	  if (keep_num && mem + sizeof (char *) + discard_heap[0]->word[0] > k)
	    {
	      /* Transfer the word at the top of the keep heap to the
	         discard heap. */
	      WordData *word = keep_heap[0];
	      int idx = KIND (word);
	      heap_deletehead (keep_heap, sizeof (word), &keep_num, SmallSaving);
	      discard_heap[discard_num] = word;
	      heap_additem (discard_heap, sizeof (word), &discard_num,
			    BigSaving);
	      freqs_trans[idx] -= word->freq;
	      mem -= sizeof (u_char *) + word->word[0];
	      num_trans++;
	      total += word->saving;
	      keep_to_discard++;
#ifdef DEBUG
	      fprintf (stderr,
		       "KEEP -> DISCARD %8d :%8d :%8.0f :%8.0f : \"%s\"\n",
		       mem, word->freq, word->char_bit_cost,
		       word->saving, word2str (word->word));
#endif
	    }
	  else
	    {
	      /* Transfer the word at the top of the discard heap to the
	         keep heap. */
	      WordData *word = discard_heap[0];
	      int idx = KIND (word);
	      heap_deletehead (discard_heap, sizeof (word),
			       &discard_num, BigSaving);
	      keep_heap[keep_num] = word;
	      heap_additem (keep_heap, sizeof (word), &keep_num, SmallSaving);
	      freqs_trans[idx] += word->freq;
	      mem += sizeof (u_char *) + word->word[0];
	      num_trans++;
	      total += word->saving;
	      discard_to_keep++;
#ifdef DEBUG
	      fprintf (stderr,
		       "DISCARD -> KEEP %8d :%8d :%8.0f :%8.0f : \"%s\"\n",
		       mem, word->freq, word->char_bit_cost,
		       word->saving, word2str (word->word));
#endif
	    }

	  recalc_reqd = 1;

	}
      else
	{
	  if (recalc_reqd == 0)
	    break;
#ifdef DEBUG1
	  fprintf (stderr, "--------------\n");
#endif
	  if (recalcs == MAX_RECALCULATIONS)
	    break;
	  CalcBitCost (discard_heap, discard_num, keep_heap, keep_num,
		       freqs_trans, escape, num_trans);
	  heap_build (discard_heap, sizeof (*discard_heap),
		      discard_num, BigSaving);
	  heap_build (keep_heap, sizeof (keep_heap), keep_num, SmallSaving);
	  recalc_reqd = 0;
	  recalcs++;
	}
    }

  Alloc_keep_discard ();

  for (i = 0; i < discard_num; i++)
    {
      WordData *word = discard_heap[i];
      int idx = KIND (word);
      assert (IsWord (word) || IsNonWord (word));
      discard[idx].wd[discard[idx].num_wds++] = word;
    }
  for (i = 0; i < keep_num; i++)
    {
      WordData *word = keep_heap[i];
      int idx = KIND (word);
      assert (IsWord (word) || IsNonWord (word));
      keep[idx].wd[keep[idx].num_wds++] = word;
    }
  SortAndCount_DictData (&keep[0]);
  SortAndCount_DictData (&keep[1]);
  SortAndCount_DictData (&discard[0]);
  SortAndCount_DictData (&discard[1]);

  assert (keep[0].num_wds + discard[0].num_wds == Num[0]);
  assert (keep[1].num_wds + discard[1].num_wds == Num[1]);
  Message ("Keep -> Discard        : %8d", keep_to_discard);
  Message ("Discard -> Keep        : %8d", discard_to_keep);
  Message ("Huffman Recalculations : %8d", recalcs);
  if (recalcs == MAX_RECALCULATIONS)
    Message ("WARNING: The number of recalculations == %d", MAX_RECALCULATIONS);
}










/****************************************************************************
 *****									*****
 *****			Dictionary saving code				*****
 *****									*****
 ****************************************************************************/



static void 
Write_cdh (FILE * f, compression_dict_header * cdh)
{
  fwrite (cdh, sizeof (*cdh), 1, f);
}


static void 
Write_words (FILE * f, DictData * dd)
{
  int i;
  u_char *curr, *prev = NULL;
  for (i = 0; i < dd->num_wds; i++)
    {
      int len;
      curr = dd->wd[i]->word;
      if (prev)
	/* look for prefix match with prev string */
	len = prefixlen (prev, curr);
      else
	len = 0;
      fputc ((len << 4) + (curr[0] - len), f);
      fwrite (curr + len + 1, sizeof (u_char), curr[0] - len, f);
      prev = curr;
    }

}


static int 
Uncompressed_size (DictData * dd)
{
  int i, us;
  for (us = i = 0; i < dd->num_wds; i++)
    us += dd->wd[i]->word[0];
  return us;
}


static u_long 
Write_data (FILE * f, DictData * dd, int lookback)
{
  u_long mem_reqd;
  huff_data *hd;
  int i; 
  u_long us = dd->num_wds;
  long *freqs;
  u_long huff_words_size[MAX_HUFFCODE_LEN + 1];
  u_long lencounts[MAX_HUFFCODE_LEN + 1];
  u_char *lastword[MAX_HUFFCODE_LEN + 1];

  if (!(freqs = Xmalloc ((dd->num_wds) * sizeof (*freqs))))
    FatalError (1, "Unable to allocate memory for freqs");

  for (i = 0; i < dd->num_wds; i++)
    {
      freqs[i] = dd->wd[i]->freq;
      us += dd->wd[i]->word[0];
    }

  if (!(hd = Generate_Huffman_Data (dd->num_wds, freqs, NULL, NULL)))
    FatalError (1, "Unable to allocate memory for huffman data");

  Xfree (freqs), freqs = NULL;

  if (Write_Huffman_Data (f, hd) == -1)
    FatalError (1, "Unable to write huffman data");


  fwrite (&us, sizeof (us), 1, f);



/* Calculate the amount of memory that will be required to store the text for
   each different huffman code len. Every 1<<lookback words for each different
   codelen length will not be prefixed by previous strings. */


  bzero ((char *) &huff_words_size, sizeof (huff_words_size));
  bzero ((char *) &lencounts, sizeof (lencounts));

  mem_reqd = 0;

  for (i = 0; i < dd->num_wds; i++)
    {
      int codelen = hd->clens[i];
      u_char *word = dd->wd[i]->word;

      if (!codelen)
	FatalError (1, "The length of a code for a word was zero");

      huff_words_size[codelen] += word[0] + 1;
      mem_reqd += word[0] + (lookback != 0);
#if 0
      if ((lencounts[codelen] & ((1 << lookback) - 1)) == 0)
	lastword[codelen] = word;
      else
	huff_words_size[codelen] -= prefixlen (lastword[codelen], word);
#else
      if ((lencounts[codelen] & ((1 << lookback) - 1)) != 0)
	{
	  int save = prefixlen (lastword[codelen], word);
	  mem_reqd -= save;
	  huff_words_size[codelen] -= save;
	}
      else
	{
	  mem_reqd += sizeof (u_char *);
	}
      lastword[codelen] = word;
#endif
      lencounts[codelen]++;
    }

  fwrite (huff_words_size + hd->mincodelen, sizeof (*huff_words_size),
	  hd->maxcodelen - hd->mincodelen + 1, f);
  Write_words (f, dd);

  Xfree (hd->clens);
  Xfree (hd);

  return mem_reqd;
}



static void 
Write_charfreqs (FILE * f, DictData * dd, int words,
		 int zero_freq_permitted)
{
  int j;
  long freqs[256];
  WordData **wd = dd->wd;
  huff_data *hd;

  bzero ((char *) freqs, sizeof (freqs));

  for (j = 0; j < dd->num_wds; j++, wd++)
    {
      u_char *buf = (*wd)->word;
      int len = *buf++;
      for (; len; len--, buf++)
	freqs[(u_long) (*buf)] += (*wd)->freq;
    }

  if (!zero_freq_permitted)
    for (j = 0; j < 256; j++)
      if (!freqs[j] && INAWORD (j) == words)
	freqs[j] = 1;

  if (!(hd = Generate_Huffman_Data (256, freqs, NULL, NULL)))
    FatalError (1, "Unable to allocate memory for huffman data");

  if (Write_Huffman_Data (f, hd) == -1)
    FatalError (1, "Unable to write huffman data");

  Xfree (hd->clens);
  Xfree (hd);
}




static void 
Write_wordlens (FILE * f, DictData * dd, int zero_freq_permitted)
{
  int j;
  long freqs[16];
  WordData **wd = dd->wd;
  huff_data *hd;

  bzero ((char *) freqs, sizeof (freqs));

  for (j = 0; j < dd->num_wds; j++, wd++)
    freqs[(*wd)->word[0]] += (*wd)->freq;

  if (!zero_freq_permitted)
    for (j = 0; j < 16; j++)
      if (!freqs[j])
	freqs[j] = 1;

  if (!(hd = Generate_Huffman_Data (16, freqs, NULL, NULL)))
    FatalError (1, "Unable to allocate memory for huffman data");

  if (Write_Huffman_Data (f, hd) == -1)
    FatalError (1, "Unable to write huffman data");


  Xfree (hd->clens);
  Xfree (hd);
}



static u_long 
WriteOutWords (char *file_name, u_long type, int lookback)
{
  FILE *f;
  int i;
  u_long mem_reqd = 0;
  compression_dict_header cdh;
  f = create_file (file_name, TEXT_DICT_SUFFIX, "w+",
		   MAGIC_DICT, MG_ABORT);

  bzero((char *) &cdh, sizeof(cdh));

  cdh.dict_type = type;
  cdh.novel_method = (type == MG_SEED_DICTIONARY) ? novel_method :
    MG_NOVEL_HUFFMAN_CHARS;

  cdh.num_words[1] = keep[1].num_wds;
  cdh.num_words[0] = keep[0].num_wds;
  cdh.num_word_chars[1] = Uncompressed_size (&keep[1]);
  cdh.num_word_chars[0] = Uncompressed_size (&keep[0]);
  cdh.lookback = lookback;
  Write_cdh (f, &cdh);

  for (i = 0; i < 2; i++)
    switch (type)
      {
      case MG_COMPLETE_DICTIONARY:
	{
	  mem_reqd += Write_data (f, &keep[i], lookback);
	}
	break;
      case MG_PARTIAL_DICTIONARY:
	{
	  if (keep[i].num_wds)
	    {
	      int j;
	      WordData esc;
	      esc.freq = 0;
	      esc.word = (u_char *) "";
	      keep[i].wd[keep[i].num_wds++] = &esc;
	      for (j = 0; j < discard[i].num_wds; j++)
		esc.freq += discard[i].wd[j]->freq;
	      if (!esc.freq)
		esc.freq++;
	      mem_reqd += Write_data (f, &keep[i], lookback);
	    }
	  Write_charfreqs (f, &discard[i], i, 1);
	  Write_wordlens (f, &discard[i], 1);
	}
	break;
      case MG_SEED_DICTIONARY:
	{
	  if (keep[i].num_wds)
	    {
	      int j;
	      WordData esc;
	      esc.freq = 0;
	      esc.word = (u_char *) "";
	      keep[i].wd[keep[i].num_wds++] = &esc;
	      for (j = 0; j < all[i].num_wds; j++)
		if (all[i].wd[j]->freq == 1)
		  esc.freq++;
	      if (!esc.freq)
		esc.freq++;
	      mem_reqd += Write_data (f, &keep[i], lookback);
	    }
	  switch (novel_method)
	    {
	    case MG_NOVEL_HUFFMAN_CHARS:
	      Write_charfreqs (f, &all[i], i, 0);
	      Write_wordlens (f, &all[i], 0);
	      break;
	    case MG_NOVEL_BINARY:
	      break;
	    case MG_NOVEL_DELTA:
	      break;
	    case MG_NOVEL_HYBRID:
	      break;
	    case MG_NOVEL_HYBRID_MTF:
	      break;
	    default:
	      FatalError (1, "Bad novel method");
	    }
	}
	break;
      }
  fclose (f);
  return mem_reqd;
}
