/**************************************************************************
 *
 * perf_hash.c -- Perfect hashing functions
 * Copyright (C) 1992  Bohdan S. Majewski
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
 * $Id: perf_hash.c,v 1.2 1997/07/24 01:48:13 wew Exp wew $
 *
 **************************************************************************
 *
 * 1994 Neil Sharman
 *      Small modifications were made to the interface so that it fitted in
 *      with mg.
 *
 **************************************************************************/

/*
   $Log: perf_hash.c,v $
 * Revision 1.2  1997/07/24  01:48:13  wew
 * add prototype for irandm() (implicit return of int loses on 64-bit
 * machines)
 *
 * Revision 1.1  1997/07/24  01:23:18  wew
 * Initial revision
 *
   * Revision 1.2  1994/08/22  00:26:30  tes
   * Made perf_hash take the ceiling of 1.23 * num
   * in lieu of the floor.
   * This was done so that perf_hash works ok on
   * a small number of keys.
   *
   * Revision 1.1  1994/08/22  00:24:49  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: perf_hash.c,v 1.2 1997/07/24 01:48:13 wew Exp wew $";


#define STRUCT

#include <time.h>  /* for call to time () in SEED_RANDOM() */

#include "sysfuncs.h"

#include "local_strings.h"
#include "memlib.h"
#include "messages.h"
#include "perf_hash.h"


#define FALSE 0
#define TRUE  1

#define STATIC 0

/* Random Number stuff */

long irandm (long is[2]);  /* from random.c */

static long seed[] = {0, 0};
#define RANDOM() irandm(seed)
#define SEED_RANDOM(the_seed) do{ seed[0] = the_seed; }while(0)

/* Max number of keys - must be a power of 2 minus 1 */
static int MAX_M;


static int MAX_CH;


/* the width of the mapping tables  */
static int MAX_L;



#define c     1.23
static int MAX_N;




static u_char *translate;

#define MEMBER 0
#define ERASED 1



/* direction constants for the edge oriented 3-graph representation */
#define A              0
#define B        (MAX_M)
#define C          (2*B)

/* NODEa, NODEb, NODEc, FIRST and NEXT are used to represent a 3-graph */
static int *NODEa, *NODEb, *NODEc;
static int *FIRST;
static int *NEXT;


#define norm(e) ((e) % MAX_M)
/* in general should be ((e) % MAX_M) but MAX_M = 2^k - 1 */


static char *mk;		/* for marking edges/vertices as removed */
static int *g;			/* array g defines a minimal perfect hash function */



/* stack of edges, created during the independency */
/* test, used in the assignment step               */
static struct
  {
    int sp;
    int *st;
  }
S;
#define push(s, e) s.st[s.sp++] = (e)
#define pop(s, e)  e = s.st[--s.sp]

#ifndef STRUCT
static long **tb0, **tb1, **tb2;
#else
struct tb_entry **tb;
#endif


static int 
unique (int v)
{				/* checks if vertex v belongs to only one edge */
  return (FIRST[v] != 0 && NEXT[FIRST[v]] == 0) ? FIRST[v] : 0;
}				/*unique */








static void 
delete (int v, int e)
{				/* deletes edge e from list of v */

  int b;

  b = FIRST[v];
  assert (norm (b) != 0);
  if (norm (b) == e)
    FIRST[v] = NEXT[FIRST[v]];
  else
    {
      while (norm (NEXT[b]) != e)
	{
	  b = NEXT[b];
	  assert (norm (b) != 0);
	}
      NEXT[b] = NEXT[NEXT[b]];
    }
}				/*delete */






#if 0
/* recursively removes the edges of a 3-graph which have at least */
/* one unique vertex. The removed edges are placed on stack S     */
static void 
ph_remove (int e, int v)
{
  int b;
  mk[e] = ERASED;
  push (S, e);

  if (NODEa[e] != v)
    delete (NODEa[e], e);
  if (NODEb[e] != v)
    delete (NODEb[e], e);
  if (NODEc[e] != v)
    delete (NODEc[e], e);

  if (NODEa[e] != v && mk[b = norm (unique (NODEa[e]))] == MEMBER)
    ph_remove (b, NODEa[e]);
  if (NODEb[e] != v && mk[b = norm (unique (NODEb[e]))] == MEMBER)
    ph_remove (b, NODEb[e]);
  if (NODEc[e] != v && mk[b = norm (unique (NODEc[e]))] == MEMBER)
    ph_remove (b, NODEc[e]);
}
#else


