/**************************************************************************
 *
 * mg_invf_merge.c -- description
 * Copyright (C) 1995  Shane Hudson (shane@cosc.canterbury.ac.nz)
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
 * Last edited:  31 March, 1995 
 * $Id$
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "locallib.h"
#include "local_strings.h"
#include "messages.h"
#include "timing.h"

#include "bitio_m.h"
#include "bitio_gen.h"
#include "bitio_stdio.h"

#include "mg.h"
#include "mg_merge.h"
#include "mg_files.h"
#include "invf.h"
#include "words.h"

typedef char FileName[256];


/*
 *  The merge_info struct contains all the data for an inverted file and
 *  lexicon. We declare 3 merge_info variables, m[OLD] m[NEW] and m[MERGE].
 */
typedef struct merge_info_type
  {
    FileName fname;
    FILE *invf;
    FILE *dict;
    FILE *idx;
    struct invf_dict_header idh;
    struct invf_file_header ifh;
    u_long nDocs;

    /* Lexicon processing variables */
    u_long fcnt, wcnt;
    int suff, pref;
    unsigned char term[MAXSTEMLEN + 1];
    u_long term_count;
    int done;

    /* Variables used for merging */
    struct stdio_bitio_state sbs;	/* for bit-based I/O on inverted files */
    u_long nTerms;		/* = number of invf entries */
    int *fcntlist;		/* store fcnt values read in from the lexicons */

  }
merge_info;


/***************************************************
 **** GLOBALS ****
 ***************************************************/

merge_info m[3];		/*  m[OLD], m[NEW] and m[MERGE] */
u_long magicsize;		/* how big is magic number? currently always 4 */
u_long Nstatic;			/* static # of documents in file */

float *DocWeightBuffer;
int weightOption = 0;		/* true if weights file is to be appended */

/*** Globals for processing the lexicons -- see readTerm() and writeTerm() 
 ***/
ProgTime start;			/* for printing elapsed time */
unsigned char prevTerm[MAXSTEMLEN + 1];

/*** Globals for merging -- see processEntry() 
 ***/
char *mergedata;		/* stores value OLD/NEW/MERGE for each term */
int fastMerge = 1;		/* True if fast merge selected. Default = true */

u_long oldIdxValue;		/* stores last read value from m[OLD].idx
				   Used to calculate entry lengths */
u_long bytes_output;		/* for writing .invf.idx file */
u_long oldOnlyBits = 0;		/* entries in old only */
u_long oldMergeBits = 0;	/* entries in old that are merged with 
				   a new entry */

/**************************************************************************/


/*=======================================================================
 * init_merge_invf(): open files, initialise globals, etc
 *=======================================================================*/
