/**************************************************************************
 *
 * text_get.c -- Function for reading documents from the compressed text
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
 * $Id: text_get.c,v 1.3 1994/10/20 03:57:11 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "filestats.h"
#include "timing.h"
#include "messages.h"

#include "huffman.h"
#include "bitio_m_mem.h"
#include "bitio_m.h"
#include "bitio_stdio.h"
#include "huffman_stdio.h"

#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"
#include "text_get.h"
#include "locallib.h"
#include "words.h"
#include "mg_errors.h"
#include "local_strings.h"


/*
   $Log: text_get.c,v $
   * Revision 1.3  1994/10/20  03:57:11  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:15  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: text_get.c,v 1.3 1994/10/20 03:57:11 tes Exp $";





/* FetchDocStart () 
 * Reads into DocEnt the starting position of the document in the *.text file
 * Where the first document is document number 1
 * It returns the true weight of the document.
 */




static double 
FetchDocStartLev1 (text_data * td, u_long DN,
		   u_long * seek_pos, u_long * len)
{
  unsigned long data[2];
  /* [TS:Sep/94] Fixed up the seek call to give the correct offset */
  Fseek (td->TextIdxFile,
	 sizeof (unsigned long) * (DN - 1) +	/* the doc offsets */
	 sizeof (unsigned long) +	/* the magic number */
	 sizeof (compressed_text_header),	/* the header */
	 0);
  Fread ((char *) &data, sizeof (data), 1, td->TextIdxFile);
  *seek_pos = data[0];
  *len = data[1] - data[0];
  return (1.0);
}

#define MG_PAGE_SIZE 2048

static int 
LoadIdx (text_data * td, unsigned long DN)
{
  if (!td->idx_data)
    {
      td->idx_data = Xmalloc (sizeof (*(td->idx_data)) * MG_PAGE_SIZE);
      if (!td->idx_data)
	FatalError (1, "Out of memory in FDSL2");
    }
  if (td->current_pos == -1 || DN >= td->current_pos + MG_PAGE_SIZE - 1 ||
      DN < td->current_pos)
    {
      long rn = (long) DN - (MG_PAGE_SIZE >> 1);
      if (rn < 1)
	rn = 1;
      Fseek (td->TextIdxWgtFile, (sizeof (unsigned long) + sizeof (float)) *
	       (rn - 1) + sizeof (unsigned long), 0);
      Fread ((char *) td->idx_data, sizeof (*(td->idx_data)) * MG_PAGE_SIZE,
	     1, td->TextIdxWgtFile);
      td->current_pos = rn;
    }
  return DN - td->current_pos;
}

static double 
FDSL2 (text_data * td, unsigned long DN, unsigned long *Pos)
{
  unsigned long pos = LoadIdx (td, DN);
  *Pos = td->idx_data[pos].Start;
  return (td->idx_data[pos].Weight);
}


static double 
FetchDocStartLev2 (text_data * td, u_long DN,
		   u_long * seek_pos, u_long * len)
{
  double Weight;
  unsigned long s1, s2;
  Weight = FDSL2 (td, DN, &s1);
  do
    {
      DN++;
      FDSL2 (td, DN, &s2);
    }
  while (s2 == s1);
  *seek_pos = s1;
  *len = s2 - s1;
  return (Weight);
}




double 
FetchDocStart (query_data * qd, u_long DN, u_long * seek_pos, u_long * len)
{
  qd->text_idx_lookups++;
  if (qd->td->TextIdxWgtFile)
    return FetchDocStartLev2 (qd->td, DN, seek_pos, len);
  else
    return FetchDocStartLev1 (qd->td, DN, seek_pos, len);
}

unsigned long 
FetchInitialParagraph (text_data * td, unsigned long ParaNum)
{
  if (td->TextIdxWgtFile)
    {
      unsigned long pos;
      unsigned long start;
      int PN = ParaNum - 1;
      pos = LoadIdx (td, ParaNum);
      start = td->idx_data[pos].Start;
      while (PN > 0)
	{
	  pos = LoadIdx (td, PN);
	  if (td->idx_data[pos].Start != start)
	    return PN + 1;
	  PN--;
	}
      return PN + 1;
    }
  else
    return ParaNum;
}



/* FetchCompressed () 
 * Reads into buffer DocBuff the compressed form of document DocNum. 
 * Where the first document is document number 1
 */