static int StackPos, StackMax;
static struct Stack
  {
    int e, v, m;
  }
 *Stack = NULL;

#define StackAdd(_e, _v, _m)						\
  do {									\
    if (StackPos == StackMax)						\
      {									\
        StackMax <<= 1;							\
	if (!(Stack = Xrealloc(Stack, sizeof(*Stack) * StackMax)))	\
	  return(-1);							\
      }									\
    Stack[StackPos].e = (_e);						\
    Stack[StackPos].v = (_v);						\
    Stack[StackPos++].m = (_m);						\
  } while(0)


static int 
ph_remove (int e, int v)
{
  if (!Stack)
    {
      StackMax = 256;
      if (!(Stack = Xmalloc (sizeof (*Stack) * StackMax)))
	return (-1);
      StackPos = 0;
    }
  Stack[StackPos].e = e;
  Stack[StackPos].v = v;
  Stack[StackPos].m = 0;
  StackPos++;
  while (StackPos)
    {
      int m, b;
      StackPos--;
      e = Stack[StackPos].e;
      v = Stack[StackPos].v;
      m = Stack[StackPos].m;


      switch (m)
	{
	case 0:
	  {
	    mk[e] = ERASED;
	    push (S, e);

	    if (NODEa[e] != v)
	      delete (NODEa[e], e);
	    if (NODEb[e] != v)
	      delete (NODEb[e], e);
	    if (NODEc[e] != v)
	      delete (NODEc[e], e);

	    if (NODEa[e] != v && mk[b = norm (unique (NODEa[e]))] == MEMBER)
	      {
		Stack[StackPos++].m = 1;
		StackAdd (b, NODEa[e], 0);
		break;
	      }
	  }
	case 1:
	  {
	    if (NODEb[e] != v && mk[b = norm (unique (NODEb[e]))] == MEMBER)
	      {
		Stack[StackPos++].m = 2;
		StackAdd (b, NODEb[e], 0);
		break;
	      }
	  }
	case 2:
	  if (NODEc[e] != v && mk[b = norm (unique (NODEc[e]))] == MEMBER)
	    StackAdd (b, NODEc[e], 0);
	}
    }
  return (0);
}
#endif





/* returns TRUE if and only if set of edges created in the tree_builder */
/* procedure form a 3-graph with independent edges. Edges of a 3-graph  */
/* are independent iff repeated deletion of edges containing vertices   */
/* of degree 1 results in a graph with no edges in it.                  */
static int 
acyclic (int m, int n)
{

  int v, e;
  /* build list of edges for each node */
  for (v = 1; v <= n; FIRST[v++] = 0);
  for (e = 1; e <= m; e++)
    {
      NEXT[A + e] = FIRST[NODEa[e]];
      FIRST[NODEa[e]] = A + e;
      NEXT[B + e] = FIRST[NODEb[e]];
      FIRST[NODEb[e]] = B + e;
      NEXT[C + e] = FIRST[NODEc[e]];
      FIRST[NODEc[e]] = C + e;

      /* mark edge e as belonging to the hypergraph */
      mk[e] = MEMBER;
    }


  S.sp = 0;			/* empty the stack */
  mk[0] = ERASED;		/* a sentinel for the test below */
  for (v = 1; v <= n; v++)
    {
      if (mk[e = norm (unique (v))] == MEMBER)
	ph_remove (e, v);
    }

  /* check if any edges were left in the graph */
  return S.sp == m;
}				/*acyclic */