int 
init_merge_invf ()
{
  prevTerm[0] = prevTerm[1] = 0;

    /***
     *  open .dict files
     ***/
  m[OLD].dict = open_file (m[OLD].fname, INVF_DICT_SUFFIX, "r",
			   MAGIC_STEM_BUILD, MG_ABORT);
  magicsize = ftell (m[OLD].dict);
  fread (&(m[OLD].idh), sizeof (m[OLD].idh), 1, m[OLD].dict);
  m[NEW].dict = open_file (m[NEW].fname, INVF_DICT_SUFFIX, "r",
			   MAGIC_STEM_BUILD, MG_ABORT);
  fread (&(m[NEW].idh), sizeof (m[NEW].idh), 1, m[NEW].dict);

  m[MERGE].dict = create_file (m[MERGE].fname, INVF_DICT_SUFFIX, "w",
			       MAGIC_STEM_BUILD, MG_ABORT);

  /* write space for header */
  fwrite ((char *) &(m[OLD].idh), sizeof (m[OLD].idh), 1, m[MERGE].dict);

  m[OLD].nDocs = m[OLD].idh.num_of_docs;
  m[NEW].nDocs = m[NEW].idh.num_of_docs;
  Nstatic = m[OLD].idh.static_num_of_docs;
  m[OLD].nTerms = m[OLD].idh.dict_size;
  m[NEW].nTerms = m[NEW].idh.dict_size;
  m[MERGE].nDocs = m[OLD].nDocs + m[NEW].nDocs;
  m[MERGE].nTerms = 0;

  /* Set up weight buffer for weights of new documents */
  if (!(DocWeightBuffer = Xmalloc (m[NEW].nDocs * sizeof (*DocWeightBuffer))))
    {
      Message ("Insufficient memory\n");
      return COMPERROR;
    }
  bzero ((char *) DocWeightBuffer, m[NEW].nDocs * sizeof (*DocWeightBuffer));

    /***
     *  open .invf files
     *  Try ".ORG" extension first for m[OLD].invf since an inverted file
     *  with skips may have been created.
     ****/
  if (!(m[OLD].invf = open_file (m[OLD].fname, INVF_SUFFIX ".ORG", "r",
				 MAGIC_INVF, MG_CONTINUE)))
    m[OLD].invf = open_file (m[OLD].fname, INVF_SUFFIX, "r",
			     MAGIC_INVF, MG_ABORT);
  fread (&(m[OLD].ifh), sizeof (m[OLD].ifh), 1, m[OLD].invf);

  m[NEW].invf = open_file (m[NEW].fname, INVF_SUFFIX, "r",
			   MAGIC_INVF, MG_ABORT);
  fread (&(m[NEW].ifh), sizeof (m[NEW].ifh), 1, m[NEW].invf);

  if (m[OLD].ifh.skip_mode != 0)
    FatalError (1, "The old invf file contains skips. Unable to merge.");

  if (m[OLD].ifh.InvfLevel != m[NEW].ifh.InvfLevel)
    FatalError (1, "The two invf files have different inversion levels!");

  if ((m[OLD].ifh.InvfLevel > 2) || (m[NEW].ifh.InvfLevel > 2))
    FatalError (1, "mgmerge cannot handle level 3 inverted files!");

  m[MERGE].invf = create_file (m[MERGE].fname, INVF_SUFFIX, "w",
			       MAGIC_INVF, MG_ABORT);

  /* write space for header in inverted file */
  m[MERGE].ifh.no_of_words = m[MERGE].ifh.no_of_ptrs = 0;
  m[MERGE].ifh.skip_mode = 0;
  m[MERGE].ifh.InvfLevel = 2;
  {
    int i;
    for (i = 0; i < 16; i++)
      m[MERGE].ifh.params[i] = 0;
  }
  fwrite ((char *) &(m[MERGE].ifh), sizeof (m[MERGE].ifh), 1, m[MERGE].invf);

    /***
     *  open .invf.idx files
     ***/
  m[OLD].idx = open_file (m[OLD].fname, INVF_IDX_SUFFIX, "rb",
			  MAGIC_INVI, MG_ABORT);
  fread (&oldIdxValue, sizeof (u_long), 1, m[OLD].idx);

  m[NEW].idx = open_file (m[NEW].fname, INVF_IDX_SUFFIX, "rb",
			  MAGIC_INVI, MG_ABORT);

  m[MERGE].idx = create_file (m[MERGE].fname, INVF_IDX_SUFFIX, "wb",
			      MAGIC_INVI, MG_ABORT);

  return OK;
}


/*=======================================================================
 * procEntry_slow(): merge an entry, slow method
 *=======================================================================*/