int 
FetchCompressed (query_data * qd, char **DocBuff, DocEntry * DocEnt)
{
  if (!DocEnt->SeekPos)
    FetchDocStart (qd, DocEnt->DocNum, &DocEnt->SeekPos, &DocEnt->Len);
  if (!(*DocBuff = Xmalloc (DocEnt->Len)))
    return (-1);

  if (Fseek (qd->td->TextFile, DocEnt->SeekPos, 0) == -1)
    FatalError (1, "Error when seeking into text file");
#if 0
  printf ("Loading compressed text %d %d\n", DocEnt->SeekPos, DocEnt->Len);
#endif
  if (Fread (*DocBuff, 1, DocEnt->Len, qd->td->TextFile) != DocEnt->Len)
    FatalError (1, "Error when reading data");

  return (DocEnt->Len);

}


text_data *
LoadTextData (File * text, File * text_idx_wgt, File * text_idx)
{
  text_data *td;

  if (!(td = Xmalloc (sizeof (text_data))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  td->TextFile = text;
  td->TextIdxWgtFile = text_idx_wgt;
  td->TextIdxFile = text_idx;
  td->current_pos = -1;
  td->idx_data = NULL;
  Fread (&td->cth, sizeof (td->cth), 1, text);
  return (td);
}


void 
FreeTextData (text_data * td)
{
  if (td)
    {
      if (td->idx_data)
	Xfree (td->idx_data);
      Xfree (td);
    }
}


static int 
pts_comp (const void *A, const void *B)
{
  const DocEntry *const *a = A;
  const DocEntry *const *b = B;
  return (*a)->DocNum - (*b)->DocNum;
}




int 
GetPosLens (query_data * qd, DocEntry * Docs, int num)
{
  DocEntry **pts;
  int i, j;
  if (!(pts = Xmalloc (num * sizeof (DocEntry *))))
    {
      mg_errno = MG_NOMEM;
      return (-1);
    }
  for (i = j = 0; i < num; i++, Docs++)
    if (!Docs->SeekPos)
      pts[j++] = Docs;

  if (j)
    {
      qsort (pts, j, sizeof (DocEntry *), pts_comp);
      for (i = 0; i < j; i++)
	FetchDocStart (qd, pts[i]->DocNum, &pts[i]->SeekPos, &pts[i]->Len);
    }

  Xfree (pts);
  return (0);
}





int 
LoadBuffers (query_data * qd, DocEntry * Docs, int max_mem, int num)
{
  DocEntry **pts;
  int i, j;
  int mem;

  if (!num)
    return (0);
  if (!(pts = Xmalloc (num * sizeof (DocEntry *))))
    {
      mg_errno = MG_NOMEM;
      return (-1);
    }

  mem = i = 0;
  do
    {
      pts[i] = Docs;
      mem += Docs->Len;
      i++;
      Docs++;
    }
  while (i < num && mem < max_mem);
  if (i > 1)
    qsort (pts, i, sizeof (DocEntry *), pts_comp);
  for (j = 0; j < i; j++)
    {
      if (FetchCompressed (qd, &pts[j]->CompTextBuffer, pts[j]) == -1)
	return (-1);
      ChangeMemInUse (qd, pts[j]->Len);
    }

  Xfree (pts);

  return (i);
}





void 
FreeBuffers (query_data * qd, DocEntry * Docs, int num)
{
  int i;
  for (i = 0; i < num; i++, Docs++)
    if (Docs->CompTextBuffer)
      {
	Xfree (Docs->CompTextBuffer);
	Docs->CompTextBuffer = NULL;
	ChangeMemInUse (qd, -Docs->Len);
      }
}



/****************************************************************************/

static void 
FreeAuxDict (auxiliary_dict * ad)
{
  if (!ad)
    return;
  if (ad->word_data[0])
    Xfree (ad->word_data[0]);
  if (ad->word_data[1])
    Xfree (ad->word_data[1]);
  if (ad->words[0])
    Xfree (ad->words[0]);
  if (ad->words[1])
    Xfree (ad->words[1]);
  Xfree (ad);
}

static auxiliary_dict *
LoadAuxDict (compression_dict * cd, File * text_aux_dict)
{
  auxiliary_dict *ad;
  int i;

  if (!(ad = Xmalloc (sizeof (auxiliary_dict))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  bzero ((char *) ad, sizeof (*ad));

  for (i = 0; i <= 1; i++)
    {
      int j;
      u_char *pos;

      Fread (&ad->afh[i], sizeof (aux_frags_header), 1, text_aux_dict);

      if (!(ad->word_data[i] = Xmalloc (ad->afh[i].mem_for_frags)))
	{
	  mg_errno = MG_NOMEM;
	  FreeAuxDict (ad);
	  return (NULL);
	}
      if (!(ad->words[i] = Xmalloc (ad->afh[i].num_frags * sizeof (u_char *))))
	{
	  mg_errno = MG_NOMEM;
	  FreeAuxDict (ad);
	  return (NULL);
	}

      Fread (ad->word_data[i], ad->afh[i].mem_for_frags, sizeof (u_char),
	     text_aux_dict);

      pos = ad->word_data[i];
      for (j = 0; j < ad->afh[i].num_frags; j++)
	{
	  ad->words[i][j] = pos;
	  pos += *pos + 1;
	}
      if (cd->cdh.novel_method == MG_NOVEL_HYBRID ||
	  cd->cdh.novel_method == MG_NOVEL_HYBRID_MTF)
	{
	  int num;
	  num = 1;
	  ad->blk_start[i][0] = 0;
	  ad->blk_end[i][0] = cd->cdh.num_words[i] - 1;
	  while (num < 33)
	    {
	      ad->blk_start[i][num] = ad->blk_end[i][num - 1] + 1;
	      ad->blk_end[i][num] = ad->blk_start[i][num] +
		(ad->blk_end[i][num - 1] - ad->blk_start[i][num - 1]) * 2;
	      num++;
	    }
	}
    }
  return (ad);
}






static u_char ***
ReadInWords (File * dict, compression_dict * cd,
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

  if (!(vals = Xmalloc (ptrs_reqd * sizeof (*vals))))
    return (NULL);

  if (!(values = Xmalloc ((MAX_HUFFCODE_LEN + 1) * sizeof (u_char **))))
    return (NULL);

  if (!(next_word[0] = Xmalloc (mem_reqd)))
    return (NULL);

  cd->MemForCompDict += ptrs_reqd * sizeof (*vals) +
    (MAX_HUFFCODE_LEN + 1) * sizeof (u_char **) +
    mem_reqd;

  values[0] = vals;
  values[0][0] = next_word[0];
  for (i = 1; i <= cfh->hd.maxcodelen; i++)
    {
      int next_start = (values[i - 1] - vals) +
      ((cfh->hd.lencount[i - 1] + ((1 << lookback) - 1)) >> lookback);
      values[i] = &vals[next_start];
      next_word[i] = next_word[i - 1] + cfh->huff_words_size[i - 1];
      values[i][0] = next_word[i];
    }

  bzero ((char *) num_set, sizeof (num_set));

  for (i = 0; i < cfh->hd.num_codes; i++)
    {
      register int val, copy;
      register int len = cfh->hd.clens[i];
      val = Getc (dict);
      copy = (val >> 4) & 0xf;
      val &= 0xf;

      Fread (word + copy + 1, sizeof (u_char), val, dict);
      *word = val + copy;

      if ((num_set[len] & ((1 << lookback) - 1)) == 0)
	{
	  values[len][num_set[len] >> lookback] = next_word[len];
	  memcpy (next_word[len], word, *word + 1);
	  if (escape && i == cfh->hd.num_codes - 1)
	    *escape = next_word[len];
	  next_word[len] += *word + 1;
	}
      else
	{
	  copy = prefixlen (last_word[len], word);
	  memcpy (next_word[len] + 1, word + copy + 1, *word - copy);
	  *next_word[len] = (copy << 4) + (*word - copy);
	  if (escape && i == cfh->hd.num_codes - 1)
	    *escape = next_word[len];
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


static compression_dict *
Load_Comp_Dict (File * dict, File * aux_dict)
{
  int which;
  compression_dict *cd;

  if (!(cd = Xmalloc (sizeof (compression_dict))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  bzero ((char *) cd, sizeof (compression_dict));

  cd->MemForCompDict = sizeof (compression_dict);

  if (F_Read_cdh (dict, &cd->cdh, &cd->MemForCompDict, NULL) == -1)
    return NULL;

  for (which = 0; which < 2; which++)
    switch (cd->cdh.dict_type)
      {
      case MG_COMPLETE_DICTIONARY:
	{
	  if (!(cd->cfh[which] = Xmalloc (sizeof (*cd->cfh[which]))))
	    return NULL;
	  cd->MemForCompDict += sizeof (*cd->cfh[which]);
	  if (F_Read_cfh (dict, cd->cfh[which], &cd->MemForCompDict, NULL) == -1)
	    return NULL;

	  if (!(cd->values[which] = ReadInWords (dict, cd, cd->cfh[which],
						 NULL)))
	    return NULL;
	  cd->escape[which] = NULL;

	}
	break;
      case MG_PARTIAL_DICTIONARY:
	{
	  huff_data *hd;
	  u_long **vals;
	  if (cd->cdh.num_words[which])
	    {
	      if (!(cd->cfh[which] = Xmalloc (sizeof (*cd->cfh[which]))))
		return NULL;
	      cd->MemForCompDict += sizeof (*cd->cfh[which]);
	      if (F_Read_cfh (dict, cd->cfh[which], &cd->MemForCompDict, NULL) == -1)
		return NULL;

	      if (!(cd->values[which] = ReadInWords (dict, cd, cd->cfh[which],
						     &cd->escape[which])))
		return NULL;
	    }
	  if (!(hd = Xmalloc (sizeof (huff_data))))
	    return NULL;
	  cd->MemForCompDict += sizeof (huff_data);
	  if (F_Read_Huffman_Data (dict, hd, &cd->MemForCompDict, NULL) == -1)
	    return NULL;
	  if (!(vals = Generate_Huffman_Vals (hd, &cd->MemForCompDict)))
	    return NULL;
	  if (hd->clens)
	    Xfree (hd->clens);
	  hd->clens = NULL;
	  cd->chars_huff[which] = hd;
	  cd->chars_vals[which] = vals;
	  if (!(hd = Xmalloc (sizeof (huff_data))))
	    return NULL;
	  cd->MemForCompDict += sizeof (huff_data);
	  if (F_Read_Huffman_Data (dict, hd, &cd->MemForCompDict, NULL) == -1)
	    return NULL;
	  if (!(vals = Generate_Huffman_Vals (hd, &cd->MemForCompDict)))
	    return NULL;
	  cd->lens_huff[which] = hd;
	  cd->lens_vals[which] = vals;
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
	      if (!(cd->cfh[which] = Xmalloc (sizeof (*cd->cfh[which]))))
		return NULL;
	      cd->MemForCompDict += sizeof (*cd->cfh[which]);
	      if (F_Read_cfh (dict, cd->cfh[which], &cd->MemForCompDict, NULL) == -1)
		return NULL;

	      if (!(cd->values[which] = ReadInWords (dict, cd, cd->cfh[which],
						     &cd->escape[which])))
		return NULL;
	    }
	  switch (cd->cdh.novel_method)
	    {
	    case MG_NOVEL_HUFFMAN_CHARS:
	      if (!(hd = Xmalloc (sizeof (huff_data))))
		return NULL;
	      cd->chars_huff[which] = hd;
	      cd->MemForCompDict += sizeof (huff_data);
	      if (F_Read_Huffman_Data (dict, hd, &cd->MemForCompDict,
				       NULL) == -1)
		return NULL;
	      if (!(vals = Generate_Huffman_Vals (hd, &cd->MemForCompDict)))
		return NULL;
	      cd->chars_vals[which] = vals;
	      if (hd->clens)
		Xfree (hd->clens);
	      hd->clens = NULL;
	      if (!(hd = Xmalloc (sizeof (huff_data))))
		return NULL;
	      cd->MemForCompDict += sizeof (huff_data);
	      cd->lens_huff[which] = hd;
	      if (F_Read_Huffman_Data (dict, hd, &cd->MemForCompDict
				       ,NULL) == -1)
		return NULL;
	      if (!(vals = Generate_Huffman_Vals (hd, &cd->MemForCompDict)))
		return NULL;
	      cd->lens_vals[which] = vals;
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

  if (cd->cdh.novel_method == MG_NOVEL_BINARY ||
      cd->cdh.novel_method == MG_NOVEL_DELTA ||
      cd->cdh.novel_method == MG_NOVEL_HYBRID ||
      cd->cdh.novel_method == MG_NOVEL_HYBRID_MTF)
    {
      if (!aux_dict)
	{
	  mg_errno = MG_NOFILE;
	  FreeCompDict (cd);
	  return (NULL);
	}

      if (!(cd->ad = LoadAuxDict (cd, aux_dict)))
	{
	  FreeCompDict (cd);
	  return (NULL);
	}
    }


  mg_errno = MG_NOERROR;

  cd->fast_loaded = 0;
  return (cd);
}

#define WORDNO(p, base) ((((char*)(p))-((char*)(base)))/sizeof(u_char*))

#define IS_FIXUP(p) ((fixup[WORDNO(p,cd)/8] & (1<<(WORDNO(p, cd) & 7))) != 0)


static compression_dict *
Load_Fast_Comp_Dict (File * text_fast_comp_dict)
{
  compression_dict *cd;
  u_long *p, *end;
  u_char *fixup;
  u_long mem;
  u_long fixup_mem;
  Fread (&mem, sizeof (mem), 1, text_fast_comp_dict);
  Fread (&fixup_mem, sizeof (fixup_mem), 1, text_fast_comp_dict);
  if (!(cd = Xmalloc (mem)))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  end = (u_long *) (((u_char *) cd) + mem);
  Fread (cd, sizeof (u_char), mem, text_fast_comp_dict);

  if (!(fixup = Xmalloc (fixup_mem)))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  Fread (fixup, fixup_mem, sizeof (u_char), text_fast_comp_dict);

  for (p = (u_long *) cd; (u_long) p < (u_long) end; p++)
    if (IS_FIXUP (p))
      *p = *p + (u_long) cd;

  free (fixup);
  return (cd);
}




compression_dict *
LoadCompDict (File * text_comp_dict,
	      File * text_aux_dict,
	      File * text_fast_comp_dict)
{
  return text_fast_comp_dict ?
    Load_Fast_Comp_Dict (text_fast_comp_dict) :
    Load_Comp_Dict (text_comp_dict, text_aux_dict);
}




void 
FreeCompDict (compression_dict * cd)
{
  int which;
  if (cd->fast_loaded)
    {
      free (cd);
      return;
    }
  for (which = 0; which < 2; which++)
    {
      if (cd->cfh[which])
	Xfree (cd->cfh[which]);
      if (cd->chars_huff[which])
	Xfree (cd->chars_huff[which]);
      if (cd->lens_huff[which])
	Xfree (cd->lens_huff[which]);
      if (cd->values[which])
	{
	  Xfree (cd->values[which][0]);
	  Xfree (cd->values[which]);
	}
      if (cd->chars_vals[which])
	{
	  Xfree (cd->chars_vals[which][0]);
	  Xfree (cd->chars_vals[which]);
	}
      if (cd->lens_vals[which])
	{
	  Xfree (cd->lens_vals[which][0]);
	  Xfree (cd->lens_vals[which]);
	}
    }
  if (cd->ad)
    FreeAuxDict (cd->ad);
  Xfree (cd);
}





#define MY_HUFF_DECODE(len, code, mcodes)				\
  do {									\
    register unsigned long *__min_code = (mcodes);			\
    register unsigned long *__mclen = __min_code;			\
    register unsigned long __code = 0;					\
    do									\
      {									\
        DECODE_ADD(__code);						\
      }									\
    while (__code < *++__mclen);					\
    (len) = __mclen - __min_code;					\
    (code) = __code - *__mclen;						\
  } while(0);


/*#define DUMPDOC */

#define MAX_SWAPS 10000

int 
DecodeText (compression_dict * cd,
	    u_char * s_in, int l_in, u_char * s_out, int *l_out)
{
  auxiliary_dict *ad = cd->ad;
  int which;
  u_long num_bits, bits;
  u_char *ptr = s_out;
  static int num = 0;
  u_long binary_start[2];
  int novels_used[2];
  int swaps[2][MAX_SWAPS];
  novels_used[0] = novels_used[1] = 0;

  {
    unsigned char bf = s_in[l_in - 1];
    num_bits = 1;
    while ((bf & 1) != 1)
      {
	num_bits++;
	bf >>= 1;
      }
    num_bits = l_in * 8 - num_bits;
  }

  DECODE_START (s_in, l_in)

    which = DECODE_BIT;
  bits = 1;

  if (cd->cdh.novel_method == MG_NOVEL_BINARY)
    {
      DELTA_DECODE_L (binary_start[0], bits);
      DELTA_DECODE_L (binary_start[1], bits);
    }


  while (bits < num_bits)
    {
      register unsigned code, len;
      register int r;
      register u_char *t, *b = NULL;
      u_char word[MAXWORDLEN + 1];

#ifdef DUMPDOC
      printf ("\n%d %d  ", bits, num_bits);
#endif
      if (cd->cfh[which])
	{
	  MY_HUFF_DECODE (len, code, cd->cfh[which]->hd.min_code);
	  bits += len;

	  r = code & ((1 << cd->cdh.lookback) - 1);
	  t = cd->values[which][len][code >> cd->cdh.lookback];

	  /* step through from base pointer */
	  b = word + 1;
	  while (r--)
	    {
	      register int copy = *t >> 4;
	      memcpy (word + copy + 1, t + 1, *t & 0xf);
	      word[0] = copy + (*t & 0xf);
	      t += ((*t) & 0xf) + 1;
	    }
	}
      else
	t = NULL;
      if (t == cd->escape[which])
	{
	  switch (cd->cdh.novel_method)
	    {
	    case MG_NOVEL_HUFFMAN_CHARS:
	      {
		int len, i;
		int c;
		HUFF_DECODE_L (len, cd->lens_huff[which]->min_code,
			       cd->lens_vals[which], bits);
		for (i = 0; i < len; i++)
		  {
		    HUFF_DECODE_L (c, cd->chars_huff[which]->min_code,
				   cd->chars_vals[which], bits);
		    *ptr++ = c;
		  }
	      }
	      break;
	    case MG_NOVEL_BINARY:
	    case MG_NOVEL_DELTA:
	    case MG_NOVEL_HYBRID:
	    case MG_NOVEL_HYBRID_MTF:
	      {
		int idx = 0, len;
		u_char *base;
		switch (cd->cdh.novel_method)
		  {
		  case MG_NOVEL_BINARY:
		    {
		      BINARY_DECODE_L (idx, binary_start[which], bits);
		      if (idx == binary_start[which])
			binary_start[which]++;
		      idx--;
		    }
		    break;
		  case MG_NOVEL_DELTA:
		    {
		      DELTA_DECODE_L (idx, bits);
		      idx--;
		    }
		    break;
		  case MG_NOVEL_HYBRID:
		    {
		      int k;
		      GAMMA_DECODE_L (k, bits);
		      k--;
		      BINARY_DECODE_L (idx,
				       ad->blk_end[which][k] -
				       ad->blk_start[which][k] + 1, bits);
		      idx += ad->blk_start[which][k] - 1;
		    }
		    break;
		  case MG_NOVEL_HYBRID_MTF:
		    {
		      int k;
		      GAMMA_DECODE_L (k, bits);
		      k--;
		      BINARY_DECODE_L (idx,
				       ad->blk_end[which][k] -
				       ad->blk_start[which][k] + 1, bits);
		      idx += ad->blk_start[which][k] - 1;
		      if (idx >= novels_used[which])
			{
			  u_char *temp;
			  temp = ad->words[which][idx];
			  ad->words[which][idx] =
			    ad->words[which][novels_used[which]];
			  ad->words[which][novels_used[which]] = temp;
			  swaps[which][novels_used[which]] = idx;
			  idx = novels_used[which]++;
			}
		    }
		    break;
		  }
		base = ad->words[which][idx];
		len = *base++;
#ifdef DUMPDOC
		printf ("[[");
#endif
		for (; len; len--)
		  {
		    *ptr++ = *base++;
#ifdef DUMPDOC
		    putchar (*(base - 1));
#endif
		  }
#ifdef DUMPDOC
		printf ("]]");
#endif
	      }
	      break;
	    }
	}
      else
	{
	  /* copy over the matching prefix */
	  r = (*t >> 4);
	  while (r--)
#ifndef DUMPDOC
	    *ptr++ = *b++;
#else
	    {
	      *ptr = *b++;
	      putchar (*ptr);
	      ptr++;
	    }
#endif

	  /* and the stored suffix */
	  r = ((*t) & 0xf);
	  while (r--)
#ifndef DUMPDOC
	    *ptr++ = *++t;
#else
	    {
	      *ptr = *++t;
	      putchar (*ptr);
	      ptr++;
	    }
#endif
	}
      which = !which;
    }

  DECODE_DONE

    * l_out = ptr - s_out;
  num += *l_out + 1;

  if (cd->cdh.novel_method == MG_NOVEL_HYBRID_MTF)
    for (which = 0; which <= 1; which++)
      for (novels_used[which]--; novels_used[which] >= 0; novels_used[which]--)
	{
	  int a = novels_used[which];
	  int b = swaps[which][novels_used[which]];
	  u_char *temp;
	  temp = ad->words[which][a];
	  ad->words[which][a] = ad->words[which][b];
	  ad->words[which][b] = temp;
	}
  return (COMPALLOK);
}
