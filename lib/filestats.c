/**************************************************************************
 *
 * filestats.c -- Functions for keeping stats on file accesses
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
 * $Id: filestats.c,v 1.1 1994/08/22 00:24:42 tes Exp $
 *
 **************************************************************************/


/*
   $Log: filestats.c,v $
   * Revision 1.1  1994/08/22  00:24:42  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: filestats.c,v 1.1 1994/08/22 00:24:42 tes Exp $";

#include "sysfuncs.h"

#include "memlib.h"
#include "filestats.h"


File *
Fopen (char *name, char *mode, unsigned long magic)
{
  FILE *f;
  File *F;
  unsigned long m;
  if (!(f = fopen (name, mode)))
    return (NULL);
  if (magic)
    switch (*mode)
      {
      case 'r':
	fread ((char *) &m, sizeof (m), 1, f);
	if (m != magic)
	  {
	    fclose (f);
	    return (NULL);
	  }
	break;
      case 'w':
	fwrite ((char *) &magic, sizeof (magic), 1, f);
      }
  if (!(F = Xmalloc (sizeof (File))))
    {
      fclose (f);
      return (NULL);
    }
  F->pathname = Xstrdup (name);
  F->name = strrchr (F->pathname, '/');
  F->name = F->name ? F->name + 1 : F->pathname;
  F->f = f;
  F->Current.NumSeeks = F->Current.NumReads = F->Current.NumBytes = 0;
  F->Cumulative = F->Current;
  return (F);
}


int 
Fclose (File * F)
{
  int num;
  if (!F)
    return (0);
  num = fclose (F->f);
  if (F->pathname)
    Xfree (F->pathname);
  Xfree (F);
  return (num);
}

size_t 
Fread (void *ptr, size_t size, size_t nitems, File * F)
{
  int num;
  num = fread ((char *) ptr, size, nitems, F->f);
  F->Current.NumReads++;
  F->Current.NumBytes += num * size;
  return (num);
}

int 
Fseek (File * F, long offset, int ptrname)
{
  int num;
  num = fseek (F->f, offset, ptrname);
  F->Current.NumSeeks++;
  return (num);
}

void 
Rewind (File * F)
{
  rewind (F->f);
  F->Current.NumSeeks++;
}


void 
ZeroFileStats (File * F)
{
  F->Cumulative.NumSeeks += F->Current.NumSeeks;
  F->Cumulative.NumReads += F->Current.NumReads;
  F->Cumulative.NumBytes += F->Current.NumBytes;
  F->Current.NumSeeks = F->Current.NumReads = F->Current.NumBytes = 0;
}
