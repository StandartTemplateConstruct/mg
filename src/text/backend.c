/**************************************************************************
 *
 * backend.c -- Underlying routines for mgquery
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
 * $Id: backend.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "messages.h"
#include "timing.h"
#include "filestats.h"
#include "sptree.h"
#include "netorder.h"  /* [RPAP - Jan 97: Endian Ordering] */


#include "mg_files.h"
#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"
#include "stem_search.h"
#include "invf_get.h"
#include "text_get.h"
#include "weights.h"
#include "locallib.h"
#include "mg_errors.h"


static File *
OpenFile (char *base, char *suffix, unsigned long magic, int *ok)
{
  char FileName[512];
  File *F;
  sprintf (FileName, "%s%s", base, suffix);
  if (!(F = Fopen (FileName, "rb", 0)))  /* [RPAP - Feb 97: WIN32 Port] */
    {
      mg_errno = MG_NOFILE;
      MgErrorData (FileName);
      if (ok)
	*ok = 0;
      return (NULL);
    }
  if (magic)
    {
      unsigned long m;
      if (fread ((char *) &m, sizeof (m), 1, F->f) == 0)
	{
	  mg_errno = MG_READERR;
	  MgErrorData (FileName);
	  if (ok)
	    *ok = 0;
	  Fclose (F);
	  return (NULL);
	}
      NTOHUL(m);  /* [RPAP - Jan 97: Endian Ordering] */
      if (m != magic)
	{
	  mg_errno = MG_BADMAGIC;
	  MgErrorData (FileName);
	  if (ok)
	    *ok = 0;
	  Fclose (F);
	  return (NULL);
	}
    }
  return (F);
}


static int 
open_all_files (query_data * qd)
{
  int ok = 1;

  qd->File_text = OpenFile (qd->textpathname, TEXT_SUFFIX, /* [RJM 06/97: text filename] */
			    MAGIC_TEXT, &ok);
  qd->File_fast_comp_dict = OpenFile (qd->textpathname, /* [RJM 06/97: text filename] */
				      TEXT_DICT_FAST_SUFFIX, MAGIC_FAST_DICT, NULL);
  if (!qd->File_fast_comp_dict)
    {
      qd->File_comp_dict = OpenFile (qd->textpathname, /* [RJM 06/97: text filename] */
				     TEXT_DICT_SUFFIX, MAGIC_DICT, &ok);
      qd->File_aux_dict = OpenFile (qd->textpathname, /* [RJM 06/97: text filename] */
				    TEXT_DICT_AUX_SUFFIX, MAGIC_AUX_DICT, NULL);
    }
  else
    qd->File_comp_dict = qd->File_aux_dict = NULL;

  qd->File_stem = OpenFile (qd->pathname, INVF_DICT_BLOCKED_SUFFIX,
			    MAGIC_STEM, &ok);

  /* [RPAP - Jan 97: Stem Index Change]
     These will fail if collection not built with stem indexes */
  qd->File_stem1 = OpenFile (qd->pathname, INVF_DICT_BLOCKED_1_SUFFIX,
			     MAGIC_STEM_1, NULL);
  qd->File_stem2 = OpenFile (qd->pathname, INVF_DICT_BLOCKED_2_SUFFIX,
			     MAGIC_STEM_2, NULL);
  qd->File_stem3 = OpenFile (qd->pathname, INVF_DICT_BLOCKED_3_SUFFIX,
			     MAGIC_STEM_3, NULL);

  qd->File_invf = OpenFile (qd->pathname, INVF_SUFFIX,
			    MAGIC_INVF, &ok);

  /* These will fail if a level 1 inverted file was created because there 
     will be no document weights */
  qd->File_text_idx_wgt = OpenFile (qd->pathname, TEXT_IDX_WGT_SUFFIX,
				    MAGIC_TEXI_WGT, NULL);
  qd->File_weight_approx = OpenFile (qd->pathname, APPROX_WEIGHTS_SUFFIX,
				     MAGIC_WGHT_APPROX, NULL);
  if (qd->File_text_idx_wgt == NULL && qd->File_weight_approx == NULL)
    qd->File_text_idx = OpenFile (qd->textpathname, /* [RJM 06/97: text filename] */
				  TEXT_IDX_SUFFIX, MAGIC_TEXI, NULL);
  else
    qd->File_text_idx = NULL;


  if (!ok)
    {
      Fclose (qd->File_text);
      if (qd->File_fast_comp_dict)
	Fclose (qd->File_fast_comp_dict);
      if (qd->File_comp_dict)
	Fclose (qd->File_comp_dict);
      Fclose (qd->File_stem);

      /* [RPAP - Jan 97: Stem Index Change] */
      if (qd->File_stem1)
	Fclose (qd->File_stem1);
      if (qd->File_stem2)
	Fclose (qd->File_stem2);
      if (qd->File_stem3)
	Fclose (qd->File_stem3);

      Fclose (qd->File_invf);
      if (qd->File_text_idx_wgt)
	Fclose (qd->File_text_idx_wgt);
      if (qd->File_weight_approx)
	Fclose (qd->File_weight_approx);
      if (qd->File_text_idx)
	Fclose (qd->File_text_idx);
      return (-1);
    }
  return (0);

}