void 
procEntry_slow (int tnum, int type, u_long oldN, u_long newN, u_long mergeN)
/* 
 *  Process an invf entry. 
 *  Slow Merge. (Always decode/re-encode).
 *
 *  tnum is the entry number, type = OLD or NEW or MERGE.
 *  oldN, newN and mergeN are the N parameters for Bblock coding.
 *  They are passed so other procEntry_fast can call this one
 *  when a decode/recode is needed to save repeating code.
 */
{
  int doc = 0;
  int fcntOLD = m[OLD].fcntlist[tnum];
  int fcntNEW = m[NEW].fcntlist[tnum];
  int inblk, outblk;
  u_long inbits = 0, outbits = 0;

  /* write .invf.idx pointer */
  fwrite (&bytes_output, sizeof (u_long), 1, m[MERGE].idx);

  outblk = BIO_Bblock_Init (mergeN, (fcntOLD + fcntNEW));

/*** OLD entry -> merge entry ***/
  if (type != NEW)
    {
      int i;
      inblk = BIO_Bblock_Init (oldN, fcntOLD);
      for (i = 0; i < fcntOLD; i++)
	{
	  int num;
	  num = BIO_Stdio_Bblock_Decode (inblk, &(m[OLD].sbs), &inbits);
	  doc += num;
	  BIO_Stdio_Bblock_Encode (num, outblk, &(m[MERGE].sbs), &outbits);
	  if (m[OLD].ifh.InvfLevel >= 2)	/* frequencies */
	    {
	      num = BIO_Stdio_Gamma_Decode (&(m[OLD].sbs), &inbits);
	      BIO_Stdio_Gamma_Encode (num, &(m[MERGE].sbs), &outbits);
	    }
	}
      if (type == OLD)
	oldOnlyBits += (inbits + m[OLD].sbs.Btg);
      else
	oldMergeBits += (inbits + m[OLD].sbs.Btg);
    }

/*** NEW entry -> merge entry ***/
  if (type != OLD)
    {
      int i;
      int offset;
      double logN = log ((double) m[MERGE].nDocs);
      double idf = logN - log ((double) (fcntOLD + fcntNEW));

      inblk = BIO_Bblock_Init (newN, fcntNEW);
      offset = (m[OLD].nDocs - doc);	/* amount to be added to 1ST pointer */
      for (i = 0; i < fcntNEW; i++)
	{
	  register int num;
	  register double weight;
	  num = BIO_Stdio_Bblock_Decode (inblk, &(m[NEW].sbs), &inbits);
	  if (i == 0)
	    num += offset;
	  doc += num;
	  BIO_Stdio_Bblock_Encode (num, outblk, &(m[MERGE].sbs), &outbits);
	  if (m[OLD].ifh.InvfLevel >= 2)	/* frequencies */
	    {
	      num = BIO_Stdio_Gamma_Decode (&(m[NEW].sbs), &inbits);
	      BIO_Stdio_Gamma_Encode (num, &(m[MERGE].sbs), &outbits);
	      weight = num * idf;
	      DocWeightBuffer[doc - m[OLD].nDocs - 1] += weight * weight;
	    }
	}
    }
/*** Now the padding bits ***/
  while (m[OLD].sbs.Btg)
    BIO_Stdio_Decode_Bit (&(m[OLD].sbs));
  while (m[NEW].sbs.Btg)
    BIO_Stdio_Decode_Bit (&(m[NEW].sbs));
  while (m[MERGE].sbs.Btg != 8)
    {
      BIO_Stdio_Encode_Bit (0, &(m[MERGE].sbs));
      outbits++;
    }
  bytes_output += (outbits >> 3);
  return;
}


/*=======================================================================
 * procEntry_fast(): merge an entry, WITHOUT decoding if it's in the
 *                   old inverted file only
 *=======================================================================*/
void 
procEntry_fast (int tnum, int type)
/*
 *  Merge an invf entry. 
 *  Faster Method : Copy (don't re-code) entries for terms only in IFold.
 *  The N parameter is Nstatic, from m[OLD].idh.static_num_of_docs.
 *  If entry type isnt OLD, just call procEntry_slow()
 */
{
  u_long newIdxValue, len = 0;

  /* Calculate IFold entry length in bytes if not NEW type */
  if (type != NEW)		/* read in an index number from m[OLD].idx */
    {
      fread (&newIdxValue, sizeof (u_long), 1, m[OLD].idx);
      len = newIdxValue - oldIdxValue;
      oldIdxValue = newIdxValue;
    }

  if (type != OLD)		/* must decode/recode */
    procEntry_slow (tnum, type, Nstatic, m[NEW].nDocs, Nstatic);
  else
    {
      /* write .invf.idx pointer */
      fwrite (&bytes_output, sizeof (u_long), 1, m[MERGE].idx);

      /* copy entry */
      {
	int i;
	char c;
	for (i = 0; i < len; i++)
	  {
	    c = fgetc (m[OLD].invf);
	    fputc (c, m[MERGE].invf);
	  }
      }
      bytes_output += len;
      oldOnlyBits += (len * 8);
    }
  return;
}


/*=======================================================================
 * processEntry(): merge an entry. Call either procEntry_slow() or
 *                 procEntry_fast.
 *=======================================================================*/
