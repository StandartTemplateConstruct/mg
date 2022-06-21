/**************************************************************************
 *
 * build.h -- Global information for the passes of mg_passes
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
 * $Id: build.h,v 1.3 1994/10/20 03:56:40 tes Exp $
 *
 **************************************************************************/


#ifndef H_BUILD
#define H_BUILD

#include "longlong.h"

#define	TERMPARAGRAPH	'\003'


int init_special (char *file_name);
int init_text_1 (char *file_name);
int init_text_2 (char *file_name);
int init_invf_1 (char *file_name);
int init_invf_2 (char *file_name);
int init_ivf_1 (char *file_name);
int init_ivf_2 (char *file_name);
/*
 * file_name  IN      The name of the dictionary file
 * 
 * returns      COMPALLOK for all ok,
 *              COMPERROR for any error.  e.g. cannot read file
 */





int process_special (u_char * s_in, int l_in);
int process_text_1 (u_char * s_in, int l_in);
int process_text_2 (u_char * s_in, int l_in);
int process_invf_1 (u_char * s_in, int l_in);
int process_invf_2 (u_char * s_in, int l_in);
int process_ivf_1 (u_char * s_in, int l_in);
int process_ivf_2 (u_char * s_in, int l_in);
/*
 * s_in               IN      The binary string to be compressed
 * l_in         IN      The number of characters in s_in
 *
 * returns      COMPALLOK for all ok,
 *              COMPERROR for any error.  e.g. cannot read file
 *
 * The calling routine is responsible for ensuring that s_out is long 
 * enough.
 */





int done_special (char *filename);
int done_text_1 (char *filename);
int done_text_2 (char *filename);
int done_invf_1 (char *filename);
int done_invf_2 (char *filename);
int done_ivf_1 (char *filename);
int done_ivf_2 (char *filename);
/*
 * returns    COMPALLOK for all ok,
 *              COMPERROR for any error.  e.g. cannot write file
 */














extern char InvfLevel;
/*
 * This will determine the level of the inverted file it can take on the
 * values 1, 2, or 3.
 *
 * Level 1: The inverted file contains only document numbers making it possible
 *          to do only boolean queries.
 * 
 * Level 2: The inverted file also contains word counts per document making it
 *          possible to do cosine ranked queries.
 *
 * Level 3: The inverted file contains word positions.
 */


extern unsigned long buf_size;
/*
 * The size of the document input buffer.
 */

extern unsigned long ChunkLimit;
/*
 * The maximum number of chunks that can be written to disk.
 */

extern unsigned long invf_buffer_size;
/*
 * The amount of memory to allocate to the invertion buffer.
 */

extern char SkipSGML;
/*
 * 1 if SGML tags are to be considered non-words when building the 
 * inverted file.
 */

extern char MakeWeights;
/*
 * 1 if the weights file should be generated.
 */

extern FILE *Comp_Stats;
/*
 * Contains a file pointer to the file where compression stats should be sent
 */

extern int comp_stat_point;
/*
 * Generate a compression stat entry every comp_stat_point bytes 
 */

extern mg_ullong bytes_processed;
/*
 * The number of bytes processed. NOTE: This excludes document separators.
 */

extern mg_ullong bytes_received;
/*
 * The number of bytes processed. NOTE: This includes document separators.
 */

extern int stem_method;
/*
 * The method to use for stemming words for the inverted file.
 * see stemmer.h
 */
#endif
