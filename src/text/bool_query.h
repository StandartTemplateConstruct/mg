/**************************************************************************
 *
 * bool_query.h -- boolean query processing
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
 * $Id: bool_query.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:23  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:17:45  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.1  1998/11/17 09:34:28  rjmcnab
   *** empty log message ***

   * Revision 1.1  1994/10/20  03:56:36  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.1  1994/10/12  01:15:33  tes
   * Found bugs in the existing boolean query optimiser.
   * So decided to rewrite it.
   * I accidentally deleted query.bool.y, but I have replaced it
   * with bool_parser.y (which I have forgotten to add here ! ;-(
   *
 */

#ifndef BOOL_QUERY_H
#define BOOL_QUERY_H

#include "backend.h"

void BooleanQuery (query_data * qd, char *Query, BooleanQueryInfo * bqi,
		   int stem_method);

#endif
