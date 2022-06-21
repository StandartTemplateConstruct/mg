/**************************************************************************
 *
 * lists.h -- List processing functions for boolean queries
 * Copyright (C) 1994  Neil Sharman, Alistair Moffat and Lachlan Andrew
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
 * $Id: lists.h,v 1.3 1995/03/14 05:15:28 tes Exp $
 *
 **************************************************************************/


#ifndef H_LISTS
#define H_LISTS



typedef struct DocEntry
  {
    float Weight;
    int DocNum;
    unsigned long SeekPos;	/* position in the text file in bytes */
    unsigned long Len;		/* length of the document in bytes */
    char *CompTextBuffer;
    struct DocEntry *Next;
    short or_included;		/*[TS:Mar/95] whether included in an AND of ORed-terms */
  }
DocEntry;

typedef struct DocList
  {
    int num;
    DocEntry DE[1];
  }
DocList;

DocList *MakeDocList (int num);
DocList *ResizeDocList (DocList * d, int num);
DocList *IntersectLists (DocList * List1, DocList * List2);
DocList *DiffLists (DocList * List1, DocList * List2);
DocList *MergeLists (DocList * List1, DocList * List2);



#endif
