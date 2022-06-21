/**************************************************************************
 *
 * mg_weights_build.c -- Program to build the document weights file
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
 * $Id: mg_weights_build.c 16583 2008-07-29 10:20:36Z davidb $
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
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */

#include "mg_files.h"
#include "locallib.h"
#include "invf.h"
#include "text.h"
#include "words.h"

#define MAXBITS (sizeof(unsigned long) * 8)

/*
   $Log$
   Revision 1.2  2004/05/24 21:12:18  kjdon
   changed a message

   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:16  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:49  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:22  rjmcnab
   *** empty log message ***

   * Revision 1.4  1994/11/29  00:32:05  tes
   * Committing the new merged files and changes.
   *
   * Revision 1.3  1994/10/20  03:57:00  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:55  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_weights_build.c 16583 2008-07-29 10:20:36Z davidb $";

unsigned char bits = 8;
static char *file_name = "";
static char *text_file_name = "";
static unsigned long NumPara = 0;
static unsigned long StaticNumOfDocs = 0;

unsigned long get_NumPara (void);
unsigned long get_StaticNumOfDocs (void);
void GenerateWeights (void);
void Make_weight_approx (void);
void Make_text_idx_wgt (void);


int main (int argc, char **argv)
{
  ProgTime StartTime;
  int ch;
  opterr = 0;
  msg_prefix = argv[0];
  while ((ch = getopt (argc, argv, "f:t:d:b:sh")) != -1) /* [RJM 10/98 - Text Filename] */
    switch (ch)
      {
      case 'f':		/* input file */
	file_name = optarg;
	if (strlen(text_file_name) == 0) text_file_name = optarg;
	break;
      /* [RJM 10/98 - Text Filename] */
      case 't':		/* text input file */
	text_file_name = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'b':
	bits = atoi (optarg);
	if (bits > 32)
	  {
	    fprintf (stderr, "b may only take values 0-32\n");
	    exit (1);
	  }
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-f input_file]"
		 "[-d data directory] [-b bits] [-s] [-h]\n", argv[0]);
	exit (1);
      }
  GetTime (&StartTime);

  GenerateWeights ();

  Make_weight_approx ();

  Make_text_idx_wgt ();

  Message ("%s", ElapsedTime (&StartTime, NULL));

  return 0;
}




