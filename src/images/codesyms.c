/**************************************************************************
 *
 * codesyms.c -- Functions which code the symbol sequence
 * Copyright (C) 1994  Hugh Emberson
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
 * $Id: codesyms.c,v 1.1.1.1 1994/08/11 03:26:09 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "arithcode.h"
#include "marklist.h"
#include "pbmtools.h"
#include "utils.h"
#include "codesyms.h"

int charsetsize = 256;

void 
EncodeSymbols (marklistptr MarkList, int NSyms)
{
  marklistptr step;
  ushort Events[PPM_CHUNK];
  int j = 0;

  charsetsize = max (NSyms, 256);

  step = MarkList;
  while (step)
    {
      Events[j] = step->data.symnum;

      assert (Events[j] > 0 || Events[j] < (unsigned) NSyms);

      j++;

      if (j == PPM_CHUNK)
	{
	  encodestring (Events, PPM_CHUNK);
	  j = 0;
	}
      step = step->next;
    };

  if (j != 0)
    encodestring (Events, j);

  encodestring (Events, 0);
}


marklistptr 
DecodeSymbols (int NSyms)
{
  marklistptr MarkList = NULL;
  ushort Events[PPM_CHUNK];
  int j, n;
  marktype d;

  charsetsize = max (NSyms, 256);

  do
    {
      decodestring (Events, &n);

      for (j = 0; j < n; j++)
	{
	  d.symnum = Events[j];
	  marklist_add (&MarkList, d);
	}
    }
  while (n != 0);

  return MarkList;
}

void 
InitPPM (void)
{
  kbytes = 896;
}
