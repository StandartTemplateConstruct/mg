/**************************************************************************
 *
 * mgdictlist.c -- Program to list a dictionary
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
 * $Id: mgdictlist.c,v 1.4 1994/11/29 00:32:07 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "messages.h"
#include "memlib.h"

#include "mg_files.h"
#include "text.h"
#include "invf.h"
#include "locallib.h"
#include "local_strings.h"
#include "words.h"

/*
   $Log: mgdictlist.c,v $
   * Revision 1.4  1994/11/29  00:32:07  tes
   * Committing the new merged files and changes.
   *
   * Revision 1.3  1994/10/20  03:57:01  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:56  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mgdictlist.c,v 1.4 1994/11/29 00:32:07 tes Exp $";


int quick = 0;
int no_of_words[2];
u_long maxcodelen[2];

char *dictname = "";




void 
DumpStemDict (FILE * f)
{
  struct invf_dict_header idh;
  int i;
  u_char prev[MAXSTEMLEN + 1];

  fread (&idh, sizeof (idh), 1, f);
  if (quick)
    printf ("%ld\n", idh.dict_size);
  else
    {
      printf ("# lookback           = %lu\n", idh.lookback);
      printf ("# dict size          = %lu\n", idh.dict_size);
      printf ("# total bytes        = %lu\n", idh.total_bytes);
      printf ("# index string bytes = %lu\n", idh.index_string_bytes);
      printf ("# input bytes        = %lu\n", idh.input_bytes);
      printf ("# num of docs        = %lu\n", idh.num_of_docs);
      printf ("# static num of docs = %lu\n", idh.static_num_of_docs);
      printf ("# num of words       = %lu\n", idh.num_of_words);
      printf ("#\n");
    }

  for (i = 0; i < idh.dict_size; i++)
    {
      register unsigned long copy, suff;
      unsigned long wcnt, fcnt;

      /* build a new word on top of prev */
      copy = getc (f);
      suff = getc (f);
      *prev = copy + suff;
      fread (prev + copy + 1, sizeof (u_char), suff, f);

      /* read other data, but no need to store it */
      fread (&fcnt, sizeof (fcnt), 1, f);
      fread (&wcnt, sizeof (wcnt), 1, f);
      if (!quick)
	{
	  printf ("%d: %8ld ", i, wcnt);
	  printf ("/ %5ld ", fcnt);
	  printf ("%2d %2ld\t\"", *prev, copy);
	}
      printf ("%s", word2str (prev));
      if (quick)
	printf (" %ld %ld\n", wcnt, fcnt);
      else
	{
	  putchar ('"');
	  putchar ('\n');
	}
    }
}




void 
ReadInWords (FILE * f)
{
  comp_frags_header cfh;
  u_long *codes;
  u_char prev[MAXSTEMLEN + 1];
  int i;

  if (Read_cfh (f, &cfh, NULL, NULL) == -1)
    FatalError (1, "Unable to read in the dictionary");

  printf ("#\n");
  printf ("#   max code len       = %u\n", cfh.hd.maxcodelen);
  printf ("#   total bytes        = %lu\n", cfh.uncompressed_size);
  printf ("#\n");

  if (!(codes = Generate_Huffman_Codes (&cfh.hd, NULL)))
    FatalError (1, "no memory for huffman codes\n");

  for (i = 0; i < cfh.hd.num_codes; i++)
    {
      register int val, copy, j, k;
      char code[33];
      val = fgetc (f);
      copy = (val >> 4) & 0xf;
      val &= 0xf;

      fread (prev + copy + 1, sizeof (u_char), val, f);
      *prev = val + copy;

      for (k = 0, j = cfh.hd.clens[i] - 1; j >= 0; j--, k++)
	code[k] = '0' + ((codes[i] >> j) & 1);
      code[k] = '\0';

      printf ("%d: %2d : %*s : \"%s\"\n", i, cfh.hd.clens[i],
	      cfh.hd.maxcodelen, code, word2str (prev));
    }
  Xfree (codes);
  Xfree (cfh.hd.clens);
}


void 
ReadCharHuffman (FILE * f, char *title)
{
  int i;
  huff_data hd;
  u_long *codes;

  if (Read_Huffman_Data (f, &hd, NULL, NULL) == -1)
    FatalError (1, "Unable to read huffman data");

  if (!(codes = Generate_Huffman_Codes (&hd, NULL)))
    FatalError (1, "no memory for huffman codes\n");

  printf ("#\n# %s\n#\n", title);
  for (i = 0; i < hd.num_codes; i++)
    if (hd.clens[i])
      {
	int j, k;
	char code[33];
	for (k = 0, j = hd.clens[i] - 1; j >= 0; j--, k++)
	  code[k] = '0' + ((codes[i] >> j) & 1);
	code[k] = '\0';
	printf ("%2d : %*s : \"%s\"\n", hd.clens[i],
		hd.maxcodelen, code, char2str (i));
      }
  Xfree (codes);
  Xfree (hd.clens);
}