unsigned long 
get_NumPara (void)
{
  struct invf_dict_header idh;
  FILE *invf_dict;
  if (NumPara)
    return (NumPara);
  invf_dict = open_file (file_name, INVF_DICT_SUFFIX, "rb", MAGIC_STEM_BUILD,
			 MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */
  fread ((char *) &idh, sizeof (idh), 1, invf_dict);
  fclose (invf_dict);
  NTOHUL2(idh.num_of_docs, NumPara);  /* [RPAP - Jan 97: Endian Ordering] */
  return NumPara;
}



unsigned long 
get_StaticNumOfDocs (void)
/* the static number of documents is the N parameter used to
 * decode document gaps in the inverted file encoded using
 * the Bblock method.
 */
{
  struct invf_dict_header idh;
  FILE *invf_dict;
  if (StaticNumOfDocs)
    return (StaticNumOfDocs);
  invf_dict = open_file (file_name, INVF_DICT_SUFFIX, "rb", MAGIC_STEM_BUILD,
			 MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */
  fread ((char *) &idh, sizeof (idh), 1, invf_dict);
  fclose (invf_dict);
  NTOHUL2(idh.static_num_of_docs, StaticNumOfDocs);  /* [RPAP - Jan 97: Endian Ordering] */
  return StaticNumOfDocs;
}



void GenerateWeights (void) {
  FILE *dict, *invf, *f, *idx;
  struct invf_dict_header idh;
  struct invf_file_header ifh;
  int i;
  double logN;
  float *DocWeights;

  /* make sure the globals NumPara and StaticNumOfDocs are loaded */
  get_NumPara ();
  get_StaticNumOfDocs ();

  /* check to see if the weights file has already been built */
  if ((f = open_file (file_name, WEIGHTS_SUFFIX, "rb", MAGIC_WGHT,
		      MG_CONTINUE)) != NULL) {
      fclose (f);
      return;
  }
  Message ("The file \"%s.weight\" does not exist.", file_name);
  Message ("Building the weight data from the file \"%s.invf\".", file_name);

  logN = log ((double) NumPara);

  /* allocate memory for the weights */
  if (!(DocWeights = Xmalloc (sizeof (float) * (NumPara + 1))))
      FatalError (1, "No memory for doc weights");
  bzero ((char *) DocWeights, sizeof (float) * (NumPara + 1));

  /* open the .invf.dict file and read in its header */
  dict = open_file (file_name, INVF_DICT_SUFFIX, "rb", MAGIC_STEM_BUILD,
		    MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */
  fread ((char *) &idh, sizeof (idh), 1, dict);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(idh.lookback);
  NTOHUL(idh.dict_size);
  NTOHUL(idh.total_bytes);
  NTOHUL(idh.index_string_bytes);
  NTOHD(idh.input_bytes); /* [RJM 07/97: 4G limit] */
  NTOHUL(idh.num_of_docs);
  NTOHUL(idh.static_num_of_docs);
  NTOHUL(idh.num_of_words);
  NTOHUL(idh.stemmer_num);
  NTOHUL(idh.stem_method);

  /* open .invf.idx */
  idx = open_file (file_name, INVF_IDX_SUFFIX, "rb", MAGIC_INVI,
		   MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  /* open .invf and read in its header */
  invf = open_file (file_name, INVF_SUFFIX, "rb", MAGIC_INVF,
		    MG_ABORT);
  fread ((char *) &ifh, sizeof (ifh), 1, invf);

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(ifh.no_of_words);
  NTOHUL(ifh.no_of_ptrs);
  NTOHUL(ifh.skip_mode);
  for (i = 0; i <= 15; i++)
    NTOHUL(ifh.params[i]);
  NTOHUL(ifh.InvfLevel);

  /* make sure the inverted file does not contain skips and is not level 1 */
  if (ifh.skip_mode != 0)
    FatalError (0, "Can\'t make weights file from a skipped inverted file.");
  if (ifh.InvfLevel == 1)
    FatalError (0, "Can\'t make weights file from level 1 inverted file.");

  DECODE_START (invf)

    /* process each word adding its contributions to the document weights */
    for (i = 0; i < ifh.no_of_words; i++)
    {
      u_char dummy1, dummy2[MAXSTEMLEN + 1];
      unsigned long fcnt, wcnt, blk, CurrDoc, p, j;
      float idf;

      /* give a little feedback every 4096 words */
      if ((i & 0xfff) == 0)
	fprintf (stderr, ".");

      /* read an entry for a word, just to get p value */
      dummy1 = fgetc (dict);
      dummy1 = fgetc (dict);
      fread (dummy2, sizeof (u_char), dummy1, dict);
      fread ((char *) &fcnt, sizeof (fcnt), 1, dict);
      fread ((char *) &wcnt, sizeof (wcnt), 1, dict);

      dummy2[dummy1] = '\0';

      /* [RPAP - Jan 97: Endian Ordering] */
      NTOHUL(fcnt);
      NTOHUL(wcnt);

      p = fcnt;

      idf = logN - log ((double) fcnt);
      blk = BIO_Bblock_Init (StaticNumOfDocs, p);
      CurrDoc = 0;

      /* check the inverted file index entry for this word */
      {
	unsigned long loc;
	fread ((char *) &loc, sizeof (loc), 1, idx);
	NTOHUL(loc);  /* [RPAP - Jan 97: Endian Ordering] */
	if (ftell (invf) != loc)
	  {
	    FatalError (1, "Word %d  %d != %d", i, ftell (invf), loc);
	  }
      }

      for (j = 0; j < p; j++)
	{
	  unsigned long x, tf;
	  BBLOCK_DECODE (x, blk);
	  CurrDoc += x;

	  if (CurrDoc > idh.num_of_docs) {
	    FatalError (1, "CurrDoc = %d, number of documents = %d", 
			CurrDoc, idh.num_of_docs);
	  }	  

	  if (ifh.InvfLevel >= 2)
	    {
	      double weight;
	      GAMMA_DECODE (tf);
	      weight = tf * idf;
	      DocWeights[CurrDoc - 1] += weight * weight;
	    }
	}
      
      while (__btg)
	DECODE_BIT;
    }

  DECODE_DONE

  fclose (dict);
  fclose (invf);
  fprintf (stderr, "\n");

  /* [RPAP - Jan 97: Endian Ordering] */
  for (i = 0; i < NumPara; i++)
    HTONF(DocWeights[i]);

  f = create_file (file_name, WEIGHTS_SUFFIX, "wb", MAGIC_WGHT,
		   MG_ABORT);

  fwrite ((char *) DocWeights, sizeof (float), NumPara, f);
  fclose (f);
  Xfree (DocWeights);
}











void 
Make_weight_approx (void)
{
  int i, pos, max;
  unsigned long buf;
  double U, L, B;
  FILE *approx, *exact;

  exact = open_file (file_name, WEIGHTS_SUFFIX, "rb", MAGIC_WGHT,
		     MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  /* calculate U and L */
  L = 1e300;
  U = 0;
  for (i = 0; i < NumPara; i++)
    {
      float wgt;
      fread ((char *) &wgt, sizeof (wgt), 1, exact);
      NTOHF(wgt);  /* [RPAP - Jan 97: Endian Ordering] */
      wgt = sqrt (wgt);
      if (wgt > U)
	U = wgt;
      if (wgt > 0 && wgt < L)
	L = wgt;

    }
  fseek (exact, sizeof (u_long), SEEK_SET);

  B = pow (U / L, pow (2.0, -(double) bits));

  fprintf (stderr, "L = %f\n", L);
  fprintf (stderr, "U = %f\n", U);
  fprintf (stderr, "B = %f\n", B);



  approx = create_file (file_name, APPROX_WEIGHTS_SUFFIX, "wb",
			MAGIC_WGHT_APPROX, MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  fwrite ((char *) &bits, sizeof (bits), 1, approx);
  HTOND(L);  /* [RPAP - Jan 97: Endian Ordering] */
  HTOND(B);  /* [RPAP - Jan 97: Endian Ordering] */
  fwrite ((char *) &L, sizeof (L), 1, approx);
  fwrite ((char *) &B, sizeof (B), 1, approx);
  NTOHD(L);  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHD(B);  /* [RPAP - Jan 97: Endian Ordering] */

  max = bits == 32 ? 0xffffffff : (1 << bits) - 1;
  for (buf = pos = i = 0; i < NumPara; i++)
    {
      unsigned long fx;
      float wgt;
      fread ((char *) &wgt, sizeof (wgt), 1, exact);
      NTOHF(wgt);  /* [RPAP - Jan 97: Endian Ordering] */
      wgt = sqrt (wgt);
      if (wgt == 0)
	{
	  wgt = L;
#ifndef QUIET
	  Message ("Warning: Document %d had a weight of 0.", i);
#endif
	}
      fx = (int) floor (log (wgt / L) / log (B));

      if (fx > max)
	fx = max;

      buf |= (fx << pos);
      pos += bits;

      if (pos >= MAXBITS)
	{
	  HTONUL(buf);
	  fwrite ((char *) &buf, sizeof (buf), 1, approx);
	  buf = fx >> (bits - (pos - MAXBITS));
	  pos = pos - MAXBITS;
	}
    }
  if (pos > 0)
    {
      /* [RPAP - Jan 97: Endian Ordering] */
      HTONUL(buf);
      fwrite ((char *) &buf, sizeof (buf), 1, approx);
    }

  fclose (approx);
  fclose (exact);
}





void 
Make_text_idx_wgt (void)
{
  compressed_text_header cth;
  int i;
  FILE *idx_wgt, *idx, *para, *exact;

  idx_wgt = create_file (file_name, TEXT_IDX_WGT_SUFFIX, "wb", MAGIC_TEXI_WGT,
			 MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  /* [RJM 10/98 - Text Filename] */
  idx = open_file (text_file_name, TEXT_IDX_SUFFIX, "rb", MAGIC_TEXI,
		   MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */
  if (fread (&cth, sizeof (cth), 1, idx) != 1)
    FatalError (1, "Unable to read header of index file");

  /* [RPAP - Jan 97: Endian Ordering] */
  NTOHUL(cth.num_of_docs);
  NTOHD(cth.num_of_bytes); /* [RJM 07/97: 4G limit] */
  NTOHUL(cth.num_of_words);
  NTOHUL(cth.length_of_longest_doc);
  NTOHD(cth.ratio);

  exact = open_file (file_name, WEIGHTS_SUFFIX, "rb", MAGIC_WGHT,
		     MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */

  get_NumPara ();
  if (cth.num_of_docs != NumPara)
    {
      Message ("The number of documents %d does not equal "
	       "the number of paragraphs %d.", cth.num_of_docs, NumPara);
      Message ("Using the \"%s.invf.paragraph\" file\n", file_name);
      para = open_file (file_name, INVF_PARAGRAPH_SUFFIX, "rb", MAGIC_PARAGRAPH,
			MG_ABORT);  /* [RPAP - Feb 97: WIN32 Port] */
    }
  else
    para = NULL;

  {
    struct
      {
	unsigned long Start;
	float Weight;
      }
    data;
    for (i = 0; i < cth.num_of_docs; i++)
      {
	int count;
	fread ((char *) &data.Start, sizeof (unsigned long), 1, idx);
	if (para && i < cth.num_of_docs)
	  {
	    /* [RPAP - Jan 97: Endian Ordering] */
	    fread ((char *) &count, sizeof (count), 1, para);
	    NTOHSI(count);
	  }
	else
	  count = 1;
	while (count--)
	  {
	    fread ((char *) &data.Weight, sizeof (float), 1, exact);
	    NTOHF(data.Weight);  /* [RPAP - Jan 97: Endian Ordering] */
	    data.Weight = sqrt (data.Weight);
	    HTONF(data.Weight);  /* [RPAP - Jan 97: Endian Ordering] */
	    fwrite ((char *) &data, sizeof (data), 1, idx_wgt);
	  }
      }
    /* Write out the extra entry for the idx file */
    fread ((char *) &data.Start, sizeof (unsigned long), 1, idx);
    data.Weight = 0;
    fwrite((char*)&data, sizeof(data), 1, idx_wgt);
  }

  fclose (idx_wgt);
  fclose (idx);
  fclose (exact);
  if (para)
    fclose (para);
}
