/**************************************************************************
 *
 * comp_dict.h -- Functions for loading the compression dictionary
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
 * $Id: comp_dict.h,v 1.3 1994/10/20 03:56:42 tes Exp $
 *
 **************************************************************************/




#define HASH_RATIO 2




typedef struct dict_hash_table
  {
    u_long size;
    huff_data *hd;
    u_long *codes;
    u_char **words;
    u_char **table[1];
  }
dict_hash_table;




extern compression_dict_header cdh;
extern compressed_text_header cth;
extern comp_frags_header cfh[2];

extern dict_hash_table *ht[2];

extern huff_data char_huff[2];
extern huff_data lens_huff[2];
extern u_long *char_codes[2], *lens_codes[2];
extern u_long Words_disk;
extern u_long Chars_disk;



int LoadCompressionDictionary (char *dict_file_name);
