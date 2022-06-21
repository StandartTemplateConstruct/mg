/**************************************************************************
 *
 * query.ranked.c -- Ranked query evaluation
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
 * $Id: query.ranked.c,v 1.4 1994/11/25 03:47:46 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "memlib.h"
#include "filestats.h"
#include "messages.h"
#include "timing.h"
#include "sptree.h"

#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"
#include "stem_search.h"
#include "weights.h"
#include "text_get.h"
#include "invf_get.h"
#include "words.h"
#include "stemmer.h"
#include "locallib.h"
#include "environment.h"
#include "term_lists.h"
#include "local_strings.h"

/*
   $Log: query.ranked.c,v $
   * Revision 1.4  1994/11/25  03:47:46  tes
   * Committing files before adding the merge stuff.
   *
   * Revision 1.3  1994/10/20  03:57:03  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:04  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: query.ranked.c,v 1.4 1994/11/25 03:47:46 tes Exp $";

/*************************************************************************/

typedef struct HeapEntry
  {
    float Weight;
    float *OrgWeight;
    int DocNum;
    u_long SeekPos;		/* position in the text file in bytes */
    u_long Len;			/* length of the document in bytes */
  }
HeapEntry;

typedef int (*HeapComp) (HeapEntry *, HeapEntry *);

typedef struct Heap
  {
    int NumItems;
    int MaxSize;
    HeapComp HC;
    HeapEntry HE[1];
  }
Heap;


Heap *
Heap_Make (int size, HeapComp hc)
{
  Heap *H;
  H = Xmalloc (sizeof (Heap) + (size - 1) * sizeof (HeapEntry));
  if (!H)
    return NULL;
  H->NumItems = 0;
  H->MaxSize = size;
  H->HC = hc;
  return H;
}

int 
Heap_Size (Heap * H)
{
  return sizeof (Heap) + H->MaxSize * sizeof (HeapEntry);
}

HeapEntry *
Heap_GetHead (Heap * H)
{
  if (H && H->NumItems)
    return &H->HE[0];
  else
    return NULL;
}



void 
Heap_Heapify (Heap * H, int i)
{
  register int curr, child;
  curr = i;
  child = curr * 2;
  while (child <= H->NumItems)
    {
      if (child < H->NumItems && H->HC (&H->HE[child], &H->HE[child - 1]) > 0)
	child++;
      if (H->HC (&H->HE[curr - 1], &H->HE[child - 1]) < 0)
	{
	  HeapEntry temp = H->HE[child - 1];
	  H->HE[child - 1] = H->HE[curr - 1];
	  H->HE[curr - 1] = temp;
	  curr = child;
	  child = 2 * child;
	}
      else
	break;
    }
}


void 
Heap_Build (Heap * H)
{
  register int i;
  for (i = H->NumItems / 2; i > 0; i--)
    Heap_Heapify (H, i);
}


void 
Heap_Sort (Heap * H)
{
  register int i;
  for (i = H->NumItems; i > 1; i--)
    {
      HeapEntry temp = H->HE[0];
      H->HE[0] = H->HE[i - 1];
      H->HE[i - 1] = temp;
      H->NumItems--;
      Heap_Heapify (H, 1);
    }
}


void 
Heap_DeleteHead (Heap * H)
{
  H->HE[0] = H->HE[--H->NumItems];
  Heap_Heapify (H, 1);
}

int 
Heap_Lesser (HeapEntry * a, HeapEntry * b)
{
  return (a->Weight > b->Weight ? -1 :
	  (a->Weight == b->Weight ? 0 : 1));
}


int 
Heap_Greater (HeapEntry * a, HeapEntry * b)
{
  return (a->Weight > b->Weight ? 1 :
	  (a->Weight == b->Weight ? 0 : -1));
}