/* repeatedly maps words into edges of a 3-graph until the edges */
/* are independent.                                              */
static int 
tree_builder (int num, u_char ** keys, int *n)
{

  int e, k = 0;
  int i, j;
  u_char *w;

  do
    {
      *n = (int) (num * c + 1);
      /* initialize tables of random integers */
      for (i = 0; i < MAX_CH; i++)
	for (j = 0; j < MAX_L; j++)
	  {
#ifndef STRUCT
	    tb0[i][j] = RANDOM () % (long) *n;
	    tb1[i][j] = RANDOM () % (long) *n;
	    tb2[i][j] = RANDOM () % (long) *n;
#else
	    struct tb_entry *tbp = &tb[i][j];
	    tbp->tb0 = RANDOM () % (long) *n;
	    tbp->tb1 = RANDOM () % (long) *n;
	    tbp->tb2 = RANDOM () % (long) *n;
#endif
	  }

      /* compute an edge (NODEa[e], NODEb[e], NODEc[e]) for each word */
      for (e = 1; e <= num; e++)
	{
	  /*generate an edge corresponding to the ith word */
	  int x = 0, y = 0, z = 0, l;
	  w = keys[e - 1];

	  l = *w++;
	  j = (l - 1) % MAX_L;

	  do
	    {
#ifndef STRUCT
	      x += tb0[translate[*w]][j];
	      y += tb1[translate[*w]][j];
	      z += tb2[translate[*w]][j];
#else
	      struct tb_entry *tbp = &tb[translate[*w]][j];
	      x += tbp->tb0;
	      y += tbp->tb1;
	      z += tbp->tb2;
#endif
	      if (++j >= MAX_L)
		j = 0;
	      w++;
	    }
	  while (--l);
	  x = (x % *n) + 1;
	  y = (y % *n) + 1;
	  z = (z % *n) + 1;
	  if (y == x && ++y > *n)
	    y = 1;
	  if (z == x && ++z > *n)
	    z = 1;
	  if (z == y)
	    {
	      if (++z > *n)
		z = 1;
	      if (z == x && ++z > *n)
		z = 1;
	    }

	  NODEa[e] = x;
	  NODEb[e] = y;
	  NODEc[e] = z;
	}
      if (++k > 50)		/* make sure we will not wait for ever */
	FatalError (1, "Unable to generate the perfect hash function. "
		    "This is probably because there aren't enough words");
    }
  while (!acyclic (num, *n));
  return k;
}				/*tree_builder */









/* calculates values of the g function/array so that i-th word is */
/* placed at the (i-1)st position of a hash table.                */
static void 
assign (int n)
{

  int v, w = 0, e;

  /* mark all the vertices of the 3-graph as unassigned */
  for (v = 1; v <= n; mk[v++] = FALSE);

  while (S.sp > 0)
    {
      int xor = 0;
      pop (S, e);
      v = NODEa[e];
      if (mk[v])
	xor ^= g[v];
      else
	{
	  g[w = v] = 0;
	  mk[v] = TRUE;
	}
      v = NODEb[e];
      if (mk[v])
	xor ^= g[v];
      else
	{
	  g[w = v] = 0;
	  mk[v] = TRUE;
	}
      v = NODEc[e];
      if (mk[v])
	xor ^= g[v];
      else
	{
	  g[w = v] = 0;
	  mk[v] = TRUE;
	}
      g[w] = (e - 1) ^ xor;
    }
}				/*assign */









/*check returns 0 if no error in mphf is detected, else returns error code */
static int 
check (int m)
{

  int e, err_code = 0;

  /*for each word compute its place in hash table and check if correct */
  for (e = 1; e <= m; e++)
    {
      if ((e - 1) != (g[NODEa[e]] ^ g[NODEb[e]] ^ g[NODEc[e]]))
	{
	  (void) fprintf (stderr, "Error for %dth word detected.\n", e);
	  err_code = e;
	}
    }
  return err_code;
}				/*check */







