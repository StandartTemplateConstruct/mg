/**************************************************************************
 *
 * ppm.c -- PPM data compression
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
 * $Id: ppm.c,v 1.2 1994/09/20 04:42:02 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "arithcode.h"
#include "model.h"



/*
   $Log: ppm.c,v $
   * Revision 1.2  1994/09/20  04:42:02  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: ppm.c,v 1.2 1994/09/20 04:42:02 tes Exp $";

/* FIXEDORDER sets the maximum context length that will be used.
   Making FIXEDORDER large increases the compression if there is enough
   space to store the model, but when space limit is reached increasing
   FIXEDORDER will decrease compression because of model thrashing.
   Third order (FIXEDORDER=3) is suitable for a few hundred Kb of model
   space, FIXEDORDER=1 is suitable for a few tens of kB.
   This version requires that FIXEDORDER be a preprocessor rather
   than runtime variable to allow loops to be unrolled and make for faster
   execution. Maximum in this implementation is 3 */

#ifndef FIXEDORDER
#define FIXEDORDER	3
#endif

/* global variables */
eventptr E = NULL;		/* array of eventnodes */
point numnodes = 0, nfreenodes = 0;
long kbytes = defaultkbytes;
/*boolean               excluded[nchars]={false};  - MAHE */
boolean excluded[16384] =
{false};			/* excluded characters */

/* local variables */
#define defaultchunksize        4096	/* compression chunks */

#define EOSchar         (charsetsize)

unsigned chunksize = defaultchunksize;
eventptr s[FIXEDORDER + 1] =
{NULL},				/* current contexts */
  t[FIXEDORDER + 1] =
{NULL};				/* new contexts */

#if (FIXEDORDER==3)
#define SHIFT_ST	s[1] = t[0]; s[2] = t[1] ; s[3] = t[2]
#else
#if (FIXEDORDER==2)
#define SHIFT_ST	s[1] = t[0]; s[2] = t[1]
#else
#if (FIXEDORDER==1)
#define SHIFT_ST	s[1] = t[0]
#else
#define SHIFT_ST
#endif
#endif
#endif

/* follow a character list and set the exclusions */
#define SET_EX(p)				\
do						\
{	register eventptr q;			\
	q = p32((p32(p))->next);		\
	for (; q!=ENULL; q=p32(q->next))	\
		excluded[q->eventnum] = true;	\
}						\
while (false)

/* clear the exclusions */
#define CLEAR_EX(p)				\
do						\
{	register eventptr q;			\
	q = p32((p32(p))->next);		\
	for (; q!=ENULL; q=p32(q->next))	\
		excluded[q->eventnum] = false;	\
}						\
while (false)

/*==================================*/

void
write_model ()
{
  fprintf (stderr, "ppm");
  write_method ();
#ifdef NOEXCLUSIONS
  fprintf (stderr, "nx");
#endif
  fprintf (stderr, "-o%1d", FIXEDORDER);
  fprintf (stderr, "-k%ld", kbytes);
}

/*==================================*/

void
encode_in_order (n, e)
     int n;
     event e;
/* encode character e in order n */
{
  boolean encoded;
  int i, c;
  t[n] = encode_event_noex (&s[n]->foll, e, &encoded);
  if (encoded)
    c = n;
  else
    for (c = n - 1; c >= 0; c--)
      {
#ifdef NOEXCLUSIONS
	t[c] = encode_event_noex (&s[c]->foll, e, &encoded);
#else
	SET_EX (s[c + 1]->foll.list);
	t[c] = encode_event (&s[c]->foll, e, &encoded);
#endif
	if (encoded)
	  break;
      }
  if (!encoded)
    arithmetic_encode (e, e + 1, nchars);
  for (i = c + 1; i <= n; i++)
    if (i > 0)
      t[i]->prev = p16 (t[i - 1]);
    else
      t[0]->prev = p16 (s[0]);
  for (i = c - 1; i >= 0; i--)
    t[i] = p32 (t[i + 1]->prev);
#ifndef NOEXCLUSIONS
  if (c != n)
    CLEAR_EX (s[c + 1]->foll.list);
#endif
  SHIFT_ST;
}

/*==================================*/

/* not required in this version */

#if 0

void
decode_in_order (n, e)
     int n;
     event *e;
{
  boolean decoded;
  int i, c;
  t[n] = decode_event_noex (&s[n]->foll, &decoded, e);
  if (decoded)
    c = n;
  else
    for (c = n - 1; c >= 0; c--)
      {
#ifdef NOEXCLUSIONS
	t[c] = decode_event_noex (&s[c]->foll, &decoded, e);
#else
	SET_EX (s[c + 1]->foll.list);
	t[c] = decode_event (&s[c]->foll, &decoded, e);
#endif
	if (decoded)
	  break;
      }
  if (!decoded)
    {
      *e = arithmetic_decode_target (nchars);
      arithmetic_decode (*e, *e + 1, nchars);
    }
  for (i = c + 1; i <= n; i++)
    {
      t[i]->eventnum = *e;
      if (i > 0)
	t[i]->prev = p16 (t[i - 1]);
      else
	t[0]->prev = p16 (s[0]);
    }
  {
    register int i;
    for (i = c - 1; i >= 0; i--)
      t[i] = p32 (t[i + 1]->prev);
  }
#ifndef NOEXCLUSIONS
  if (c != n)
    CLEAR_EX (s[c + 1]->foll.list);
#endif
  SHIFT_ST;
}

