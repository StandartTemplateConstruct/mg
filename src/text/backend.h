/**************************************************************************
 *
 * backend.h -- Underlying routines and datastructures for mgquery
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
 * $Id: backend.h,v 1.4 1994/11/29 00:31:56 tes Exp $
 *
 **************************************************************************/


#ifndef BACKEND_H
#define BACKEND_H

#include "sysfuncs.h"

#include "timing.h"
#include "lists.h"
#include "term_lists.h"
#include "mg.h"
#include "invf.h"
#include "text.h"


typedef struct invf_data
  {
    File *InvfFile;
    unsigned long N;
    unsigned long Nstatic;	/* N parameter for decoding inverted file entries */
    struct invf_file_header ifh;
  }
invf_data;

typedef struct text_data
  {
    File *TextFile;
    File *TextIdxFile;
    File *TextIdxWgtFile;
    long current_pos;
    struct
      {
	unsigned long Start;
	float Weight;
      }
     *idx_data;
    compressed_text_header cth;
  }
text_data;


typedef struct auxiliary_dict
  {
    aux_frags_header afh[2];
    u_char *word_data[2];
    u_char **words[2];
    int blk_start[2][33], blk_end[2][33];	/* blk_start and blk_end are required
						   for the hybrid methods */
  }
auxiliary_dict;


typedef struct compression_dict
  {
    compression_dict_header cdh;
    comp_frags_header *cfh[2];
    unsigned long MemForCompDict;
    u_char ***values[2];
    u_char *escape[2];
    huff_data *chars_huff[2];
    u_long **chars_vals[2];
    huff_data *lens_huff[2];
    u_long **lens_vals[2];
    auxiliary_dict *ad;
    int fast_loaded;
  }
compression_dict;

typedef struct stemmed_dict
  {
    File *stem_file;
    struct stem_dict_header sdh;
    u_char **index;
    unsigned long *pos;
    int active;
    u_char *buffer;
    unsigned long MemForStemDict;
  }
stemmed_dict;



typedef struct approx_weights_data
  {
    double L;
    double B;
    unsigned long *DocWeights;
    char bits;
    float *table;
    unsigned long mask;
    unsigned long MemForWeights;
    unsigned long num_of_docs;
  }
approx_weights_data;



typedef struct RankedQueryInfo
  {
    int QueryFreqs;
    int Exact;           /* use exact weights for ranking or not */
    long MaxDocsToRetrieve;	/* may be -1 for all */
    long MaxParasToRetrieve;
    int Sort;        
    char AccumMethod;		/* 'A' = array,  'S' = splay tree,  'H' = hash_table */
    long MaxAccums;		/* may be -1 for all */
    long MaxTerms;		/* may be -1 for all */
    int StopAtMaxAccum; /* Stop at maximum accumulator or not */
    long HashTblSize;
    char *skip_dump;
  }
RankedQueryInfo;



typedef struct BooleanQueryInfo
  {
    long MaxDocsToRetrieve;
  }
BooleanQueryInfo;


/* [TS:24/Aug/94] - maximum number of characters in term string */
#define MAXTERMSTRLEN 1023

typedef struct query_data
  {
    stemmed_dict *sd;
    compression_dict *cd;
    approx_weights_data *awd;
    invf_data *id;
    text_data *td;
    char *pathname;
    File *File_text;
    File *File_comp_dict;
    File *File_aux_dict;
    File *File_fast_comp_dict;
    File *File_text_idx_wgt;
    File *File_text_idx;
    File *File_stem;
    File *File_invf;
    File *File_weight_approx;
    unsigned long mem_in_use, max_mem_in_use;
    unsigned long num_of_ptrs, tot_num_of_ptrs;
    unsigned long num_of_terms, tot_num_of_terms;
    unsigned long num_of_accum, tot_num_of_accum;
    unsigned long num_of_ans, tot_num_of_ans;
    unsigned long hops_taken, tot_hops_taken;
    unsigned long text_idx_lookups, tot_text_idx_lookups;
    unsigned long max_buffers;
    unsigned doc_pos;
    unsigned buf_in_use;
    DocList *DL;
    TermList *TL;		/* [TS:Oct/94] - so term list for query can easily be accessed */
    u_char *TextBuffer;
    int TextBufferLen;

  }
query_data;



typedef struct InitQueryTimes
  {
    ProgTime Start;
    ProgTime StemDict;
    ProgTime ApproxWeights;
    ProgTime CompDict;
    ProgTime Invf;
    ProgTime Text;
  }
InitQueryTimes;



query_data *InitQuerySystem (char *dir, char *name, InitQueryTimes * iqt);

void ChangeMemInUse (query_data * qd, long delta);

void FinishQuerySystem (query_data * qd);

void ResetFileStats (query_data * qd);

void TransFileStats (query_data * qd);

void ChangeMemInUse (query_data * qd, long delta);

void RankedQuery (query_data * qd, char *Query, RankedQueryInfo * rqi);

void BooleanQuery (query_data * qd, char *Query, BooleanQueryInfo * bqi);

void DocnumsQuery (query_data * qd, char *QueryLine);

void FreeTextBuffer (query_data * qd);

void FreeQueryDocs (query_data * qd);

int LoadCompressedText (query_data * qd, int max_mem);

int GetDocNum (query_data * qd);

float GetDocWeight (query_data * qd);

long GetDocCompLength (query_data * qd);

u_char *GetDocText (query_data * qd, unsigned long *len);

DocEntry *GetDocChain (query_data * qd);

int NextDoc (query_data * qd);

#endif