#if 0
void 
outputh (int m, int n)
{

  int i, j, w = (int) ceil (log10 ((double) n + 1.0)) + 1;

#if 0
  char *PRG =
  "int h(char *s)\n"
  "{\n"
  "  int u, v, w, i;\n"
  "i = (strlen(s) - 1) % _HXL;\n"
  "  u = v = w = 0;\n"
  "  while(*s)\n"
  "    {\n"
  "      u += mt1[(*s) - _HFC][i];\n"
  "      v += mt2[(*s) - _HFC][i];\n"
  "      w += mt3[(*s++) - _HFC][i];\n"
  "      if(++i >= _HXL) i = 0;\n"
  "    }\n"
  "  u %= _HFN; v %= _HFN; w %= _HFN;\n"
  "  if(u == v && ++v >= _HFN) v = 0;\n"
  "  if(w == u && ++w >= _HFN) w = 0;\n"
  "  if(w == v && ++w >= _HFN) w = 0;\n"
  "  if(w == u && ++w >= _HFN) w = 0;\n"
  "  return (g[u] ^ g[v] ^ g[w]) % _HFM;\n"
  "};\n\n";
#endif

  char *PRG =
  "int h(char *s)\n"
  "{\n"
  "  int u, v, w, i;\n"
  "  i = (strlen(s) - 1) % _HXL;\n"
  "  u = v = w = 0;\n"
  "  while(*s)\n"
  "    {\n"
  "      if((u += mt1[(*s) - _HFC][i]) >= _HFN) u -= _HFN;\n"
  "      if((v += mt2[(*s) - _HFC][i]) >= _HFN) v -= _HFN;\n"
  "      if((w += mt3[(*s++) - _HFC][i]) >= _HFN) w -= _HFN;\n"
  "      if(++i >= _HXL) i = 0;\n"
  "    }\n"
  "  if(u == v && ++v >= _HFN) v = 0;\n"
  "  if(w == v) \n"
  "    {\n"
  "      if(++w >= _HFN) w = 0;\n"
  "      if(w == u && ++w >= _HFN) w = 0;\n"
  "    }\n"
  "  else\n"
  "    if(w == u)\n"
  "      {\n"
  "	     if(++w >= _HFN) w = 0;\n"
  "	     if(w == v && ++w >= _HFN) w = 0;\n"
  "      }\n"
  "  return ((g[u] ^ g[v] ^ g[w]) > _HFM)? 0 : (g[u] ^ g[v] ^ g[w]);\n"
  "}\n\n";

  printf ("#define _HFN %d\n#define _HFC %d\n", n, fc);
  printf ("#define _HFM %d\n#define _HXL %d\n", m, MAX_L);
  printf ("#define _HFA %d\n\nstatic int g[_HFN] = {\n", lc - fc + 1);
  for (i = 1; i < n; i++)
    {
      if (i % 8 == 0)
	putchar ('\n');
      printf ("%5d, ", g[i]);
    }
  printf ("%5d};\n\n", g[n]);

  printf ("static int mt1[_HFA][_HXL] = {\n");
  for (j = fc; j < lc; j++)
    {
      printf ("{");
      for (i = 0; i < MAX_L - 1; i++)
	printf ("%*d,", w, tb0[j][i]);
      printf ("%*d},\n", w, tb0[j][MAX_L - 1]);
    }
  printf ("{");
  for (i = 0; i < MAX_L - 1; i++)
    printf ("%*d,", w, tb0[lc][i]);
  printf ("%*d}\n", w, tb0[lc][MAX_L - 1]);
  printf ("};\n\n");

  printf ("static int mt2[_HFA][_HXL] = {\n");
  for (j = fc; j < lc; j++)
    {
      printf ("{");
      for (i = 0; i < MAX_L - 1; i++)
	printf ("%*d,", w, tb1[j][i]);
      printf ("%*d},\n", w, tb1[j][MAX_L - 1]);
    }
  printf ("{");
  for (i = 0; i < MAX_L - 1; i++)
    printf ("%*d,", w, tb1[lc][i]);
  printf ("%*d}\n", w, tb1[lc][MAX_L - 1]);
  printf ("};\n\n");

  printf ("static int mt3[_HFA][_HXL] = {\n");
  for (j = fc; j < lc; j++)
    {
      printf ("{");
      for (i = 0; i < MAX_L - 1; i++)
	printf ("%*d,", w, tb2[j][i]);
      printf ("%*d},\n", w, tb2[j][MAX_L - 1]);
    }
  printf ("{");
  for (i = 0; i < MAX_L - 1; i++)
    printf ("%*d,", w, tb2[lc][i]);
  printf ("%*d}\n", w, tb2[lc][MAX_L - 1]);
  printf ("};\n\n");

  printf ("%s", PRG);
}				/*outputh */
#endif







static void 
make_trans_func (int num, u_char ** keys)
{
  int i, j;

  bzero ((char *) translate, sizeof (translate));

  MAX_L = 1;
  for (i = 0; i < num; i++)
    {
      unsigned len = keys[i][0];
      char *s = (char *) (keys[i] + 1);
      for (; len; len--, s++)
	translate[(unsigned) *s] = 1;
      if (i)
	{
	  int l;
	  l = prefixlen (keys[i - 1], keys[i]);
	  if (l + 1 > MAX_L)
	    MAX_L = l + 1;
	}
    }
  j = 0;
  for (i = 0; i < 256; i++)
    if (translate[i])
      translate[i] = j++;
  MAX_CH = j;
}


