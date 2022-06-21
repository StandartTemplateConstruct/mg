/**************************************************************************
 *
 * comp_dict.c -- Functions for loading the compression dictionary
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
 * $Id: comp_dict.c,v 1.3 1994/10/20 03:56:41 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "huffman.h"
#include "local_strings.h"
#include "memlib.h"
#include "messages.h"

#include "mg.h"
#include "hash.h"
#include "text.h"
#include "comp_dict.h"
#include "locallib.h"
#include "mg_files.h"

/*
   $Log: comp_dict.c,v $
   * Revision 1.3  1994/10/20  03:56:41  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:24  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: comp_dict.c,v 1.3 1994/10/20 03:56:41 tes Exp $";

compression_dict_header cdh;
compressed_text_header cth;
comp_frags_header cfh[2];

dict_hash_table *ht[2];

huff_data char_huff[2];
huff_data lens_huff[2];
u_long *char_codes[2], *lens_codes[2];
u_long Words_disk = 0;
u_long Chars_disk = 0;


static dict_hash_table *
ReadInWords (FILE * dict, comp_frags_header * cfh,
	     int esc)
{
  int i;
  u_char *allwords, *prev = NULL;
  dict_hash_table *ht;
  u_char **words;
  u_long ht_size;

  ht_size = prime (cfh->hd.num_codes * HASH_RATIO);
  if (!(ht = Xmalloc (sizeof (dict_hash_table) +
		      (ht_size - 1) * sizeof (ht->table[0]))))
    {
      Message ("no memory for hash_table\n");
      return NULL;
    }

  ht->size = ht_size;
  ht->hd = &cfh->hd;

  if (!(ht->codes = Generate_Huffman_Codes (&cfh->hd, NULL)))
    {
      Message ("no memory for huffman codes\n");
      return NULL;
    }

  if (!(ht->words = Xmalloc (sizeof (u_char *) * cfh->hd.num_codes)))
    {
      Message ("no memory for word pointers\n");
      return NULL;
    }
  words = ht->words;

  bzero ((char *) ht->table, ht_size * sizeof (ht->table[0]));

  if (!(allwords = Xmalloc (sizeof (u_char) * cfh->uncompressed_size)))
    {
      Message ("no memory for words\n");
      return NULL;
    }

  for (i = 0; i < cfh->hd.num_codes; i++, words++)
    {
      register int val, copy;
      val = fgetc (dict);
      copy = (val >> 4) & 0xf;
      val &= 0xf;

      *words = allwords;

      memcpy (allwords + 1, prev + 1, copy);
      fread (allwords + copy + 1, sizeof (u_char), val, dict);
      *allwords = val + copy;

      Words_disk += val + 1;

      /* insert into the hash table */
      if (i < cfh->hd.num_codes - esc)
	{
	  register u_char **wptr;
	  register int tsize = ht->size;
	  register unsigned long hashval, step;

	  HASH (hashval, step, allwords, tsize);

	  wptr = ht->table[hashval];
	  for (; wptr;)
	    {
	      hashval += step;
	      if (hashval >= tsize)
		hashval -= tsize;
	      wptr = ht->table[hashval];
	    }
	  ht->table[hashval] = words;
	}
      prev = allwords;
      allwords += *allwords + 1;
    }
  return ht;
}



int 
LoadCompressionDictionary (char *dict_file_name)
{
  FILE *dict;
  int which;
  if (!(dict = open_named_file (dict_file_name, "r", MAGIC_DICT, MG_MESSAGE)))
    return COMPERROR;

  Words_disk = sizeof (u_long);

  if (Read_cdh (dict, &cdh, NULL, &Words_disk) == -1)
    goto error;

  for (which = 0; which < 2; which++)
    switch (cdh.dict_type)
      {
      case MG_COMPLETE_DICTIONARY:
	{
	  if (Read_cfh (dict, &cfh[which], NULL, &Words_disk) == -1)
	    goto error;

	  if (!(ht[which] = ReadInWords (dict, &cfh[which], 0)))
	    goto error;

	}
	break;
      case MG_PARTIAL_DICTIONARY:
	{
	  if (cdh.num_words[which])
	    {
	      if (Read_cfh (dict, &cfh[which], NULL, &Words_disk) == -1)
		goto error;
	      if (!(ht[which] = ReadInWords (dict, &cfh[which], 1)))
		goto error;
	    }
	  else
	    ht[which] = NULL;
	  if (Read_Huffman_Data (dict, &char_huff[which], NULL,
				 &Chars_disk) == -1)
	    goto error;
	  if (!(char_codes[which] =
		Generate_Huffman_Codes (&char_huff[which], NULL)))
	    goto error;
	  if (Read_Huffman_Data (dict, &lens_huff[which], NULL,
				 &Chars_disk) == -1)
	    goto error;
	  if (!(lens_codes[which] =
		Generate_Huffman_Codes (&lens_huff[which], NULL)))
	    goto error;
	}
	break;
      case MG_SEED_DICTIONARY:
	{
	  if (cdh.num_words[which])
	    {
	      if (Read_cfh (dict, &cfh[which], NULL, &Words_disk) == -1)
		goto error;
	      if (!(ht[which] = ReadInWords (dict, &cfh[which], 1)))
		goto error;
	    }
	  else
	    ht[which] = NULL;
	  switch (cdh.novel_method)
	    {
	    case MG_NOVEL_HUFFMAN_CHARS:
	      if (Read_Huffman_Data (dict, &char_huff[which], NULL,
				     &Chars_disk) == -1)
		goto error;
	      if (!(char_codes[which] =
		    Generate_Huffman_Codes (&char_huff[which], NULL)))
		goto error;
	      if (Read_Huffman_Data (dict, &lens_huff[which], NULL,
				     &Chars_disk) == -1)
		goto error;
	      if (!(lens_codes[which] =
		    Generate_Huffman_Codes (&lens_huff[which], NULL)))
		goto error;
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
      default:
	FatalError (1, "Bad dictionary kind\n");
      }

  return (COMPALLOK);


error:
  fclose (dict);
  return (COMPERROR);
}
