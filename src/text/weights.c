/**************************************************************************
 *
 * weights.c -- Functions for reading the weights file in mgquery
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
 * $Id: weights.c,v 1.2 1994/09/20 04:42:18 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "filestats.h"
#include "memlib.h"
#include "messages.h"
#include "timing.h"

#include "mg.h"
#include "invf.h"
#include "text.h"
#include "lists.h"
#include "backend.h"
#include "weights.h"
#include "locallib.h"
#include "mg_errors.h"

#define MAXBITS (sizeof(unsigned long) * 8)

/*
   $Log: weights.c,v $
   * Revision 1.2  1994/09/20  04:42:18  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: weights.c,v 1.2 1994/09/20 04:42:18 tes Exp $";



approx_weights_data *
LoadDocWeights (File * weight_file,
		unsigned long num_of_docs)
{
  approx_weights_data *awd;
  int num;

  if (!(awd = Xmalloc (sizeof (*awd))))
    {
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  Fseek (weight_file, sizeof (long), 0);
  Fread (&awd->bits, sizeof (awd->bits), 1, weight_file);
  Fread (&awd->L, sizeof (awd->L), 1, weight_file);
  Fread (&awd->B, sizeof (awd->B), 1, weight_file);

  awd->mask = awd->bits == 32 ? 0xffffffff : (1 << awd->bits) - 1;

  num = (num_of_docs * awd->bits + 31) / 32;
  if (!(awd->DocWeights = Xmalloc (sizeof (unsigned long) * num)))
    {
      Xfree (awd);
      mg_errno = MG_NOMEM;
      return (NULL);
    }

  Fread (awd->DocWeights, sizeof (unsigned long), num, weight_file);

  awd->MemForWeights = num * sizeof (unsigned long);
  awd->num_of_docs = num_of_docs;

  mg_errno = MG_NOERROR;

  if (awd->bits <= 12)
    {
      int i, size = (1 << awd->bits);
      if (!(awd->table = (float *) Xmalloc (size * sizeof (float))))
	  return (awd);
      awd->table[0] = awd->L;
      for (i = 1; i < size; i++)
	awd->table[i] = awd->table[i - 1] * awd->B;
    }
  else
    awd->table = NULL;
  return awd;
}






/*
 * the first document in the collection has DocNum = 0
 */
float 
GetLowerApproxDocWeight (approx_weights_data * awd, register int DocNum)
{
  register unsigned long c, Pos;
  register unsigned long *dw;
  if (awd == NULL)
    return 1.0;
#if 0
  if (DocNum < 0 || DocNum >= awd->num_of_docs)
    FatalError (1, "Something is wrong in \"GetDocWeight\" DocNum = %d\n",
		DocNum);
#endif
  Pos = DocNum * awd->bits;
  dw = &(awd->DocWeights[Pos / MAXBITS]);
  Pos &= (MAXBITS - 1);
  c = *dw >> Pos;
  if (Pos + awd->bits > MAXBITS)
    c |= *(dw + 1) << (MAXBITS - Pos);
  c &= awd->mask;
  if (awd->table)
    return (awd->table[c]);
  else
    return (awd->L * pow (awd->B, (double) c));

}

void 
FreeWeights (approx_weights_data * awd)
{
  Xfree (awd->DocWeights);
  if (awd->table)
    Xfree (awd->table);
  Xfree (awd);
}