static void 
temp_free (void)
{
  if (NODEa)
    Xfree (NODEa);
  if (NODEb)
    Xfree (NODEb);
  if (NODEc)
    Xfree (NODEc);
  if (FIRST)
    Xfree (FIRST);
  if (NEXT)
    Xfree (NEXT);
  if (S.st)
    Xfree (S.st);
  if (mk)
    Xfree (mk);
}

static void 
all_free (void)
{
  int i;
  temp_free ();
  if (translate)
    Xfree (translate);
  if (g)
    Xfree (g);
#ifndef STRUCT
  for (i = 0; i < MAX_CH; i++)
    {
      if (tb0 && tb0[i])
	Xfree (tb0[i]);
      if (tb1 && tb1[i])
	Xfree (tb1[i]);
      if (tb2 && tb2[i])
	Xfree (tb2[i]);
    }
  if (tb0)
    Xfree (tb0);
  if (tb1)
    Xfree (tb1);
  if (tb2)
    Xfree (tb2);
#else
  for (i = 0; i < MAX_CH; i++)
    if (tb && tb[i])
      Xfree (tb[i]);
  if (tb)
    Xfree (tb);
#endif
}

static int 
allocate_memory (void)
{
  int i, ok = 1;
  NODEa = Xmalloc (sizeof (int) * (MAX_M + 1));
  NODEb = Xmalloc (sizeof (int) * (MAX_M + 1));
  NODEc = Xmalloc (sizeof (int) * (MAX_M + 1));
  FIRST = Xmalloc (sizeof (int) * (MAX_N + 1));
  NEXT = Xmalloc (sizeof (int) * (MAX_M + 1) * 3);
  S.st = Xmalloc (sizeof (int) * MAX_M);
  mk = Xmalloc (sizeof (char) * (MAX_N + 1));
  g = Xmalloc (sizeof (int) * (MAX_N + 1));
#ifndef STRUCT
  tb0 = Xmalloc (sizeof (int *) * MAX_CH);
  tb1 = Xmalloc (sizeof (int *) * MAX_CH);
  tb2 = Xmalloc (sizeof (int *) * MAX_CH);
  for (i = 0; i < MAX_CH; i++)
    {
      if (tb0)
	if (!(tb0[i] = Xmalloc (sizeof (long) * MAX_L)))
	    ok = 0;
      if (tb1)
	if (!(tb1[i] = Xmalloc (sizeof (long) * MAX_L)))
	    ok = 0;
      if (tb2)
	if (!(tb2[i] = Xmalloc (sizeof (long) * MAX_L)))
	    ok = 0;
    }

  if (!(NODEa && NODEb && NODEc && FIRST && NEXT && S.st && mk && g &&
	tb0 && tb1 && tb2 && ok))
    {
      all_free ();
      return (-1);
    }
#else
  tb = Xmalloc (sizeof (struct tb_entry *) * MAX_CH);
  for (i = 0; i < MAX_CH; i++)
    if (tb)
      if (!(tb[i] = Xmalloc (sizeof (struct tb_entry) * MAX_L)))
	  ok = 0;

  if (!(NODEa && NODEb && NODEc && FIRST && NEXT && S.st && mk && g &&
	tb && ok))
    {
      all_free ();
      return (-1);
    }
#endif
  return (0);
}


perf_hash_data *
gen_hash_func (int num, u_char ** keys, int r)
{
  int n;
  perf_hash_data *phd;
  translate = Xmalloc (sizeof (u_char) * 256);
  make_trans_func (num, keys);

  if (r <= 0)
    SEED_RANDOM ((long) time ((time_t *) NULL));
  else
    SEED_RANDOM (r);

  MAX_M = num + 1;

  MAX_N = MAX_M + MAX_M / 4;

  if (allocate_memory () == -1)
    return (NULL);

  /* construct an "acyclic" hypergraph */
  tree_builder (num, keys, &n);
  /* compute function g */
  assign (n);

  if (check (num) != 0)
    FatalError (1, "An error has been detected in the mphf.");
  temp_free ();

  if (!(phd = Xmalloc (sizeof (perf_hash_data))))
    return (NULL);

  bcopy ((char *) &g[1], (char *) g, n * sizeof (int));

  phd->MAX_L = MAX_L;
  phd->MAX_N = n;
  phd->MAX_M = num;
  phd->MAX_CH = MAX_CH;
  phd->translate = translate;
  phd->g = g;
#ifndef STRUCT
  phd->tb0 = tb0;
  phd->tb1 = tb1;
  phd->tb2 = tb2;
#else
  phd->tb = tb;
#endif
  return (phd);
}