int 
Make_Exact_Root (query_data * qd, Heap * H)
{
  int num = 0;
  HeapEntry *he = H->HE;
  while (he->SeekPos == 0)
    {
      he->Weight = he->Weight *
	GetLowerApproxDocWeight (qd->awd, he->DocNum - 1) /
	FetchDocStart (qd, he->DocNum, &he->SeekPos, &he->Len);
      Heap_Heapify (H, 1);
      num++;
    }
  return num;
}

void 
Heap_Dump (Heap * H, int num)
{
  int i, l, lines, r;
  if (num > H->NumItems)
    num = H->NumItems;
  lines = (num + 3) / 4;
  for (l = 0; l < lines; l++)
    for (r = 0; r < 4; r++)
      {
	i = lines * r + l;
	if (i < num)
	  fprintf (stderr, "[%2d] %7.4f", i, H->HE[i].Weight);
	fprintf (stderr, r == 3 ? "\n" : "        ");
      }
  fprintf (stderr, "\n");
}

/*************************************************************************/






int 
doc_count_comp (const void *A, const void *B)
{
  const TermEntry *a = A;
  const TermEntry *b = B;
  return (a->WE.doc_count - b->WE.doc_count);
}

/* =========================================================================
 * Function: ParseRankedQuery
 * Description:
 *      Takes a string query line and extracts the terms in the query
 *      which exist in the stemmed dictionary.
 *      Optionally sorts the terms into ascending order by doc. number.
 * Input:
 *      stemmed dictionary, query line, sort-flag
 * Output:
 *      A list of terms.
 * ========================================================================= */


static TermList *
ParseRankedQuery (stemmed_dict * sd, char *QueryLine, int Sort)
{
  u_char Word[MAXSTEMLEN + 1];
  u_char *end, *s_in;
  TermList *Terms = MakeTermList (0);
  s_in = (u_char *) QueryLine;
  end = s_in + strlen ((char *) s_in) - 1;


  /* find the start of the first word */
  if (!INAWORD (*s_in))
    PARSE_NON_STEM_WORD (s_in, end);

  while (s_in <= end)
    {
      int j;
      long word_num;
      unsigned long count, doc_count, invf_ptr, invf_len;

      /* Get a word and stem it */
      PARSE_STEM_WORD (Word, s_in, end);
      stemmer (sd->sdh.stem_method, Word);

      /* Skip over the non word separator */
      PARSE_NON_STEM_WORD (s_in, end);

      /* Look for the word in the already identified terms */
      for (j = 0; j < Terms->num; j++)
	if (compare (Terms->TE[j].Word, Word) == 0)
	  break;

      /* Increment the weight if the word is in the list */
      if (j < Terms->num)
	Terms->TE[j].Count++;
      else
	{

	  /* Look for it in the stemmed dictionary */
	  if ((word_num = FindWord (sd, Word, &count, &doc_count,
				    &invf_ptr, &invf_len)) != -1)
	    {
	      /* Search the list for the word */
	      for (j = 0; j < Terms->num; j++)
		if (Terms->TE[j].WE.word_num == word_num)
		  break;

	      /* Increment the weight if the word is in the list */
	      if (j < Terms->num)
		Terms->TE[j].Count++;
	      else
		/* Create a new entry in the list for the new word */
		{
		  /* Create a new entry in the list for the new word */
		  TermEntry te;

		  te.WE.word_num = word_num;
		  te.WE.count = count;
		  te.WE.doc_count = doc_count;
		  te.WE.invf_ptr = invf_ptr;
		  te.WE.invf_len = invf_len;
		  te.Count = 1;
		  te.Word = copy_string (Word);
		  if (!te.Word)
		    FatalError (1, "Could NOT create memory to add term");

		  AddTermEntry (&Terms, &te);
		}
	    }
	}
    }
  if (Sort)
    /* Sort the terms in ascending order by doc_count */
    qsort (Terms->TE, Terms->num, sizeof (TermEntry), doc_count_comp);
  return (Terms);
}




static int 
DE_comp (void *a, void *b)
{
  return ((DocEntry *) a)->SeekPos - ((DocEntry *) b)->SeekPos;
}