void 
processEntry (int tnum, int type)
/* 
 *  Read an entry from IFold and/or IFnew, and write to IFmerge.
 *  tnum is the term number, type is OLD or NEW or MERGE
 */
{
  if (fastMerge)
    procEntry_fast (tnum, type);
  else
    procEntry_slow (tnum, type, Nstatic, m[NEW].nDocs, m[MERGE].nDocs);
  return;
}


/*=======================================================================
 * readTerm()
 *=======================================================================*/
void 
readTerm (int x)
/*
 *  Read the next term from the appropriate ".invf.dict" file.
 *  x should be either OLD or NEW.
 *  Sets m[x].done to true when that lexicon has been completely read.
 */
{
  int i;

  if (m[x].term_count >= m[x].nTerms)
    {
      m[x].term[0] = 0;
      m[x].done = 1;
      return;
    }
  m[x].pref = fgetc (m[x].dict);
  m[x].suff = fgetc (m[x].dict);
  m[x].term[0] = m[x].pref + m[x].suff;

  for (i = 0; i < m[x].suff; i++)
    {
      m[x].term[m[x].pref + i + 1] = fgetc (m[x].dict);
    }
  m[x].term_count = m[x].term_count + 1;

  fread ((char *) &(m[x].fcnt), sizeof (u_long), 1, m[x].dict);
  fread ((char *) &(m[x].wcnt), sizeof (u_long), 1, m[x].dict);

  return;
}


/*=======================================================================
 * writeTerm()
 *=======================================================================*/
void 
writeTerm (int x)
/*
 *  Write the current term to m[MERGE].dict file.
 *  x is OLD, NEW or MERGE.
 */
{
  unsigned char i, prefix, suffix;
  if (x == MERGE)
    {
      m[MERGE].fcnt = m[OLD].fcnt + m[NEW].fcnt;
      m[MERGE].wcnt = m[OLD].wcnt + m[NEW].wcnt;
      m[MERGE].pref = m[OLD].pref;
      m[MERGE].suff = m[OLD].suff;
      memcpy (m[MERGE].term, m[OLD].term, m[OLD].term[0] + 1);
    }
  if (x == OLD)
    {
      m[MERGE].fcnt = m[OLD].fcnt;
      m[MERGE].wcnt = m[OLD].wcnt;
    }
  if (x == NEW)
    {
      m[MERGE].fcnt = m[NEW].fcnt;
      m[MERGE].wcnt = m[NEW].wcnt;
    }
  prefix = prefixlen (prevTerm, m[x].term);
  suffix = m[x].term[0] - prefix;
  fputc (prefix, m[MERGE].dict);	/* prefix length */
  fputc (suffix, m[MERGE].dict);	/* suffix length */
  for (i = 0; i < suffix; i++)
    fputc (m[x].term[prefix + i + 1], m[MERGE].dict);
  fwrite ((char *) &(m[MERGE].fcnt), sizeof (m[MERGE].fcnt), 1, m[MERGE].dict);
  fwrite ((char *) &(m[MERGE].wcnt), sizeof (m[MERGE].wcnt), 1, m[MERGE].dict);
  memcpy (prevTerm, m[x].term, prefix + suffix + 1);
  m[MERGE].idh.total_bytes += m[x].term[0] + 1;
  m[MERGE].idh.index_string_bytes += m[x].term[0] + 2 - prefix;
  fflush (m[MERGE].dict);
}


/*=======================================================================
 * process_merge_invf(): The main loop
 *=======================================================================*/
