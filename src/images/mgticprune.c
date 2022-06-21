/**************************************************************************
 *
 * mgticprune.c -- Program to prune a textual image library
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
 * $Id: mgticprune.c,v 1.1.1.1 1994/08/11 03:26:12 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "marklist.h"
#include "pbmtools.h"
#include "extractor.h"
#include "utils.h"
#include "match.h"

marklistptr *avglib = NULL;
int avglibsize;
int libsize = 0;


void 
usage ()
{
  fprintf (stderr, "usage:\n"
    "\tmgticbuild [-a n]  [-s n]  [-Q]   libraryfile   [pruned-library]\n");
  exit (1);
}




/* foreach list in the avglib[] array, an average mark is returned */
marklistptr 
average_library ()
{
  marklistptr librarylist;
  int i;
  marktype avgmark;

  librarylist = NULL;
  for (i = 0; i < libsize; i++)
    {
      if (avglib[i])
	{
	  marklist_average (avglib[i], &avgmark);
	  avgmark.symnum = i;
	  marklist_addcopy (&librarylist, avgmark);
	}
    }
  return librarylist;
}


void 
average_space_enlarge ()
{
  int j;

  if (libsize >= avglibsize)
    {
      avglib = (marklistptr *) realloc (avglib, (avglibsize + 100) * sizeof (marklistptr));
      if (!avglib)
	error_msg ("mgticprune", "out of memory in reallocating..", "");
      for (j = avglibsize; j < avglibsize + 100; j++)
	avglib[j] = NULL;
      avglibsize += 100;
    }
}


/* for each mark in 'origlist', if it can't be matched with one in 
   'librarylist', then add to the librarylist, and update the relevant
   avglib[] position */
void 
greedy_add (marklistptr origlist, marklistptr * librarylist)
{
  marklistptr step, step_i;
  int i, j, notfound;
  marktype d_i;

  for (i = 0, step_i = origlist; step_i; i++, step_i = step_i->next)
    {
      d_i = step_i->data;

      notfound = 1;
      for (j = 0, step = *librarylist; step; j++, step = step->next)
	{
	  if (NOT_SCREENED (d_i, step->data))
	    if (MATCH (&d_i, &step->data) < MATCH_VAL)
	      {
		d_i.symnum = j;
		marklist_addcopy (&avglib[j], d_i);
		notfound = 0;
		break;
	      }
	}

      if (notfound)
	{
	  d_i.symnum = libsize;
	  marklist_addcopy (librarylist, d_i);
	  marklist_addcopy (&avglib[libsize], d_i);
	  libsize++;
	  if (libsize >= avglibsize)
	    average_space_enlarge ();
	}
    }
}



/* for each mark in 'library', if another matches with it, remove
   one copy from the library, and erase the relevant avglib[] */
void 
greedy_prune (marklistptr * library)
{
  int count;
  int i, j;
  marklistptr step;
  marktype d;

  count = marklist_length (*library);
  /* an invariant, all marks <i have been kept in the pruned library */
  for (i = 0; i < count; i++)
    {
      marklist_getat (*library, i, &d);

      for (j = 0, step = *library; j < i; j++, step = step->next)
	{
	  if (NOT_SCREENED (d, step->data))
	    if (MATCH (&d, &step->data) < MATCH_VAL)
	      {
		/* [i,d] has matched with library element [j,d2]. */
		marklist_removeat (library, i);		/* because we kill it here */
		marklist_free (&avglib[d.symnum]);
		avglib[d.symnum] = NULL;
		i--, count--;
		break;
	      }
	}
    }
}


void 
prune_on_size (marklistptr * list, int sizethres)
{
  int count, i;
  marktype d;

  if (sizethres)
    {
      count = marklist_length (*list);
      i = 0;
      do
	{
	  marklist_getat (*list, i, &d);
	  if ((d.w <= sizethres) && (d.h <= sizethres))
	    {
	      marklist_removeat (list, i);
	      i--;
	      count--;
	    }
	  i++;
	}
      while (i < count);
    }
}



void 
prune_on_area (marklistptr * list, int areathres)
{
  int count, i;
  marktype d;

  if (areathres)
    {
      count = marklist_length (*list);
      i = 0;
      do
	{
	  marklist_getat (*list, i, &d);
	  if (d.set <= areathres)
	    {
	      marklist_removeat (list, i);
	      i--;
	      count--;
	    }
	  i++;
	}
      while (i < count);
    }
}