/*
 * This function is given a list of term numbers and it returns a list
 * of document numbers based on the cosine document weighting system.
 * This puts the entries in an array.
 * inverted file.
 * If MaxDocs == -1 then it means all 
 */
static DocList *
CosineGet (query_data * qd, TermList * Terms,
	   RankedQueryInfo * rqi)
{
  DocList *Docs;
  float *AccumulatedWeights = NULL;
  Splay_Tree *ST = NULL;
  Splay_Tree *Para_ST = NULL;
  Hash_Table *HT = NULL;
  List_Table *LT = NULL;
  Heap *H;
  HeapEntry *he;
  register float *fptr = NULL;
  register Invf_Doc_Entry *ide = NULL;
  register Invf_Doc_EntryH *ideh = NULL;
  int BackEnd, NumExact, MaxExact, NumParas;
  int MaxDocs = 0, MaxParas = 0;
  int i;
  Invf_Doc_Entry_Pool ide_pool;
  ide_pool.pool = NULL;

  qd->hops_taken = qd->num_of_ptrs = qd->num_of_accum = 0;

  switch (rqi->AccumMethod)
    {
    case 'S':
      ST = CosineDecodeSplay (qd, Terms, rqi, &ide_pool);
      if (!ST)
	return NULL;
      break;
    case 'A':
      AccumulatedWeights = CosineDecode (qd, Terms, rqi);
      if (!AccumulatedWeights)
	return NULL;
      break;
    case 'H':
      HT = CosineDecodeHash (qd, Terms, rqi);
      if (!HT)
	return NULL;
      break;
    case 'L':
      LT = CosineDecodeList (qd, Terms, rqi);
      if (!LT)
	return NULL;
      break;
    }

#if 0
  if (rqi->UseSplayTree)
    {

      AccumulatedWeights = CosineDecode (qd, Terms, rqi);
      fptr = AccumulatedWeights;
      ide = SP_get_first (ST);
      for (i = 0; i < qd->sd->sdh.num_of_docs; i++)
	{
	  if (AccumulatedWeights[i] != 0)
	    {
	      if (i != ide->DocNum)
		fprintf (stderr, "Sum mismatch for %d %f %d %f\n", i + 1,
			 AccumulatedWeights[i], ide->DocNum + 1, ide->Sum);
	      ide = SP_get_next (ST);
	    }
	}
    }
#endif

  switch (rqi->AccumMethod)
    {
    case 'S':
      MaxParas = ST->no_of_items;
      break;
    case 'A':
      {				/* count the number of non-zero document weights */
	register int i = qd->sd->sdh.num_of_docs;
	register float *d;
	MaxParas = 0;
	for (d = AccumulatedWeights; i; i--, d++)
	  if (*d)
	    MaxParas++;
      }
      break;
    case 'H':
      MaxParas = HT->num + HT->Suplimentary_Num;
      break;
    case 'L':
      MaxParas = LT->num;
      break;
    }

  if (rqi->MaxParasToRetrieve != -1 && MaxParas > rqi->MaxParasToRetrieve)
    MaxParas = rqi->MaxParasToRetrieve;
  MaxDocs = MaxParas;

  /* Allocate memory for the heap */
  Docs = MakeDocList (MaxDocs);
  ChangeMemInUse (qd, sizeof (DocEntry) * MaxDocs);

  H = Heap_Make (MaxDocs, Heap_Lesser);


  /* Get the sums from the array divide the sums by the 
     document weights which we retrieve from the ".idx.wgt" file and put
     the resulting data into a heap */


  he = H->HE;
  H->NumItems = MaxDocs;
  switch (rqi->AccumMethod)
    {
    case 'S':
      {
	ide = SP_get_first (ST);
	for (i = 0; i < H->NumItems; i++, ide = SP_get_next (ST), he++)
	  {
	    he->DocNum = ide->DocNum + 1;
	    he->OrgWeight = &ide->Sum;
	    qd->num_of_accum++;
	  }
      }
      break;
    case 'A':
      {
	fptr = AccumulatedWeights;
	for (i = 0; i < H->NumItems; i++, fptr++, he++)
	  {
	    he->DocNum = i + 1;
	    he->OrgWeight = fptr;
	    if (*fptr)
	      qd->num_of_accum++;
	  }
      }
      break;
    case 'H':
      {
	ideh = HT->Head;
	for (i = 0; i < H->NumItems; i++, ideh = ideh->next, he++)
	  {
	    he->DocNum = ideh->IDE.DocNum + 1;
	    he->OrgWeight = &ideh->IDE.Sum;
	    qd->num_of_accum++;
	  }
      }
      break;
    case 'L':
      {
	ide = LT->IDE;
	for (i = 0; i < H->NumItems; i++, ide++, he++)
	  {
	    he->DocNum = ide->DocNum + 1;
	    he->OrgWeight = &ide->Sum;
	    qd->num_of_accum++;
	  }
      }
      break;
    }

  he = H->HE;
  for (i = 0; i < H->NumItems; i++, he++)
    {
      *he->OrgWeight /= GetLowerApproxDocWeight (qd->awd, he->DocNum - 1);
      he->Weight = *he->OrgWeight;
      *he->OrgWeight = 0;
      he->SeekPos = he->Len = 0;
    }

  Heap_Build (H);

  he = H->HE;
  switch (rqi->AccumMethod)
    {
    case 'S':
      {
	for (i = MaxDocs; i < ST->no_of_items; i++, ide = SP_get_next (ST))
	  {
	    ide->Sum /= GetLowerApproxDocWeight (qd->awd, ide->DocNum);
	    qd->num_of_accum++;
	    if (ide->Sum <= he->Weight)
	      continue;
	    *he->OrgWeight = he->Weight;
	    he->DocNum = ide->DocNum + 1;
	    he->Weight = ide->Sum;
	    he->OrgWeight = &ide->Sum;
	    *he->OrgWeight = 0;
	    Heap_Heapify (H, 1);
	  }
      }
      break;
    case 'A':
      {
	for (i = MaxDocs; i < qd->sd->sdh.num_of_docs; i++, fptr++)
	  {
	    if (!*fptr)
	      continue;
	    qd->num_of_accum++;
	    *fptr /= GetLowerApproxDocWeight (qd->awd, i);
	    if (*fptr <= he->Weight)
	      continue;
	    *he->OrgWeight = he->Weight;
	    he->DocNum = i + 1;
	    he->Weight = *fptr;
	    he->OrgWeight = fptr;
	    *he->OrgWeight = 0;
	    Heap_Heapify (H, 1);
	  }
      }
      break;
    case 'H':
      {
	for (; ideh; ideh = ideh->next)
	  {
	    qd->num_of_accum++;
	    ideh->IDE.Sum /=
	      GetLowerApproxDocWeight (qd->awd, ideh->IDE.DocNum);
	    if (ideh->IDE.Sum <= he->Weight)
	      continue;
	    *he->OrgWeight = he->Weight;
	    he->DocNum = ideh->IDE.DocNum + 1;
	    he->Weight = ideh->IDE.Sum;
	    he->OrgWeight = &ideh->IDE.Sum;
	    *he->OrgWeight = 0;
	    Heap_Heapify (H, 1);
	  }
      }
      break;
    case 'L':
      {
	for (i = MaxDocs; i < LT->num; i++, ide++)
	  {
	    qd->num_of_accum++;
	    ide->Sum /=
	      GetLowerApproxDocWeight (qd->awd, ide->DocNum);
	    if (ide->Sum <= he->Weight)
	      continue;
	    *he->OrgWeight = he->Weight;
	    he->DocNum = ide->DocNum + 1;
	    he->Weight = ide->Sum;
	    he->OrgWeight = &ide->Sum;
	    *he->OrgWeight = 0;
	    Heap_Heapify (H, 1);
	  }
      }
      break;
    }


  if (rqi->Exact && qd->id->ifh.InvfLevel != 3)
    {
      HeapEntry *he = H->HE;

      for (i = 0; i < H->NumItems; i++, he++)
	{
	  he->Weight = he->Weight *
	    GetLowerApproxDocWeight (qd->awd, he->DocNum - 1) /
	    FetchDocStart (qd, he->DocNum, &he->SeekPos, &he->Len);
	}

      Heap_Build (H);

      he = H->HE;

      switch (rqi->AccumMethod)
	{
	case 'S':
	  {
	    ide = SP_get_first (ST);
	    for (i = 0; i < ST->no_of_items; i++, ide = SP_get_next (ST))
	      {
		u_long SeekPos, Len;
		float Weight;
		if (!ide->Sum)
		  continue;
		if (ide->Sum <= he->Weight)
		  continue;
		Weight = ide->Sum *
		  GetLowerApproxDocWeight (qd->awd, ide->DocNum) /
		  FetchDocStart (qd, ide->DocNum + 1, &SeekPos, &Len);
		if (Weight <= he->Weight)
		  continue;
		he->DocNum = ide->DocNum + 1;
		he->OrgWeight = &ide->Sum;
		he->Weight = Weight;
		he->SeekPos = SeekPos;
		he->Len = Len;
		ide->Sum = 0;
		Heap_Heapify (H, 1);
	      }
	  }
	  break;

	  /* up to here */

	case 'A':
	  {
	    fptr = AccumulatedWeights;
	    for (i = 0; i < qd->sd->sdh.num_of_docs; i++, fptr++)
	      {
		u_long SeekPos, Len;
		float Weight;
		if (!*fptr)
		  continue;
		if (*fptr <= he->Weight)
		  continue;
		Weight = *fptr *
		  GetLowerApproxDocWeight (qd->awd, i) /
		  FetchDocStart (qd, i + 1, &SeekPos, &Len);
		if (Weight <= he->Weight)
		  continue;
		he->DocNum = i + 1;
		he->OrgWeight = fptr;
		he->Weight = Weight;
		he->SeekPos = SeekPos;
		he->Len = Len;
		*fptr = 0;
		Heap_Heapify (H, 1);
	      }
	  }
	  break;
	case 'H':
	  {
	    ideh = HT->Head;
	    for (ideh = HT->Head; ideh; ideh = ideh->next)
	      {
		u_long SeekPos, Len;
		float Weight;
		if (!ideh->IDE.Sum)
		  continue;
		if (ideh->IDE.Sum <= he->Weight)
		  continue;
		Weight = ideh->IDE.Sum *
		  GetLowerApproxDocWeight (qd->awd, ideh->IDE.DocNum) /
		  FetchDocStart (qd, ideh->IDE.DocNum + 1, &SeekPos, &Len);
		if (Weight <= he->Weight)
		  continue;
		he->DocNum = ideh->IDE.DocNum + 1;
		he->OrgWeight = &ideh->IDE.Sum;
		he->Weight = Weight;
		he->SeekPos = SeekPos;
		he->Len = Len;
		ideh->IDE.Sum = 0;
		Heap_Heapify (H, 1);
	      }
	  }
	  break;
	case 'L':
	  {
	    ide = LT->IDE;
	    for (i = 0; i < LT->num; i++, ide++)
	      {
		u_long SeekPos, Len;
		float Weight;
		if (!ide->Sum)
		  continue;
		if (ide->Sum <= he->Weight)
		  continue;
		Weight = ide->Sum *
		  GetLowerApproxDocWeight (qd->awd, ide->DocNum) /
		  FetchDocStart (qd, ide->DocNum + 1, &SeekPos, &Len);
		if (Weight <= he->Weight)
		  continue;
		he->DocNum = ide->DocNum + 1;
		he->OrgWeight = &ide->Sum;
		he->Weight = Weight;
		he->SeekPos = SeekPos;
		he->Len = Len;
		ide->Sum = 0;
		Heap_Heapify (H, 1);
	      }
	  }
	  break;
	}
    }



  H->HC = Heap_Greater;
  Heap_Build (H);


  MaxDocs = H->NumItems;
  if (rqi->MaxDocsToRetrieve != -1 && MaxDocs > rqi->MaxDocsToRetrieve)
    MaxDocs = rqi->MaxDocsToRetrieve;

  /* Alarm */

  he = H->HE;
  BackEnd = H->NumItems - 1;
  NumExact = 0;
  MaxExact = H->NumItems;
  NumParas = 0;
  Para_ST = SP_createset (DE_comp);
  while (H->NumItems && Docs->num < MaxDocs)
    {
      DocEntry DocEnt;
      DocEntry *mem;

      if (rqi->Exact)
	{
	  if (H->HE[0].SeekPos == 0)
	    NumExact += Make_Exact_Root (qd, H);
	}
      else
	FetchDocStart (qd, he->DocNum, &he->SeekPos, &he->Len);

      NumParas++;

      DocEnt.DocNum = he->DocNum;
      DocEnt.Weight = he->Weight;
      DocEnt.Len = he->Len;
      DocEnt.SeekPos = he->SeekPos;
      DocEnt.CompTextBuffer = NULL;
      DocEnt.Next = NULL;

      Heap_DeleteHead (H);

      if (!(mem = SP_member (&DocEnt, Para_ST)))
	{
	  Docs->DE[Docs->num] = DocEnt;
	  SP_insert (&Docs->DE[Docs->num], Para_ST);
	  Docs->num++;
	}
      else
	{
	  DocEnt.Next = mem->Next;
	  Docs->DE[BackEnd] = DocEnt;
	  mem->Next = &Docs->DE[BackEnd--];
	}
    }
  SP_freeset (Para_ST);

  if (qd->id->ifh.InvfLevel == 3)
    {
      Message ("%d Paragraphs were required to get %d documents",
	       NumParas, Docs->num);
      if (NumExact == MaxExact)
	{
	  Message ("The exact weights of all %d paragraphs had to be calculated", MaxExact);
	  Message ("to obtain %d paragraphs. This may mean that the the documents", NumParas);
	  Message ("returned do not necessarly represent an exact cosine ranking.");
	  Message ("This problem may be corrected by increasing \'maxparas\'.");
	}
    }
#if 0
  {
    int i;
    FILE *f = fopen ("top.paras", "w");
    fprintf (f, "=========================\nTop Paragraphs\n");
    for (i = 0; i < Docs->num; i++)
      {
	DocEntry *d;
	fprintf (f, "<%d(%f)>  ", Heap[i].DocNum, Heap[i].Weight);
	for (d = Heap[i].Next; d; d = d->Next)
	  fprintf (f, "%d(%f)  ", d->DocNum, d->Weight);
	fprintf (f, "\n");
      }
    fprintf (f, "=========================\n");
    fclose (f);
  }
#endif

  if (AccumulatedWeights)
    {
      Xfree (AccumulatedWeights);
      ChangeMemInUse (qd, -sizeof (float) * qd->sd->sdh.num_of_docs);
    }
  if (ST)
    {
      int mem = ST->mem_in_use;
      SP_freeset (ST);
      ChangeMemInUse (qd, -mem);
      free_ide_pool (qd, &ide_pool);
    }
  if (HT)
    HT_free (qd, HT);

  if (LT)
    LT_free (qd, LT);


  return (Docs);
}




/* if MaxDocs == -1 the it means all */
void 
RankedQuery (query_data * qd, char *Query, RankedQueryInfo * rqi)
{
  DocList *dl;

  if (qd->TL)
    FreeTermList (&(qd->TL));

  qd->TL = ParseRankedQuery (qd->sd, Query, rqi->Sort);

  dl = CosineGet (qd, qd->TL, rqi);

  if (!dl)
    FatalError (1, "Out of memory\n");

  FreeQueryDocs (qd);

  qd->DL = dl;
}
