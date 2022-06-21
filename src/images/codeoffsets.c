/**************************************************************************
 *
 * codeoffsets.c -- Functions which code the mark offsets
 * Copyright (C) 1994  Stuart Inglis
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
 * $Id: codeoffsets.c,v 1.1.1.1 1994/08/11 03:26:09 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "arithcode.h"
#include "utils.h"

#define TOP_INC 2
#define MID_INC 1
#define BOT_INC 1


void 
EncodeOffsets (marklistptr symbol_list, int nsyms)
{
  int xpos = 1, xneg = 1, ypos = 1, yneg = 1;
  marklistptr step;
  int found;
  int dox = 0, leveltop = 1, levelmid = 1, levelbot = 1;
  elemlist xlist, ylist;	/* all previous offsets */
  int p, f, t, xoff, yoff, symnum, count;
  elemlist *xcond, *ycond;	/* previous offsets conditioned on symbol num */

  xcond = (elemlist *) malloc (nsyms * sizeof (elemlist));
  if (!xcond)
    error_msg ("EncodeOffsets", "Out of memory...", "");
  ycond = (elemlist *) malloc (nsyms * sizeof (elemlist));
  if (!ycond)
    error_msg ("EncodeOffsets", "Out of memory...", "");

  for (t = 0; t < nsyms; t++)
    {
      xcond[t].head = ycond[t].head = NULL;
      xcond[t].tot = ycond[t].tot = 0;
    }
  xlist.head = NULL;
  xlist.tot = 0;
  ylist.head = NULL;
  ylist.tot = 0;
  for (step = symbol_list, count = 0; step; step = step->next, count++)
    {
      xoff = step->data.xoffset;
      yoff = step->data.yoffset;
      symnum = step->data.symnum;
      for (dox = 1; dox >= 0; dox--)
	{
	  /* encode the offsets, x first then y */

	  if (dox)
	    found = elem_find (&xcond[symnum], xoff, &p, &f, &t);
	  else
	    found = elem_find (&ycond[symnum], yoff, &p, &f, &t);

	  if (found)
	    {
	      arithmetic_encode (0, leveltop, leveltop + levelmid + levelbot);
	      leveltop += TOP_INC;
	      arithmetic_encode (p, f, t);
	    }
	  else
	    {
	      if (dox)
		found = elem_find (&xlist, xoff, &p, &f, &t);
	      else
		found = elem_find (&ylist, yoff, &p, &f, &t);

	      if (found)
		{
		  arithmetic_encode (leveltop, leveltop + levelmid, leveltop + levelmid + levelbot);
		  levelmid += MID_INC;
		  arithmetic_encode (p, f, t);
		}
	      else
		{
		  arithmetic_encode (leveltop + levelmid, leveltop + levelmid + levelbot, leveltop + levelmid + levelbot);
		  levelbot += BOT_INC;
		  if (dox)
		    EncodeGammaSigned (xoff, &xpos, &xneg);
		  else
		    EncodeGammaSigned (yoff, &ypos, &yneg);
		}
	    }
	  if (dox)
	    {
	      elem_add (&xlist, xoff);
	      elem_add (&xcond[symnum], xoff);
	    }
	  else
	    {
	      elem_add (&ylist, yoff);
	      elem_add (&ycond[symnum], yoff);
	    }

	  if (leveltop + levelmid >= maxfrequency)
	    {
	      leveltop = (leveltop + 1) / 2;
	      levelmid = (levelmid + 1) / 2;
	      levelbot = (levelbot + 1) / 2;
	    }
	}
    }
  for (t = 0; t < nsyms; t++)
    {
      elem_free (&xcond[t]);
      elem_free (&ycond[t]);
    }
  elem_free (&xlist);
  elem_free (&ylist);
  if (V)
    fprintf (stderr, "offset models: top %d, mid %d, bot %d\n", (leveltop - 1) / TOP_INC, (levelmid - 1) / MID_INC, (levelbot - 1) / BOT_INC);
}





