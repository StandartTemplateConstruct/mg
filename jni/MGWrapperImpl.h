/*
 *    MGWrapperImpl.h
 *    Copyright (C) 2002 New Zealand Digital Library, http://www.nzdl.org
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "backend.h"


/*************************************************************************
   NOTES

 - These data structures are based on the MGPP ones but are modified
   to reflect the different capabilities of the MG system.

 *************************************************************************/

 
typedef struct QueryInfo
{
  /* Name of index to use */
  char* index;
  /* Maximum number of documents to retrieve */
  unsigned long maxDocs;
  /* Whether term frequency information is desired (boolean value) */
  int needTermFreqs;
} QueryInfo;


typedef struct MGWrapperData
{
  /* Information about a query, see above */
  QueryInfo* queryInfo;
  /* Whether to perform stemming and case-folding */
  int defaultStemMethod;
  /* Whether to perform boolean AND (1) or boolean OR (0) queries */
  int defaultBoolCombine;
} MGWrapperData;
