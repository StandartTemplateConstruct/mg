/**************************************************************************
 *
 * model.h -- Part pf PPM data compression
 * Copyright (C) 1987  Alistair Moffat
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
 * $Id: model.h,v 1.2 1994/09/20 04:42:00 tes Exp $
 *
 **************************************************************************/

#define defaultkbytes	896
#define minkbytes	56
#define maxkbytes	((65536/1024)*sizeof(eventnode))

#ifndef increment
#define	increment	8
#endif
#ifndef halveat
#define halveat		64
#endif
#define maxtotalcnt 	(increment*halveat)

typedef int boolean;
#define false           0
#define true            1

typedef short event;
typedef unsigned short point;

typedef struct
  {
    short totalcnt;
    short notfound;
    point list;			/* list storing the event records */
  }
eventset;

typedef struct
  {
    event eventnum;
    short count;
    point next, prev;
    eventset foll;
  }
eventnode, *eventptr;

#define memsize		(kbytes*1024)
#define maxnodes	(memsize/sizeof(eventnode))
/* #define charsetsize     256 -- MAHE */
#define nchars          (charsetsize+1)

extern int charsetsize;		/* -- MAHE */

extern eventptr E;
extern long kbytes;
extern point numnodes, nfreenodes;
extern boolean excluded[];	/* -- MAHE */

#define ENULL		(E)

typedef unsigned u;
#define p32(p)	(E+p)
#define p16(p)	(p-E)

/* in ppm.c */
void write_model ();
void encodestring ();
void decodestring ();
void encodefile ();
void decodefile ();

/* in lstevent.c */
void write_method ();
eventnode *encode_event_noex ();
eventnode *decode_event_noex ();
eventnode *encode_event ();
eventnode *decode_event ();
/* eventnode *addevent(); */
eventnode *newnode ();