int 
write_perf_hash_data (FILE * f, perf_hash_data * phd)
{
  int tot, i;
  tot = fwrite ((char *) &phd->MAX_L, sizeof (int), 1, f) * sizeof (int);
  tot += fwrite ((char *) &phd->MAX_N, sizeof (int), 1, f) * sizeof (int);
  tot += fwrite ((char *) &phd->MAX_M, sizeof (int), 1, f) * sizeof (int);
  tot += fwrite ((char *) &phd->MAX_CH, sizeof (int), 1, f) * sizeof (int);
  tot += fwrite ((char *) phd->translate, sizeof (u_char), 256, f);
  tot += fwrite ((char *) phd->g, sizeof (int), phd->MAX_N + 1, f) * sizeof (int);
#ifndef STRUCT
  for (i = 0; i < phd->MAX_CH; i++)
    {
      tot += fwrite ((char *) phd->tb0[i], sizeof (int), phd->MAX_L, f) *
        sizeof (int);
      tot += fwrite ((char *) phd->tb1[i], sizeof (int), phd->MAX_L, f) *
        sizeof (int);
      tot += fwrite ((char *) phd->tb2[i], sizeof (int), phd->MAX_L, f) *
        sizeof (int);
    }
#else
  for (i = 0; i < phd->MAX_CH; i++)
    tot += fwrite ((char *) phd->tb[i], sizeof (struct tb_entry), phd->MAX_L, f) *
      sizeof (struct tb_entry);
#endif
  return (tot);
}


perf_hash_data *
read_perf_hash_data (FILE * f)
{
  perf_hash_data *phd;
  int i, tot, ok = 1;
  if (!(phd = Xmalloc (sizeof (perf_hash_data))))
    return (NULL);
  tot = fread ((char *) &phd->MAX_L, sizeof (int), 1, f) * sizeof (int);
  tot += fread ((char *) &phd->MAX_N, sizeof (int), 1, f) * sizeof (int);
  tot += fread ((char *) &phd->MAX_M, sizeof (int), 1, f) * sizeof (int);
  tot += fread ((char *) &phd->MAX_CH, sizeof (int), 1, f) * sizeof (int);
  if (tot != 4 * sizeof (int))
      return (NULL);
  phd->translate = Xmalloc (sizeof (u_char) * 256);
  phd->g = Xmalloc (sizeof (int) * (phd->MAX_N + 1));
#ifndef STRUCT
  phd->tb0 = Xmalloc (sizeof (int *) * phd->MAX_CH);
  phd->tb1 = Xmalloc (sizeof (int *) * phd->MAX_CH);
  phd->tb2 = Xmalloc (sizeof (int *) * phd->MAX_CH);
  for (i = 0; i < phd->MAX_CH; i++)
    {
      if (phd->tb0)
	if (!(phd->tb0[i] = Xmalloc (sizeof (long) * phd->MAX_L)))
	    ok = 0;
      if (phd->tb1)
	if (!(phd->tb1[i] = Xmalloc (sizeof (long) * phd->MAX_L)))
	    ok = 0;
      if (phd->tb2)
	if (!(phd->tb2[i] = Xmalloc (sizeof (long) * phd->MAX_L)))
	    ok = 0;
    }
  if (!(phd->translate && phd->g && phd->tb0 && phd->tb1 && phd->tb2 && ok))
    {
      if (phd->translate)
	Xfree (phd->translate);
      if (phd->g)
	Xfree (phd->g);
      for (i = 0; i < MAX_CH; i++)
	{
	  if (phd->tb0 && phd->tb0[i])
	    Xfree (phd->tb0[i]);
	  if (phd->tb1 && phd->tb1[i])
	    Xfree (phd->tb1[i]);
	  if (phd->tb2 && phd->tb2[i])
	    Xfree (phd->tb2[i]);
	}
      if (phd->tb0)
	Xfree (phd->tb0);
      if (phd->tb1)
	Xfree (phd->tb1);
      if (phd->tb2)
	Xfree (phd->tb2);
      Xfree (phd);
      return (NULL);
    }
  tot += fread ((char *) phd->translate, sizeof (u_char), 256, f);
  tot += fread ((char *) phd->g, sizeof (int), phd->MAX_N + 1, f) * sizeof (int);
  for (i = 0; i < phd->MAX_CH; i++)
    {
      tot += fread ((char *) phd->tb0[i], sizeof (long), phd->MAX_L, f) *
        sizeof (int);
      tot += fread ((char *) phd->tb1[i], sizeof (long), phd->MAX_L, f) *
        sizeof (int);
      tot += fread ((char *) phd->tb2[i], sizeof (long), phd->MAX_L, f) *
        sizeof (int);
    }
#else
  phd->tb = Xmalloc (sizeof (struct tb_entry *) * phd->MAX_CH);
  for (i = 0; i < phd->MAX_CH; i++)
    if (phd->tb)
      if (!(phd->tb[i] = Xmalloc (sizeof (struct tb_entry) * phd->MAX_L)))
	  ok = 0;
  if (!(phd->translate && phd->g && phd->tb && ok))
    {
      if (phd->translate)
	Xfree (phd->translate);
      if (phd->g)
	Xfree (phd->g);
      for (i = 0; i < MAX_CH; i++)
	if (phd->tb && phd->tb[i])
	  Xfree (phd->tb[i]);
      if (phd->tb)
	Xfree (phd->tb);
      Xfree (phd);
      return (NULL);
    }
  tot += fread ((char *) phd->translate, sizeof (u_char), 256, f);
  tot += fread ((char *) phd->g, sizeof (int), phd->MAX_N + 1, f) * sizeof (int);
  for (i = 0; i < phd->MAX_CH; i++)
    tot += fread ((char *) phd->tb[i], sizeof (struct tb_entry), phd->MAX_L, f) *
      sizeof (struct tb_entry);
#endif
  return (phd);
}

