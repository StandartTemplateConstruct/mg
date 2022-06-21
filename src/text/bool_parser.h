/**************************************************************************
 *
 * filename -- description
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
 * $Id: bool_parser.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:23  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:17:43  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.2  1998/11/25 07:55:40  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:34:26  rjmcnab
   *** empty log message ***

   * Revision 1.1  1994/10/20  03:56:33  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.1  1994/10/12  01:15:31  tes
   * Found bugs in the existing boolean query optimiser.
   * So decided to rewrite it.
   * I accidentally deleted query.bool.y, but I have replaced it
   * with bool_parser.y (which I have forgotten to add here ! ;-(
   *
 */

#ifndef BOOL_PARSER_H
#define BOOL_PARSER_H

#include "backend.h"
#include "term_lists.h"
#include "invf.h"

bool_tree_node *ParseBool (char *query_line, int query_len,
			   TermList ** the_term_list, int the_stemmer_num, 
			   int the_stem_method, int *res,
			   stemmed_dict * the_sd, int is_indexed,  /* [RPAP - Jan 97: Stem Index Change] */
			   QueryTermList ** the_query_term_list);  /* [RPAP - Feb 97: Term Frequency] */

#endif
