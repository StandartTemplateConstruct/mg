/**************************************************************************
 *
 * term_lists.h -- description
 * Copyright (C) 1994  Authors
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
 * $Id: term_lists.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:24  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:52  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:44  rjmcnab
   *** empty log message ***

   * Revision 1.1  1994/10/20  03:57:08  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
 */

#ifndef TERM_LISTS_H
#define TERM_LISTS_H

#include "sysfuncs.h"

#define MAXTERMSTRLEN 1023	/* maximum number of characters in term string */

typedef struct WordEntry
  {
    int word_num;		/* Unique number for each different word */
    u_long count;		/* Number of times the word occurs in the text */
    u_long doc_count;		/* Number of documents that contain the word */
    u_long max_doc_count;       /* [RPAP - Jan 97: Stem Index Change] */
    u_long invf_ptr;		/* This is a byte position of the  
				   inverted file entry corresponding to the word */
    u_long invf_len;		/* This is the length of the inverted 
				   file entry in bytes */
  }
WordEntry;

typedef struct TermEntry
  {
    WordEntry WE;
    int Count;			/* The number of times the word occurs in the query */
    u_char *Word;		/* The word. */
    u_char *Stem;               /* [RPAP - Jan 97: Stem Index Change] - The stemmed non-cf version of Word */
    int require_match;          /* [RJM 07/97: Ranked Required Terms] 0=optional match, 1=must match */
  }
TermEntry;

typedef struct TermList
  {
    int list_size;
    int num;
    TermEntry TE[1];
  }
TermList;

#define GetNthWE(term_list, n) (&((term_list)->TE[(n)].WE))

/* --- prototypes --- */
void ConvertTermsToString (TermList * query_term_list, char *str);
int AddTermEntry (TermList ** query_term_list, TermEntry * te);
int AddTerm (TermList ** query_term_list, u_char * Word, int Count, int word_num,
	     u_long count, u_long doc_count, u_long invf_ptr, u_long invf_len, /* [RPAP - Feb 97: Term Frequency] */
	     int stemmer_num);  
void ResetTermList (TermList ** tl);
void FreeTermList (TermList ** the_tl);
void PrintWordEntry (WordEntry * we, FILE * file);
void PrintTermEntry (TermEntry * te, FILE * file);
void PrintTermList (TermList * tl, FILE * file);
TermList *MakeTermList (int n);


#endif
