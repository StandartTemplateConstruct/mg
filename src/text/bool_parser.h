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
 * $Id: bool_parser.h,v 1.1 1994/10/20 03:56:33 tes Exp $
 *
 **************************************************************************/

/*
   $Log: bool_parser.h,v $
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

bool_tree_node *ParseBool (char *query_line, int query_len,
		  TermList ** the_term_list, int the_stem_method, int *res);

#endif