void 
averagedout_addagain (marklistptr origlist, marklistptr * librarylist)
{
  marklistptr step, stepi;
  int notfound;
  int i, j;

  for (i = 0, step = origlist; step; i++, step = step->next)
    {
      notfound = 1;

      for (j = 0, stepi = *librarylist; stepi; j++, stepi = stepi->next)
	if (NOT_SCREENED (step->data, stepi->data))
	  if (MATCH (&step->data, &stepi->data) < MATCH_VAL)
	    {
	      notfound = 0;
	      break;
	    }

      if (notfound)
	{
	  marklist_addcopy (librarylist, step->data);
	  marklist_addcopy (&avglib[libsize], step->data);
	  libsize++;
	  if (libsize >= avglibsize)
	    average_space_enlarge ();
	}
    }
}





int 
main (int argc, char *args[])
{
  int i;
  FILE *lib;

  marklistptr origlist = NULL, librarylist = NULL;
  marklistptr step;
  char tempn[1000], sysstr[1000];
  int count;
  char *libraryname = NULL, *prunename = NULL;
  int sizethres = 3, areathres = 0;
  int pass;
  int num_passes = 3;
  int overwrite = 0;

  V = 0;

  if (argc < 2)
    usage ();

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (args[i], "-v"))
	V = 1;
      else if (!strcmp (args[i], "-h"))
	usage ();
      else if (!strcmp (args[i], "-Q"))
	{
	  num_passes = 0;
	}
      else if (!strcmp (args[i], "-s"))
	{
	  i++;
	  if ((i < argc) && (isinteger (args[i])))
	    sizethres = atoi (args[i]);
	  else
	    error_msg (args[0], "follow the -s option with a numeric arg >=0", "");
	}
      else if (!strcmp (args[i], "-a"))
	{
	  i++;
	  if ((i < argc) && (isinteger (args[i])))
	    areathres = atoi (args[i]);
	  else
	    error_msg (args[0], "follow the -a option with a numeric arg >=0", "");
	}
      else if (args[i][0] == '-')
	error_msg (args[0], "unknown switch:", args[i]);
      else if (!libraryname)
	libraryname = args[i];
      else if (!prunename)
	prunename = args[i];
      else
	error_msg (args[0], "too many filenames", "");
    }

  if (!libraryname)
    error_msg (args[0], "please specify a library file", "");

  if (V)
    fprintf (stderr, "%s: processing...\n", args[0]);

  overwrite = 0;
  if (!prunename)
    {
      prunename = libraryname;
      if (V)
	fprintf (stderr, "overwriting previous library file.\n");
      overwrite = 1;
    }

  if (overwrite)
    {
      tmpnam (tempn);
    }
  else
    strcpy (tempn, prunename);


  avglibsize = count = read_library (libraryname, &origlist);
  if (V)
    fprintf (stderr, "%d marks...", count);

  avglib = (marklistptr *) malloc (count * sizeof (marklistptr));
  if (!avglib)
    error_msg (args[0], "out of memory", "");
  for (i = 0; i < avglibsize; i++)
    avglib[i] = NULL;


  prune_on_size (&origlist, sizethres);

  prune_on_area (&origlist, areathres);
  if (V)
    fprintf (stderr, "after threshold pruning, %d marks\n", count = marklist_length (origlist));


  if (V)
    fprintf (stderr, "starting greedy library creation...\n");

  greedy_add (origlist, &librarylist);


  for (pass = 0; pass < num_passes; pass++)
    {
      marklist_free (&librarylist);
      if (V)
	fprintf (stderr, "averaging...\n");
      librarylist = average_library ();
      averagedout_addagain (origlist, &librarylist);
      greedy_prune (&librarylist);
      if (V)
	fprintf (stderr, "after pass %d greedy prune %d marks\n", 1 + pass, marklist_length (librarylist));
    }


  lib = fopen (tempn, "wb");
  if (lib == NULL)
    error_msg (args[0], "trouble creating prune-library file.", "");


  if (V)
    fprintf (stderr, "writing to file: %s...", prunename);
  for (step = librarylist, count = 0; step; step = step->next, count++)
    {
      step->data.symnum = count;
      write_library_mark (lib, step->data);
    }

  if (V)
    fprintf (stderr, "%d marks.\n", count);
  fclose (lib);

  if (overwrite)
    {
      sprintf (sysstr, "mv %s %s", tempn, prunename);
      system (sysstr);		/* move temp to proper name */
    }

  for (i = 0; i < avglibsize; i++)
    {
      marklist_free (&avglib[i]);
      free (avglib[i]);
    }
  exit(0);
}