void 
ReadLenHuffman (FILE * f, char *title)
{
  int i;
  huff_data hd;
  u_long *codes;

  if (Read_Huffman_Data (f, &hd, NULL, NULL) == -1)
    FatalError (1, "Unable to read huffman data");

  if (!(codes = Generate_Huffman_Codes (&hd, NULL)))
    FatalError (1, "no memory for huffman codes\n");

  printf ("#\n# %s\n#\n", title);
  for (i = 0; i < hd.num_codes; i++)
    if (hd.clens[i])
      {
	int j, k;
	char code[33];
	for (k = 0, j = hd.clens[i] - 1; j >= 0; j--, k++)
	  code[k] = '0' + ((codes[i] >> j) & 1);
	code[k] = '\0';
	printf ("%2d : %*s : %d\n", hd.clens[i],
		hd.maxcodelen, code, i);
      }
  Xfree (codes);
  Xfree (hd.clens);
}





void 
DumpTextDict (FILE * f)
{
  struct compression_dict_header cdh;
  int which;

  if (Read_cdh (f, &cdh, NULL, NULL) == -1)
    FatalError (1, "Unable to read dictionary header");
  switch (cdh.dict_type)
    {
    case MG_COMPLETE_DICTIONARY:
      printf ("# COMPLETE DICTIONARY\n");
      break;
    case MG_PARTIAL_DICTIONARY:
      printf ("# PARTIAL DICTIONARY\n");
      break;
    case MG_SEED_DICTIONARY:
      printf ("# SEED DICTIONARY\n");
      break;
    }
  printf ("# num words          = %lu\n", cdh.num_words[1]);
  printf ("# num word chars     = %lu\n", cdh.num_word_chars[1]);
  printf ("# num non-words      = %lu\n", cdh.num_words[0]);
  printf ("# num non-word chars = %lu\n", cdh.num_word_chars[0]);
  printf ("# lookback           = %lu\n", cdh.lookback);

  for (which = 0; which < 2; which++)
    switch (cdh.dict_type)
      {
      case MG_COMPLETE_DICTIONARY:
	{
	  ReadInWords (f);
	}
	break;
      case MG_PARTIAL_DICTIONARY:
	{
	  if (cdh.num_words[which])
	    ReadInWords (f);

	  ReadCharHuffman (f, "Characters");
	  ReadLenHuffman (f, "Lengths");
	}
	break;
      case MG_SEED_DICTIONARY:
	{
	  if (cdh.num_words[which])
	    ReadInWords (f);

	  ReadCharHuffman (f, "Characters");
	  ReadLenHuffman (f, "Lengths");
	}
	break;
      }
}




void 
DumpStatsDict (FILE * f)
{
  int i;
  compression_stats_header csh;

  fread (&csh, sizeof (csh), 1, f);

  for (i = 0; i < 2; i++)
    {
      int j;
      frags_stats_header fsh;

      fread (&fsh, sizeof (fsh), 1, f);

      if (!quick)
	printf ("#\n# num %9s      = %lu\n#\n", i ? "words" : "non-words",
		fsh.num_frags);

      for (j = 0; j < fsh.num_frags; j++)
	{
	  u_char Word[16];
	  u_long freq, occur_num;

	  fread (&freq, sizeof (freq), 1, f);
	  fread (&occur_num, sizeof (occur_num), 1, f);

	  Word[0] = fgetc (f);
	  fread (Word + 1, Word[0], 1, f);
	  printf ("%d: %7ld : %7ld : \"%s\"\n", j, freq,
		  occur_num, word2str (Word));
	}
    }
}


int 
main (int argc, char **argv)
{
  FILE *fp;
  unsigned long magic = 0;

  if (argc < 2)
    FatalError (1, "A file name must be specified");
  dictname = argv[1];
  if (strcmp (dictname, "-q") == 0)
    {
      quick = 1;
      if (argc < 3)
	FatalError (1, "A file name must be specified");
      dictname = argv[2];
    }
  if (!(fp = fopen (dictname, "r")))
    FatalError (1, "Unable to open \"%s\"", dictname);

  fread (&magic, sizeof (magic), 1, fp);

  switch (magic)
    {
    case MAGIC_STEM_BUILD:
      if (!quick)
	printf ("# Contents of STEM file \"%s\"\n#\n", dictname);
      DumpStemDict (fp);
      break;
    case MAGIC_DICT:
      if (!quick)
	printf ("# Contents of DICT file \"%s\"\n#\n", dictname);
      DumpTextDict (fp);
      break;
    case MAGIC_STATS_DICT:
      if (!quick)
	printf ("# Contents of STATS file \"%s\"\n#\n", dictname);
      DumpStatsDict (fp);
      break;
    default:
      FatalError (1, "Bad magic number. \"%s\" cannot be dumped", dictname);
    }
  fclose (fp);
  exit (0);
}
