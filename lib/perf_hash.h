/**************************************************************************
 *
 * perf_hash.h -- Perfect hashing functions
 * Copyright (C) 1994  Bohdan S. Majewski and Neil Sharman
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
 * $Id: perf_hash.h,v 1.2 1994/09/20 04:20:08 tes Exp $
 *
 **************************************************************************/

#ifndef H_PERF_HASH
#define H_PERF_HASH




struct tb_entry
  {
    long tb0, tb1, tb2;
  };

typedef struct
  {
    int MAX_L;
    int MAX_N;
    int MAX_M;
    int MAX_CH;
    u_char *translate;
    int *g;
    struct tb_entry **tb;
  }
perf_hash_data;

perf_hash_data *gen_hash_func (int num, u_char ** keys, int r);

int perf_hash (perf_hash_data * phd, u_char * s);

int write_perf_hash_data (FILE * f, perf_hash_data * phd);

void free_perf_hash (perf_hash_data * phd);

perf_hash_data *read_perf_hash_data (FILE * f);

#endif
