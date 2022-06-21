/**************************************************************************
 *
 * bool_optimiser -- optimise boolean parse tree
 * Copyright (C) 1994  Tim Shimmin
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
 * $Id: bool_optimiser.h,v 1.2 1995/03/14 05:15:21 tes Exp $
 *
 **************************************************************************/

/*
   $Log: bool_optimiser.h,v $
   * Revision 1.2  1995/03/14  05:15:21  tes
   * Updated the boolean query optimiser to do different types of optimisation.
   * A query environment variable "optimise_type" specifies which one is to be
   * used. Type 1 is the new one which is faster than 2.
   *
   * Revision 1.1  1994/10/20  03:56:32  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/10/18  06:11:03  tes
   * The boolean optimiser seems to be modifying the parse tree
   * like it is supposed to.
   * Paragraph ranking now works without any text files if required to.
   *
   * Revision 1.1  1994/10/12  01:15:29  tes
   * Found bugs in the existing boolean query optimiser.
   * So decided to rewrite it.
   * I accidentally deleted query.bool.y, but I have replaced it
   * with bool_parser.y (which I have forgotten to add here ! ;-(
   *
 */

#ifndef BOOL_OPTIMISER_H
#define BOOL_OPTIMISER_H

#include "bool_tree.h"
#include "term_lists.h"

void OptimiseBoolTree (bool_tree_node * parse_tree, TermList * term_list, int type);

#endif
