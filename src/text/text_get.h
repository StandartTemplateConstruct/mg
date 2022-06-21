/**************************************************************************
 *
 * text_get.h -- Function for reading documents from the compressed text
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
 * $Id: text_get.h,v 1.3 1994/10/20 03:57:12 tes Exp $
 *
 **************************************************************************/

#ifndef H_TEXT_GET
#define H_TEXT_GET



/* FetchDocStart () 
 * Reads into DocEnt the starting position of the document in the *.text file
 * Where the first document is document number 1
 * It returns the true weight of the document.
 */
double FetchDocStart (query_data * qd, u_long DN, u_long * seek_pos, u_long * len);

unsigned long FetchInitialParagraph (text_data * td, unsigned long ParaNum);


/* FetchCompressed () 
 * Reads into buffer DocBuff the compressed form of document DocNum. 
 * Where the first document is document number 1
 */
int FetchCompressed (query_data * qd, char **DocBuff, DocEntry * DocEnt);

text_data *LoadTextData (File * text, File * text_idx_wgt, File * text_idx);

void FreeTextData (text_data * td);

int GetPosLens (query_data * qd, DocEntry * Docs, int num);

int LoadBuffers (query_data * qd, DocEntry * Docs, int max_mem, int num);

void FreeBuffers (query_data * qd, DocEntry * Docs, int num);



compression_dict *LoadCompDict (File * text_comp_dict,
				File * text_aux_dict,
				File * text_fast_comp_dict);

void FreeCompDict (compression_dict * cd);


int DecodeText (compression_dict * cd,
		u_char * s_in, int l_in, u_char * s_out, int *l_out);

#endif