void 
free_perf_hash (perf_hash_data * phd)
{
  int i;
  if (phd->translate)
    Xfree (phd->translate);
  if (phd->g)
    Xfree (phd->g);
  for (i = 0; i < phd->MAX_CH; i++)
    if (phd->tb && phd->tb[i])
      Xfree (phd->tb[i]);
  if (phd->tb)
    Xfree (phd->tb);
  Xfree (phd);
}


int 
perf_hash (perf_hash_data * phd, u_char * s)
{
  int u, v, w, i, l;
  l = *s++;
  i = (l - 1) % phd->MAX_L;
  u = v = w = 0;
  while (l)
    {
#ifndef STRUCT
      if ((u += phd->tb0[phd->translate[*s]][i]) >= phd->MAX_N)
	u -= phd->MAX_N;
      if ((v += phd->tb1[phd->translate[*s]][i]) >= phd->MAX_N)
	v -= phd->MAX_N;
      if ((w += phd->tb2[phd->translate[*s]][i]) >= phd->MAX_N)
	w -= phd->MAX_N;
#else
      struct tb_entry *tbp = &phd->tb[phd->translate[*s]][i];
      if ((u += tbp->tb0) >= phd->MAX_N)
	u -= phd->MAX_N;
      if ((v += tbp->tb1) >= phd->MAX_N)
	v -= phd->MAX_N;
      if ((w += tbp->tb2) >= phd->MAX_N)
	w -= phd->MAX_N;
#endif
      if (++i >= phd->MAX_L)
	i = 0;
      l--;
      s++;
    }
  if (u == v && ++v >= phd->MAX_N)
    v = 0;
  if (w == v)
    {
      if (++w >= phd->MAX_N)
	w = 0;
      if (w == u && ++w >= phd->MAX_N)
	w = 0;
    }
  else if (w == u)
    {
      if (++w >= phd->MAX_N)
	w = 0;
      if (w == v && ++w >= phd->MAX_N)
	w = 0;
    }
  return ((phd->g[u] ^ phd->g[v] ^ phd->g[w]) > phd->MAX_M) ? 0 :
    (phd->g[u] ^ phd->g[v] ^ phd->g[w]);
}
