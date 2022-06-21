/**************************************************************************
 *
 * query.docnums.c -- Docnums query evaluation
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
 * $Id: query.docnums.c,v 1.1.1.1 1994/08/11 03:26:13 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "filestats.h"
#include "timing.h"

#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"


static DocList *
AddToDocList (DocList * Docs, int res)
{
  int j;
  /* Search the list for the word */
  for (j = 0; j < Docs->num; j++)
    if (Docs->DE[j].DocNum == res)
      break;

  /* Increment the weight if the word is in the list */
  if (j < Docs->num)
    Docs->DE[j].Weight++;
  else
    {
      /* Create a new entry in the list for the new document */
      Docs = ResizeDocList (Docs, Docs->num + 1);
      Docs->DE[Docs->num].DocNum = res;
      Docs->DE[Docs->num].Weight = 1;
      Docs->DE[Docs->num].SeekPos = 0;
      Docs->DE[Docs->num].Len = 0;
      Docs->DE[Docs->num].CompTextBuffer = NULL;
      Docs->DE[Docs->num].Next = NULL;
      Docs->num++;
    }
  return (Docs);
}

void 
DocnumsQuery (query_data * qd, char *QueryLine)
{
  DocList *Docs = MakeDocList (0);
  unsigned long start = 0, finish;
  int range = 0; /* is there a range or not ? */

  /* find the start of the first word */
  while (*QueryLine)
    {
      char *end;
      finish = strtol (QueryLine, &end, 10);
      if (end != (char *) QueryLine && finish >= 1 &&
	  finish <= qd->sd->sdh.num_of_docs)
	{
	  unsigned long i;
	  if (!range)
	    start = finish;
	  else
	    range = 0;
	  for (i = start; i <= finish; i++)
	    Docs = AddToDocList (Docs, i);
	  while (*end == ' ' || *end == '\t' || *end == '\n')
	    end++;
	  if (*end == '-')
	    {
	      start = finish;
	      range = 1;
	      end++;
	    }
	}
      else
	break;
      QueryLine = end;
    }
  FreeQueryDocs (qd);

  qd->DL = Docs;
}