#endif

/*==================================*/

void
start_model ()
{
  int i;
  /* is this the first call? */
  if (E == NULL)
    E = (eventptr) malloc ((unsigned) kbytes * 1024);
  /* start off with no information */
  numnodes = 0;
  nfreenodes = maxnodes;
  for (i = 0; i <= FIXEDORDER; i++)
    {
      s[i] = newnode (NULL);
      if (i > 0)
	s[i]->prev = p16 (s[i - 1]);
    }
}

/*==================================*/

void
encodestring (str, n)
     unsigned short *str;
     int n;
/* encode all n characters of the supplied string str */
{
  event e;
  boolean encoded;
  register eventptr S, T;
  int i;
#ifndef NOEXCLUSIONS
  register eventptr Sex;
#endif

  if (E == NULL)
    start_model ();
  /* set the initial context node */
  S = s[FIXEDORDER];

  for (i = 0; i < n; i++)
    {
      /* encode one character */
      e = *str++;
      /* this next monstrosity is logically equivalent to 
         encode_in_order(FIXEDORDER,e)
         the loop has been unrolled to make it all go faster */
      for (;;)
	{
	  T = encode_event_noex (&S->foll, e, &encoded);
	  if (encoded)
	    break;
	  t[FIXEDORDER] = T;

#ifdef NOEXCLUSIONS
#if (FIXEDORDER>2)
	  S = p32 (S->prev);
	  t[2] = encode_event_noex (&S->foll, e, &encoded);
	  t[3]->prev = p16 (t[2]);
	  if (encoded)
	    break;
#endif

#if (FIXEDORDER>1)
	  S = p32 (S->prev);
	  t[1] = encode_event_noex (&S->foll, e, &encoded);
	  t[2]->prev = p16 (t[1]);
	  if (encoded)
	    break;
#endif

#if (FIXEDORDER>0)
	  S = p32 (S->prev);
	  t[0] = encode_event_noex (&S->foll, e, &encoded);
	  t[1]->prev = p16 (t[0]);
	  if (encoded)
	    break;
#endif
#else
#if (FIXEDORDER>2)
	  Sex = S;
	  SET_EX (Sex->foll.list);
	  S = p32 (S->prev);
	  t[2] = encode_event (&S->foll, e, &encoded);
	  t[3]->prev = p16 (t[2]);
	  if (encoded)
	    {
	      CLEAR_EX (Sex->foll.list);
	      break;
	    }
#endif

#if (FIXEDORDER>1)
	  Sex = S;
	  SET_EX (Sex->foll.list);
	  S = p32 (S->prev);
	  t[1] = encode_event (&S->foll, e, &encoded);
	  t[2]->prev = p16 (t[1]);
	  if (encoded)
	    {
	      CLEAR_EX (Sex->foll.list);
	      break;
	    }
#endif

#if (FIXEDORDER>0)
	  Sex = S;
	  SET_EX (Sex->foll.list);
	  S = p32 (S->prev);
	  t[0] = encode_event (&S->foll, e, &encoded);
	  t[1]->prev = p16 (t[0]);
	  if (encoded)
	    {
	      CLEAR_EX (Sex->foll.list);
	      break;
	    }
	  CLEAR_EX (Sex->foll.list);
#endif
#endif

	  arithmetic_encode (e, e + 1, nchars);
	  t[0]->prev = p16 (s[0]);
	  break;
	}
      /* phew! Update the current context to the latest node */
      S = p32 (T->prev);

      /* is there a danger of running out of space? */
      if (nfreenodes <= FIXEDORDER)
	{
	  start_model ();
	  S = s[FIXEDORDER];
	}
    }

  /* save the current contexts for the various order submodels */
  s[FIXEDORDER] = S;
  for (i = FIXEDORDER; i > 0; i--)
    s[i - 1] = p32 (s[i]->prev);
  /* and send the terminator character */
  encode_in_order (FIXEDORDER, EOSchar);
  /* and check the space again */
  if (nfreenodes <= FIXEDORDER)
    start_model ();
}

/*==================================*/