int 
process_merge_invf (void)
/*
 *  The main function to merge the lexicons and inverted files.
 */
{
/*** Some extra Initialisation stuff first ***/

  m[OLD].term[0] = m[NEW].term[0] = '\0';
  m[OLD].term_count = m[NEW].term_count = 0;
  m[OLD].done = m[NEW].done = 0;
  m[MERGE].idh.total_bytes = m[MERGE].idh.index_string_bytes = 0;

/*** malloc arrays: mergedata, m[OLD].fcntlist, m[NEW].fcntlist ***/
  mergedata = malloc ((m[OLD].nTerms + m[NEW].nTerms) * sizeof (char));
  if (mergedata == 0)
    {
      fprintf (stderr, "MALLOC error!\n");
      exit (1);
    }
  {
    int i;
    for (i = 0; i < 2; i++)
      {
	m[i].fcntlist = malloc ((m[OLD].nTerms + m[NEW].nTerms) * sizeof (int));
	if (m[i].fcntlist == 0)
	  {
	    fprintf (stderr, "MALLOC error!\n");
	    exit (1);
	  }
      }
  }

/*========================*/
/***    LEXICON PASS    ***/
/*========================*/

/*** read first terms ***/
  readTerm (OLD);
  readTerm (NEW);

  m[MERGE].nTerms = 0;
  while (m[OLD].done == 0 || m[NEW].done == 0)
    {
      int i;
      if (m[OLD].done)
	i = 1; /* NEW will always be greater */
      else if (m[NEW].done)
	i = -1; /* OLD will always be greater */
      else
	i = compare (m[OLD].term, m[NEW].term);

      if (i < 0)
	{			/* term in OLD only */
	  mergedata[m[MERGE].nTerms] = (char) OLD;
	  m[OLD].fcntlist[m[MERGE].nTerms] = m[OLD].fcnt;
	  m[NEW].fcntlist[m[MERGE].nTerms] = 0;
	  m[MERGE].nTerms++;
	  writeTerm (OLD);
	  readTerm (OLD);
	}
      if (i == 0)
	{			/* term in both lexions */
	  mergedata[m[MERGE].nTerms] = (char) MERGE;
	  m[OLD].fcntlist[m[MERGE].nTerms] = m[OLD].fcnt;
	  m[NEW].fcntlist[m[MERGE].nTerms] = m[NEW].fcnt;
	  m[MERGE].nTerms++;
	  writeTerm (MERGE);
	  readTerm (OLD);
	  readTerm (NEW);
	}
      if (i > 0)
	{			/* term in NEW only */
	  mergedata[m[MERGE].nTerms] = (char) NEW;
	  m[NEW].fcntlist[m[MERGE].nTerms] = m[NEW].fcnt;
	  m[OLD].fcntlist[m[MERGE].nTerms] = 0;
	  m[MERGE].nTerms++;
	  writeTerm (NEW);
	  readTerm (NEW);
	}
    }/*while*/
  Message ("%s\n", ElapsedTime (&start, NULL));
  /* print some results about terms */
  {
    fprintf (stderr, "   Terms: OLD = %ld,  NEW = %ld,  MERGE = %ld\n",
	     m[OLD].nTerms, m[NEW].nTerms, m[MERGE].nTerms);
    fprintf (stderr, "   In OLD only: %ld,  In NEW only: %ld,  BOTH: %ld\n",
	     (m[MERGE].nTerms - m[NEW].nTerms),
	     (m[MERGE].nTerms - m[OLD].nTerms),
	     (m[OLD].nTerms + m[NEW].nTerms - m[MERGE].nTerms));
  }


/*========================*/
/***      INVF PASS     ***/
/*========================*/

  bytes_output = ftell (m[MERGE].invf);
  BIO_Stdio_Decode_Start (m[OLD].invf, &m[OLD].sbs);
  BIO_Stdio_Decode_Start (m[NEW].invf, &m[NEW].sbs);
  BIO_Stdio_Encode_Start (m[MERGE].invf, &m[MERGE].sbs);
  {
    int i;
    for (i = 0; i < m[MERGE].nTerms; i++)
      {
	processEntry (i, mergedata[i]);
      }
    /* write final .invf.idx pointer */
    fwrite (&bytes_output, sizeof (u_long), 1, m[MERGE].idx);

    fprintf (stderr, "Old invf: old only bits: %ld, merged bits: %ld\n",
	     oldOnlyBits, oldMergeBits);
  }

  return OK;
}


/*=======================================================================
 * done_merge_invf(): write headers, close files, append weights to
 *                     weights file
 *=======================================================================*/