static void 
close_all_files (query_data * qd)
{
  Fclose (qd->File_text);
  if (qd->File_fast_comp_dict)
    Fclose (qd->File_fast_comp_dict);
  if (qd->File_aux_dict)
    Fclose (qd->File_aux_dict);
  if (qd->File_comp_dict)
    Fclose (qd->File_comp_dict);
  Fclose (qd->File_stem);

  /* [RPAP - Jan 97: Stem Index Change] */
  if (qd->File_stem1)
    Fclose (qd->File_stem1);
  if (qd->File_stem2)
    Fclose (qd->File_stem2);
  if (qd->File_stem3)
    Fclose (qd->File_stem3);

  Fclose (qd->File_invf);
  if (qd->File_text_idx_wgt)
    Fclose (qd->File_text_idx_wgt);
  if (qd->File_weight_approx)
    Fclose (qd->File_weight_approx);
  if (qd->File_text_idx)
    Fclose (qd->File_text_idx);
}

/* If textname is equal to null then name will be used instead */
/* [RJM 06/97: text filename] */
query_data *
InitQuerySystem (char *dir, char *name, char *textname, InitQueryTimes * iqt)
{
  query_data *qd;
  char *s;

  if (textname == NULL) textname = name; /* [RJM 06/97: text filename] */

  if (!(qd = Xmalloc (sizeof (query_data))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  bzero ((char *) qd, sizeof (*qd));

  qd->mem_in_use = qd->max_mem_in_use = 0;

  qd->doc_pos = qd->buf_in_use = 0;
  qd->TextBufferLen = 0;
  qd->DL = NULL;

  /* [RPAP - Feb 97: Term Frequency] */
  qd->TL = NULL;
  qd->QTL = NULL;

  qd->TextBuffer = NULL;

  qd->tot_hops_taken = 0;
  qd->tot_num_of_ptrs = 0;
  qd->tot_num_of_accum = 0;
  qd->tot_num_of_terms = 0;
  qd->tot_num_of_ans = 0;
  qd->tot_text_idx_lookups = 0;

  qd->hops_taken = 0;
  qd->num_of_ptrs = 0;
  qd->num_of_accum = 0;
  qd->num_of_terms = 0;
  qd->num_of_ans = 0;
  qd->text_idx_lookups = 0;

  qd->pathname = NULL; /* RJM 06/97: text filename] */
  qd->textpathname = NULL; /* RJM 06/97: text filename] */

  s = strrchr (dir, '/');
  if (s && *(s + 1) == '\0')
    {
      /* [RJM 06/97: text filename] */
      if (!(qd->pathname = Xmalloc (strlen (dir) + strlen (name) + 1)) ||
	  !(qd->textpathname = Xmalloc (strlen (dir) + strlen (textname) + 1)))
	{
	  mg_errno = MG_NOMEM;
	  if (qd->pathname) Xfree (qd->pathname); /* [RJM 06/97: text filename] */
	  Xfree (qd);
	  return (NULL);
	}
      sprintf (qd->pathname, "%s%s", dir, name);
      sprintf (qd->textpathname, "%s%s", dir, textname); /* [RJM 06/97: text filename] */
    }

  else
    {
      /* [RJM 06/97: text filename] */
      if (!(qd->pathname = Xmalloc (strlen (dir) + strlen (name) + 2)) ||
	  !(qd->textpathname = Xmalloc (strlen (dir) + strlen (textname) + 2)))
	{
	  mg_errno = MG_NOMEM;
	  if (qd->pathname) Xfree (qd->pathname); /* [RJM 06/97: text filename] */
	  Xfree (qd);
	  return (NULL);
	}
/* [RPAP - Feb 97: WIN32 Port] */
#ifdef __WIN32__
      if (dir == NULL || dir[0] == '\0') {
	sprintf (qd->pathname, "%s", name);
	sprintf (qd->textpathname, "%s", textname); /* [RJM 06/97: text filename] */
      } else {
	sprintf (qd->pathname, "%s%s", dir, name);
	sprintf (qd->textpathname, "%s%s", dir, textname); /* [RJM 06/97: text filename] */
      }
#else
      sprintf (qd->pathname, "%s/%s", dir, name);
      sprintf (qd->textpathname, "%s/%s", dir, textname); /* [RJM 06/97: text filename] */
#endif
    }

  if (open_all_files (qd) == -1)
    {
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      return (NULL);
    }

  if (iqt)
    GetTime (&iqt->Start);

  /* Initialise the stemmed dictionary system */
  if (!(qd->sd = ReadStemDictBlk (qd->File_stem)))
    {
      close_all_files (qd);
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      return (NULL);
    }

  /* [RPAP - Jan 97: Stem Index Change] */
  if ((qd->sd->sdh.indexed & 7) && qd->File_stem1 && qd->File_stem2 && qd->File_stem3)
    {
      if (!(qd->sd->stem1 = ReadStemIdxBlk (qd->File_stem1)))
	{
	  FreeStemDict (qd->sd);
	  close_all_files (qd);
	  Xfree (qd->pathname);
	  Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
	  Xfree (qd);
	  return (NULL);
	}
      if (!(qd->sd->stem2 = ReadStemIdxBlk (qd->File_stem2)))
	{
	  FreeStemDict (qd->sd);
	  close_all_files (qd);
	  Xfree (qd->pathname);
	  Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
	  Xfree (qd);
	  return (NULL);
	}
      if (!(qd->sd->stem3 = ReadStemIdxBlk (qd->File_stem3)))
	{
	  FreeStemDict (qd->sd);
	  close_all_files (qd);
	  Xfree (qd->pathname);
	  Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
	  Xfree (qd);
	  return (NULL);
	}
      }
  else if (qd->sd->sdh.indexed != 0)
    {
      FreeStemDict (qd->sd);
      close_all_files (qd);
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      return (NULL);
    }
  else
    {
      if (qd->File_stem1)
	Fclose (qd->File_stem1);
      if (qd->File_stem2)
	Fclose (qd->File_stem2);
      if (qd->File_stem3)
	Fclose (qd->File_stem3);
      qd->File_stem1 = NULL;
      qd->File_stem2 = NULL;
      qd->File_stem3 = NULL;
      qd->sd->stem1 = NULL;
      qd->sd->stem2 = NULL;
      qd->sd->stem3 = NULL;
    }

  if (iqt)
    GetTime (&iqt->StemDict);
  if (qd->File_weight_approx)
    {
      if (!(qd->awd = LoadDocWeights (qd->File_weight_approx,
				      qd->sd->sdh.num_of_docs)))
	{
	  FreeStemDict (qd->sd);
	  close_all_files (qd);
	  Xfree (qd->pathname);
	  Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
	  Xfree (qd);
	  return (NULL);
	}
    }
  else
    qd->awd = NULL;


  if (iqt)
    GetTime (&iqt->ApproxWeights);

  if (!(qd->cd = LoadCompDict (qd->File_comp_dict, qd->File_aux_dict,
			       qd->File_fast_comp_dict)))
    {
      if (qd->awd)
	FreeWeights (qd->awd);
      FreeStemDict (qd->sd);
      close_all_files (qd);
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      return (NULL);
    }

  if (iqt)
    GetTime (&iqt->CompDict);

  if (!(qd->id = InitInvfFile (qd->File_invf, qd->sd)))
    {
      FreeCompDict (qd->cd);
      if (qd->awd)
	FreeWeights (qd->awd);
      FreeStemDict (qd->sd);
      close_all_files (qd);
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      return (NULL);
    }
  if ((qd->File_text_idx_wgt == NULL || qd->File_weight_approx == NULL) &&
      qd->id->ifh.InvfLevel >= 2)
    {
      FreeInvfData (qd->id);
      FreeCompDict (qd->cd);
      if (qd->awd)
	FreeWeights (qd->awd);
      FreeStemDict (qd->sd);
      close_all_files (qd);
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      mg_errno = MG_INVERSION;
      return (NULL);
    }
  if (iqt)
    GetTime (&iqt->Invf);

  if (!(qd->td = LoadTextData (qd->File_text, qd->File_text_idx_wgt,
			       qd->File_text_idx)))
    {
      FreeInvfData (qd->id);
      FreeCompDict (qd->cd);
      if (qd->awd)
	FreeWeights (qd->awd);
      FreeStemDict (qd->sd);
      close_all_files (qd);
      Xfree (qd->pathname);
      Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
      Xfree (qd);
      return (NULL);
    }

/* [RPAP - Feb 97: NZDL Additions] */
#if defined(PARADOCNUM) || defined(NZDL)

/*

This code is based on the TREC_MODE code below to read the .paragraph
file to determine what document numbers correspond to what paragraphs.
This code is more space efficient, reading in the .paragraph file
into memory as an accumulate docnum array.  Eg.  the .paragraph may contain

	[5 3 6 4 7 9 4]

indicating the first document has 5 paragraphs, the next 3, etc.
This will be stored in memory as

	[0 5 8 14 18 25 34 38]

so a binary search can be performed.  The first 0 is for convenience;
it prevents testing boundary conditions.


The TREC_MODE code does this differently; it stores the array

	[1 1 1 1 1 2 2 2 3 3 3 3 3 3 ....]

allowing directy paragraph to docnum conversion, at the expense
of memory.

*/ 
  qd->paragraph = NULL;

  if (qd->id->ifh.InvfLevel == 3)
    {
      unsigned long magic;
      FILE *paragraph;
      int i;
      char paraFile[512];

      sprintf(paraFile, "%s%s", qd->pathname, INVF_PARAGRAPH_SUFFIX);
      paragraph = fopen(paraFile, "rb");
      if (!paragraph)
	FatalError(1, "Unable to open 'paraFile'.", paraFile);

      fread((void *)&magic, sizeof(magic), 1, paragraph);
      qd->paragraph = Xmalloc((qd->td->cth.num_of_docs+1)*sizeof(int));
      qd->paragraph[0] = 0;
      for (i = 1; i <= qd->td->cth.num_of_docs; i++)
	{
	  int count;

	  if (fread((void *)&count, sizeof(count), 1, paragraph) != 1)
	    FatalError(1, "Unexpected EOF while reading '%s'.", paraFile);
	  NTOHSI(count);  /* [RPAP - Jan 97: Endian Ordering] */
	  qd->paragraph[i] = qd->paragraph[i-1]+count;
	}

      fclose (paragraph); /* [RJM 07/98: Memory Leak] */
    }
  
  
#endif

#ifdef TREC_MODE
  {
    extern char *trec_ids;
    extern long *trec_paras;
    int size;
    char FileName[512];
    FILE *f;
    if (!strstr (qd->pathname, "trec"))
      goto error;
    sprintf (FileName, "%s%s", qd->pathname, ".DOCIDS");
    if (!(f = fopen (FileName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
      {
	Message ("Unable to open \"%s\"", FileName);
	goto error;
      }
    fseek (f, 0, 2);
    size = ftell (f);
    fseek (f, 0, 0);
    trec_ids = Xmalloc (size);
    if (!trec_ids)
      {
	fclose (f);
	goto error;
      }
    fread (trec_ids, 1, size, f);
    fclose (f);
    if (qd->id->ifh.InvfLevel == 3)
      {
	int i, d;
	unsigned long magic;
	trec_paras = Xmalloc (qd->sd->sdh.num_of_docs * sizeof (long));
	if (!trec_paras)
	  {
	    Xfree (trec_ids);
	    trec_ids = NULL;
	    goto error;
	  }
	sprintf (FileName, "%s%s", qd->pathname, INVF_PARAGRAPH_SUFFIX);
	if (!(f = fopen (FileName, "rb")))  /* [RPAP - Feb 97: WIN32 Port] */
	  {
	    Message ("Unable to open \"%s\"", FileName);
	    goto error;
	  }
	if (fread ((char *) &magic, sizeof (magic), 1, f) != 1 ||
	    NTOHUL(magic) != MAGIC_PARAGRAPH)  /* [RPAP - Jan 97: Endian Ordering] */
	  {
	    fclose (f);
	    Message ("Bad magic number in \"%s\"", FileName);
	    goto error;
	  }

	for (d = i = 0; i < qd->td->cth.num_of_docs; i++)
	  {
	    int count;
	    if (fread ((char *) &count, sizeof (count), 1, f) != 1)
	      {
		fclose (f);
		goto error;
	      }
	    NTOHSI(count);  /* [RPAP - Jan 97: Endian Ordering] */
	    while (count--)
	      trec_paras[d++] = i;
	  }
	fclose (f);
      }
    goto ok;
  error:
    if (trec_ids)
      Xfree (trec_ids);
    if (trec_paras)
      Xfree (trec_paras);
    trec_ids = NULL;
    trec_paras = NULL;
  ok:
    ;
  }
#endif

  if (iqt)
    GetTime (&iqt->Text);

  return (qd);
}






/*
 * Change the amount of memory currently in use
 *
 */
void 
ChangeMemInUse (query_data * qd, long delta)
{
  qd->mem_in_use += delta;
  if (qd->mem_in_use > qd->max_mem_in_use)
    qd->max_mem_in_use = qd->mem_in_use;
}


void 
FinishQuerySystem (query_data * qd)
{
/* [RJM 07/98: Memory Leak] */
#if defined(PARADOCNUM) || defined(NZDL)
  if (qd->paragraph != NULL) {
    Xfree (qd->paragraph);
    qd->paragraph = NULL;
  }
#endif

  FreeTextData (qd->td);
  FreeInvfData (qd->id);
  FreeCompDict (qd->cd);
  if (qd->awd)
    FreeWeights (qd->awd);
  FreeStemDict (qd->sd);
  close_all_files (qd);
  Xfree (qd->textpathname); /* [RJM 06/97: text filename] */
  Xfree (qd->pathname);
  FreeQueryDocs (qd);
  if (qd->TL != NULL) FreeTermList(&qd->TL);         /* [RJM 07/98: Memory Leak] */
  if (qd->QTL != NULL) FreeQueryTermList(&qd->QTL);  /* [RJM 07/98: Memory Leak] */
  Xfree (qd);

  /* other global stuff hanging around */
  MgErrorDeinit ();
}


void 
ResetFileStats (query_data * qd)
{
  ZeroFileStats (qd->File_text);
  if (qd->File_comp_dict)
    ZeroFileStats (qd->File_comp_dict);
  if (qd->File_fast_comp_dict)
    ZeroFileStats (qd->File_fast_comp_dict);
  ZeroFileStats (qd->File_stem);

  /* [RPAP - Jan 97: Stem Index Change] */
  if (qd->File_stem1)
    ZeroFileStats (qd->File_stem1);
  if (qd->File_stem2)
    ZeroFileStats (qd->File_stem2);
  if (qd->File_stem3)
    ZeroFileStats (qd->File_stem3);

  ZeroFileStats (qd->File_invf);
  if (qd->File_text_idx_wgt)
    ZeroFileStats (qd->File_text_idx_wgt);
  if (qd->File_weight_approx)
    ZeroFileStats (qd->File_weight_approx);
  if (qd->File_text_idx)
    ZeroFileStats (qd->File_text_idx);
}


void 
TransFileStats (query_data * qd)
{
  qd->File_text->Current = qd->File_text->Cumulative;
  if (qd->File_comp_dict)
    qd->File_comp_dict->Current = qd->File_comp_dict->Cumulative;
  if (qd->File_fast_comp_dict)
    qd->File_fast_comp_dict->Current = qd->File_fast_comp_dict->Cumulative;
  qd->File_stem->Current = qd->File_stem->Cumulative;

  /* [RPAP - Jan 97: Stem Index Change] */
  if (qd->File_stem1)
    qd->File_stem1->Current = qd->File_stem1->Cumulative;
  if (qd->File_stem2)
    qd->File_stem2->Current = qd->File_stem2->Cumulative;
  if (qd->File_stem3)
    qd->File_stem3->Current = qd->File_stem3->Cumulative;

  qd->File_invf->Current = qd->File_invf->Cumulative;
  if (qd->File_text_idx_wgt)
    qd->File_text_idx_wgt->Current = qd->File_text_idx_wgt->Cumulative;
  if (qd->File_weight_approx)
    qd->File_weight_approx->Current = qd->File_weight_approx->Cumulative;
  if (qd->File_text_idx)
    qd->File_text_idx->Current = qd->File_text_idx->Cumulative;
}


void 
FreeTextBuffer (query_data * qd)
{
  if (qd->TextBuffer)
    {
      Xfree (qd->TextBuffer);
      ChangeMemInUse (qd, -qd->TextBufferLen);
    }
  qd->TextBuffer = NULL;
  qd->TextBufferLen = 0;
}

void 
FreeQueryDocs (query_data * qd)
{
  qd->doc_pos = 0;
  qd->buf_in_use = 0;
  if (qd->DL)
    {
      int i;
      for (i = 0; i < qd->DL->num; i++)
	if (qd->DL->DE[i].CompTextBuffer)
	  {
	    Xfree (qd->DL->DE[i].CompTextBuffer);
	    qd->DL->DE[i].CompTextBuffer = NULL;
	    ChangeMemInUse (qd, -qd->DL->DE[i].Len);
	  }
      Xfree (qd->DL);
    }
  qd->DL = NULL;
  FreeTextBuffer (qd);
}

int 
LoadCompressedText (query_data * qd, int max_mem)
{
  DocEntry *DE;
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return -1;

  DE = &qd->DL->DE[qd->doc_pos];
  if (!DE->CompTextBuffer)
    {
      int i;
      DocEntry *de;
      for (i = 0, de = qd->DL->DE; i < qd->DL->num; i++, de++)
	if (de->CompTextBuffer)
	  {
	    Xfree (de->CompTextBuffer);
	    de->CompTextBuffer = NULL;
	    ChangeMemInUse (qd, -de->Len);
	  }
      if (LoadBuffers (qd, &qd->DL->DE[qd->doc_pos], max_mem,
		       qd->DL->num - qd->doc_pos) == -1)
	return -1;
    }
  return 0;
}

int 
GetDocNum (query_data * qd)
{
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return -1;
  return qd->DL->DE[qd->doc_pos].DocNum;
}

DocEntry *
GetDocChain (query_data * qd)
{
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return NULL;
  return &(qd->DL->DE[qd->doc_pos]);
}

float 
GetDocWeight (query_data * qd)
{
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return -1;
  return qd->DL->DE[qd->doc_pos].Weight;
}

long 
GetDocCompLength (query_data * qd)
{
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return -1;
  return qd->DL->DE[qd->doc_pos].Len;
}


u_char *
GetDocText (query_data * qd, unsigned long *len)
{
  DocEntry *DE;
  int ULen;
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return NULL;

  DE = &qd->DL->DE[qd->doc_pos];

  if (!DE->CompTextBuffer)
    {
      fprintf (stderr, "The compressed text buffer is NULL\n");
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  FreeTextBuffer (qd);

  qd->TextBufferLen = (int) (qd->td->cth.ratio * 1.01 *
			     DE->Len) + 100;
  if (!(qd->TextBuffer = Xmalloc (qd->TextBufferLen)))
    {
      fprintf (stderr, "No memory for TextBuffer\n");
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  DecodeText (qd->cd, (u_char *) (DE->CompTextBuffer), DE->Len,
	      (u_char *) (qd->TextBuffer), &ULen);
  qd->TextBuffer[ULen] = '\0';

  if (ULen >= qd->TextBufferLen)
    {
      fprintf (stderr, "%d >= %d\n", ULen, qd->TextBufferLen);
      mg_errno = MG_BUFTOOSMALL;
      return (NULL);
    }

  if (len)
    *len = ULen;

  return qd->TextBuffer;
}

int 
NextDoc (query_data * qd)
{
  if (qd->DL == NULL || qd->doc_pos >= qd->DL->num)
    return 0;
  qd->doc_pos++;
  return qd->doc_pos < qd->DL->num;
}
