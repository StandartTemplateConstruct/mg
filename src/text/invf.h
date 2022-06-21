/**************************************************************************
 *
 * invf.h -- Data structures for inverted files
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
 * $Id: invf.h,v 1.3 1994/11/29 00:31:56 tes Exp $
 *
 **************************************************************************/



#ifndef H_INVF
#define H_INVF

/* NOTE: This does not include the magic number */
struct invf_dict_header
  {
    unsigned long lookback;
    unsigned long dict_size;
    unsigned long total_bytes;
    unsigned long index_string_bytes;
    unsigned long input_bytes;
    unsigned long num_of_docs;
    unsigned long static_num_of_docs;
    unsigned long num_of_words;
    unsigned long stem_method;
  };

struct stem_dict_header
  {
    unsigned long lookback;
    unsigned long block_size;
    unsigned long num_blocks;
    unsigned long blocks_start;
    unsigned long index_chars;
    unsigned long num_of_docs;
    unsigned long static_num_of_docs;
    unsigned long num_of_words;
    unsigned long stem_method;
  };

struct invf_file_header
  {
    unsigned long no_of_words;
    unsigned long no_of_ptrs;
    unsigned long skip_mode;
    unsigned long params[16];
    unsigned long InvfLevel;
  };

#endif