void
decodestring (str, n)
     unsigned short *str;
     int *n;
{
  event e;
  boolean decoded;
  register eventptr S, T;
#ifndef NOEXCLUSIONS
  register eventptr Sex;
#endif

  /* just mimic the corresponding encoding actions */
  if (E == NULL)
    start_model ();
  S = s[FIXEDORDER];

  *n = 0;
  for (;;)
    {				/* logically equivalent to "decode_in_order(FIXEDORDER,&e)" */

      for (;;)
	{
	  T = decode_event_noex (&S->foll, &decoded, &e);
	  if (decoded)
	    break;
	  t[FIXEDORDER] = T;

#ifdef NOEXCLUSIONS
#if (FIXEDORDER>2)
	  S = p32 (S->prev);
	  t[2] = decode_event_noex (&S->foll, &decoded, &e);
	  t[3]->prev = p16 (t[2]);
	  if (decoded)
	    {
	      t[3]->eventnum = e;
	      break;
	    }
#endif

#if (FIXEDORDER>1)
	  S = p32 (S->prev);
	  t[1] = decode_event_noex (&S->foll, &decoded, &e);
	  t[2]->prev = p16 (t[1]);
	  if (decoded)
	    {
#if (FIXEDORDER>2)
	      t[3]->eventnum =
#endif
		t[2]->eventnum = e;
	      break;
	    }
#endif

#if (FIXEDORDER>0)
	  S = p32 (S->prev);
	  t[0] = decode_event_noex (&S->foll, &decoded, &e);
	  t[1]->prev = p16 (t[0]);
	  if (decoded)
	    {
#if (FIXEDORDER>2)
	      t[3]->eventnum =
#endif
#if (FIXEDORDER>1)
		t[2]->eventnum =
#endif
		t[1]->eventnum = e;
	      break;
	    }
#endif
#else
#if (FIXEDORDER>2)
	  Sex = S;
	  SET_EX (Sex->foll.list);
	  S = p32 (S->prev);
	  t[2] = decode_event (&S->foll, &decoded, &e);
	  t[3]->prev = p16 (t[2]);
	  if (decoded)
	    {
	      t[3]->eventnum = e;
	      CLEAR_EX (Sex->foll.list);
	      break;
	    }
#endif

#if (FIXEDORDER>1)
	  Sex = S;
	  SET_EX (Sex->foll.list);
	  S = p32 (S->prev);
	  t[1] = decode_event (&S->foll, &decoded, &e);
	  t[2]->prev = p16 (t[1]);
	  if (decoded)
	    {
#if (FIXEDORDER>2)
	      t[3]->eventnum =
#endif
		t[2]->eventnum = e;
	      CLEAR_EX (Sex->foll.list);
	      break;
	    }
#endif

#if (FIXEDORDER>0)
	  Sex = S;
	  SET_EX (Sex->foll.list);
	  S = p32 (S->prev);
	  t[0] = decode_event (&S->foll, &decoded, &e);
	  t[1]->prev = p16 (t[0]);
	  if (decoded)
	    {
#if (FIXEDORDER>2)
	      t[3]->eventnum =
#endif
#if (FIXEDORDER>1)
		t[2]->eventnum =
#endif
		t[1]->eventnum = e;
	      CLEAR_EX (Sex->foll.list);
	      break;
	    }
	  CLEAR_EX (Sex->foll.list);
#endif
#endif

	  e = arithmetic_decode_target (nchars);
	  arithmetic_decode (e, e + 1, nchars);
#if (FIXEDORDER>2)
	  t[3]->eventnum =
#endif
#if (FIXEDORDER>1)
	    t[2]->eventnum =
#endif
#if (FIXEDORDER>0)
	    t[1]->eventnum =
#endif
	    t[0]->eventnum = e;
	  t[0]->prev = p16 (s[0]);
	  break;
	}
      S = p32 (T->prev);

      /* check the space requirements */
      if (nfreenodes <= FIXEDORDER)
	{
	  start_model ();
	  S = s[FIXEDORDER];
	}
      /* are we done?  or do we save the char */
      if (e == EOSchar)
	break;
      *str++ = e;
      (*n)++;
    }
  s[FIXEDORDER] = S;
}


#if 0				/* Writing my own version of this - MAHE */
void
encodefile ()
/* encodes a file by breaking into a sequence of fixed length chunks */
{
  unsigned char *str;
  int c, n = 0;

  str = (unsigned char *) malloc ((unsigned) chunksize);
  startoutputingbits ();
  startencoding ();

  /* to encode the file, break it into chunks and encode the
     chunks one by one */
  while ((c = getchar ()) != EOF)
    {
      str[n++] = c;
      if (n == chunksize)
	{
	  encodestring (str, n);
	  rawbytes += n;
	  n = 0;
	}
    }
  /* send final partial chunk */
  if (n > 0)
    {
      encodestring (str, n);
      rawbytes += n;
    }
  /* send EOF as an empty string */
  encodestring (str, 0);
  doneencoding ();
  doneoutputingbits ();
}

void
decodefile ()
{
  unsigned char *str;
  int n, i;

  str = (unsigned char *) malloc ((unsigned) chunksize);
  startinputingbits ();
  startdecoding ();
  for (;;)
    {
      decodestring (str, &n);
      if (n == 0)
	break;
      for (i = 0; i < n; i++)
	putchar ((int) str[i]);
      rawbytes += n;
    }
}

#endif /* #if 0 */