void 
DecodeOffsets (marklistptr symbol_list, int nsyms)
{
  int xpos = 1, xneg = 1, ypos = 1, yneg = 1;
  marklistptr step;
  int i, target, dox;
  int leveltop = 1, levelmid = 1, levelbot = 1;
  elemlist xlist, ylist;
  int p, f, t, xoff = 0, yoff = 0, symnum, count = 0;
  elemlist *xcond, *ycond;

  xcond = (elemlist *) malloc (nsyms * sizeof (elemlist));
  if (!xcond)
    error_msg ("EncodeOffsets", "Out of memory...", "");
  ycond = (elemlist *) malloc (nsyms * sizeof (elemlist));
  if (!ycond)
    error_msg ("EncodeOffsets", "Out of memory...", "");

  for (i = 0; i < nsyms; i++)
    {
      xcond[i].head = ycond[i].head = NULL;
      xcond[i].tot = ycond[i].tot = 0;
    }

  xlist.head = NULL;
  xlist.tot = 0;
  ylist.head = NULL;
  ylist.tot = 0;

  for (step = symbol_list, count = 0; step; step = step->next, count++)
    {
      symnum = step->data.symnum;
      for (dox = 1; dox >= 0; dox--)
	{
	  /* decode x offset first, then y */
	  target = arithmetic_decode_target (leveltop + levelmid + levelbot);
	  if (target < leveltop)
	    {
	      arithmetic_decode (0, leveltop, leveltop + levelmid + levelbot);
	      leveltop += TOP_INC;

	      if (dox)
		i = arithmetic_decode_target (xcond[symnum].tot);
	      else
		i = arithmetic_decode_target (ycond[symnum].tot);

	      if (dox)
		{
		  elem_getrange (&xcond[symnum], i, &xoff);
		  elem_find (&xcond[symnum], xoff, &p, &f, &t);
		}
	      else
		{
		  elem_getrange (&ycond[symnum], i, &yoff);
		  elem_find (&ycond[symnum], yoff, &p, &f, &t);
		}
	      arithmetic_decode (p, f, t);
	    }
	  else if (target < leveltop + levelmid)
	    {
	      arithmetic_decode (leveltop, leveltop + levelmid, leveltop + levelmid + levelbot);
	      levelmid += MID_INC;

	      if (dox)
		i = arithmetic_decode_target (xlist.tot);
	      else
		i = arithmetic_decode_target (ylist.tot);

	      if (dox)
		{
		  elem_getrange (&xlist, i, &xoff);
		  elem_find (&xlist, xoff, &p, &f, &t);
		}
	      else
		{
		  elem_getrange (&ylist, i, &yoff);
		  elem_find (&ylist, yoff, &p, &f, &t);
		}
	      arithmetic_decode (p, f, t);
	    }
	  else
	    {
	      arithmetic_decode (leveltop + levelmid, leveltop + levelmid + levelbot, leveltop + levelmid + levelbot);
	      levelbot += BOT_INC;
	      if (dox)
		xoff = DecodeGammaSigned (&xpos, &xneg);
	      else
		yoff = DecodeGammaSigned (&ypos, &yneg);
	    }

	  if (dox)
	    {
	      step->data.xoffset = xoff;
	      elem_add (&xlist, xoff);
	      elem_add (&xcond[symnum], xoff);
	    }
	  else
	    {
	      step->data.yoffset = yoff;
	      elem_add (&ylist, yoff);
	      elem_add (&ycond[symnum], yoff);
	    }

	  if (leveltop + levelmid >= maxfrequency)
	    {
	      leveltop = (leveltop + 1) / 2;
	      levelmid = (levelmid + 1) / 2;
	      levelbot = (levelbot + 1) / 2;
	    }
	}
    }
  for (t = 0; t < nsyms; t++)
    {
      elem_free (&xcond[t]);
      elem_free (&ycond[t]);
    }
  elem_free (&xlist);
  elem_free (&ylist);
}