int 
done_merge_invf (void)
{
/*** write new lexicon header ***/
  m[MERGE].idh.dict_size = m[MERGE].nTerms;
  m[MERGE].idh.lookback = m[OLD].idh.lookback;
  m[MERGE].idh.input_bytes = m[OLD].idh.input_bytes + m[NEW].idh.input_bytes;
  m[MERGE].idh.total_bytes = m[OLD].idh.total_bytes + m[NEW].idh.total_bytes;
  m[MERGE].idh.num_of_docs = m[MERGE].nDocs;
  m[MERGE].idh.num_of_words = m[OLD].idh.num_of_words + m[NEW].idh.num_of_words;
  m[MERGE].idh.stem_method = m[OLD].idh.stem_method;

  /* if fastMerge, static num of docs stays the same! */
  if (fastMerge)
    m[MERGE].idh.static_num_of_docs = m[OLD].idh.static_num_of_docs;
  else
    m[MERGE].idh.static_num_of_docs = m[MERGE].nDocs;

  fseek (m[MERGE].dict, magicsize, 0);
  fwrite (&(m[MERGE].idh), sizeof (struct invf_dict_header), 1, m[MERGE].dict);

/*** write new inverted file header ***/
  m[MERGE].ifh.no_of_words = m[MERGE].nTerms;
  m[MERGE].ifh.no_of_ptrs = m[OLD].ifh.no_of_ptrs + m[NEW].ifh.no_of_ptrs;
  m[MERGE].ifh.skip_mode = m[OLD].ifh.skip_mode;
  m[MERGE].ifh.InvfLevel = m[OLD].ifh.InvfLevel;

  /* ifh.params[16] -- I have NO IDEA what these do!!!!! */
  /* only set if a inverted file is in skipped format, I think */
  {
    int i;
    for (i = 0; i < 16; i++)
      m[MERGE].ifh.params[i] = m[OLD].ifh.params[i];
  }

  fseek (m[MERGE].invf, magicsize, 0);	/* go to start of header */
  fwrite (&(m[MERGE].ifh), sizeof (struct invf_file_header), 1, m[MERGE].invf);

/*** Write new document weights --- leave old weights alone ***/
  if ((weightOption) && (m[OLD].ifh.InvfLevel >= 2))
    {
      FILE *weightfile = open_file (m[MERGE].fname, WEIGHTS_SUFFIX, "r+",
				    MAGIC_WGHT, MG_CONTINUE);
      if (weightfile)
	{
	  fprintf (stderr, "writing new weights: Nnew = %ld\n", m[NEW].nDocs);
	  fseek (weightfile, 0, 2);	/* end of file */
	  fwrite ((char *) DocWeightBuffer, sizeof (float), m[NEW].nDocs,
		  weightfile);
	}
      fclose (weightfile);
    }

/*** close files ***/
  fclose (m[MERGE].invf);
  fclose (m[OLD].invf);
  fclose (m[NEW].invf);
  fclose (m[MERGE].dict);
  fclose (m[OLD].dict);
  fclose (m[NEW].dict);
  fclose (m[MERGE].idx);
  fclose (m[OLD].idx);
  fclose (m[NEW].idx);

  return OK;
}


/*=======================================================================
 * usage()
 *=======================================================================*/
void 
usage (char *progname)
{
  fprintf (stderr, "usage:  %s [-s] [-w] [-d directory] -f mg_collection\n",
	   progname);
  fprintf (stderr, "  example: %s -s -w -f MERGE/mailfiles\n", progname);
  exit (1);
}


/*=======================================================================
 * main()
 *=======================================================================*/
int 
main (int argc, char *argv[])
{
  int ch;
  char *progname;
  progname = argv[0];
  m[MERGE].fname[0] = '\0';

  /* message and timing information */
  msg_prefix = argv[0];

  while ((ch = getopt (argc, argv, "f:d:swh")) != -1)
    switch (ch)
      {
      case 'f':
	strcpy (m[MERGE].fname, optarg);
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 's':
	fastMerge = 0;
	break;
      case 'w':
	weightOption = 1;
	break;
      case 'h':
      case '?':
      default:
	usage (progname);
      }

  if (m[MERGE].fname[0] == '\0')
    usage (progname);
  strcpy (m[OLD].fname, m[MERGE].fname);
  strcat (m[OLD].fname, ".old");
  strcpy (m[NEW].fname, m[MERGE].fname);
  strcat (m[NEW].fname, ".new");

  GetTime (&start);
  init_merge_invf ();
  process_merge_invf ();
  done_merge_invf ();
  Message ("%s\n", ElapsedTime (&start, NULL));
  exit (0);
}

/*************************************************************************/
/*   EOF: mg_invf_merge_new.c                                            */
/*************************************************************************/
