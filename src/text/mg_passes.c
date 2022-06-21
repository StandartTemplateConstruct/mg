/**************************************************************************
 *
 * mg_passes.c -- Driver for the various passes
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
 * $Id: mg_passes.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#ifdef HAVE_MALLINFO
# include <malloc.h> 
#endif
#include <stdlib.h>
#include "memlib.h"
#include "messages.h"
#include "timing.h"

#include "longlong.h"

#include "mg_files.h"
#include "mg.h"
#include "build.h"
#include "text.h"
#include "stemmer.h"

#include "words.h"

/*
   $Log$
   Revision 1.3  2004/11/29 03:15:13  kjdon
   added some changes made by Emanuel Dejanu (Simple Words)

   Revision 1.2  2004/04/25 23:01:18  kjdon
   added a new -M option to mg_passes, allowing maxnumeric to be altered - made this change to keep gsdl3 mg inline with gsdl2 mg.

   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.3  2001/09/21 12:46:42  kjm18
   updated mg to be in line with mg_1.3f. Now uses long long for some variables
   to enable indexing of very large collections.

   Revision 1.2  2001/06/12 23:23:42  jrm21
   fixed a bug where mg_passes segfaults when trying to print the usage message.

   Revision 1.1  1999/08/10 21:18:12  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.3  1998/12/17 09:12:53  rjmcnab

   Altered mg to process utf-8 encoded Unicode. The main changes
   are in the parsing of the input, the casefolding, and the stemming.

   Revision 1.2  1998/11/25 07:55:47  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:13  rjmcnab
   *** empty log message ***

   * Revision 1.3  1994/10/20  03:56:57  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:52  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg_passes.c 16583 2008-07-29 10:20:36Z davidb $";

#define MAX_PASSES 5

#define SPECIAL 1
#define TEXT_PASS_1 2
#define TEXT_PASS_2 4
#define IVF_PASS_1 8
#define IVF_PASS_2 16

#define MIN_BUF 8192
#define	TERMRECORD	'\002'

unsigned long buf_size = 3 * 1024 * 1024;	/* 3Mb */
unsigned long invf_buffer_size = 5 * 1024 * 1024;	/* 5Mb */
unsigned long ChunkLimit = 0;
char InvfLevel = 2;
char SkipSGML = 0;
char MakeWeights = 0;
FILE *Comp_Stats = NULL;
int comp_stat_point = 0;
mg_ullong bytes_processed = 0;
mg_ullong bytes_received = 0;
int stemmer_num = 0; /* default to the lovin stemmer */
int stem_method = 0;

static char Passes = 0;
static unsigned long trace = 0;
static int Dump = 0;
static char **files = NULL;
static int num_files = 0;
static char *trace_name = NULL;


typedef struct pass_data
  {
    char *name;
    int (*init) (char *);
    int (*process) (u_char *, int);
    int (*done) (char *);
#ifdef HAVE_TIMES
    clock_t init_time;
    clock_t process_time;
    clock_t done_time;
#else
    struct timeval init_time;
    struct timeval process_time;
    struct timeval done_time;
#endif
  }
pass_data;

#ifdef HAVE_TIMES
#define NULL_TIMES 0, 0, 0
#else
#define NULL_TIMES {0, 0}, {0, 0}, {0, 0}
#endif

static pass_data PassData[MAX_PASSES] =
{
  {"special", init_special, process_special, done_special, NULL_TIMES},
  {"text.pass1", init_text_1, process_text_1, done_text_1, NULL_TIMES},
  {"text.pass2", init_text_2, process_text_2, done_text_2, NULL_TIMES},
  {"ivf.pass1", init_ivf_1, process_ivf_1, done_ivf_1, NULL_TIMES},
  {"ivf.pass2", init_ivf_2, process_ivf_2, done_ivf_2, NULL_TIMES},
};

static char *usage_str = "\nUSAGE:\n"
"  %s [-h] [-G] [-D] [-1|-2|-3] [-T1] [-T2] [-I1] [-I2] [-N1]\n"
"  %*s [-N2] [-W] [-S] [-b buffer-size] [-d dictionary-directory]\n"
"  %*s [-t trace-point Mb] [-m invf-memory] [-c chunk-limit]\n"
"  %*s [-n trace-name] [-C comp-stat-size] [-s stem_method]\n"
"  %*s [-a stemmer] [-M max-numeric] -f doc-collection-name\n";


static void 
usage (char *err)
{
  if (err)
    Message (err);
  fprintf (stderr, usage_str, msg_prefix, strlen (msg_prefix), "",
	   strlen (msg_prefix), "",strlen (msg_prefix), "",
	   strlen (msg_prefix),"");
  exit (1);
}




#if 0
static char *
str_comma (unsigned long u)
{
  static char buf[20];
  unsigned long a, b, c, d;
  a = u / 1000000000;
  u -= a * 1000000000;
  b = u / 1000000;
  u -= b * 1000000;
  c = u / 1000;
  u -= c * 1000;
  d = u;

  if (a)
    sprintf (buf, "%u,%03u,%03u,%03u", a, b, c, d);
  else if (b)
    sprintf (buf, "%u,%03u,%03u", b, c, d);
  else if (c)
    sprintf (buf, "%u,%03u", c, d);
  else
    sprintf (buf, "%u", d);
  return (buf);
}
#endif




int 
open_next_file (int in_fd)
{
  if (in_fd > 0)
    close (in_fd);
  if (num_files == 0)
    return (-1);
  if ((in_fd = open (files[0], O_RDONLY)) == -1)
    FatalError (1, "Cannot open %s", files[0]);
  files++;
  num_files--;
  return (in_fd);
}


static void 
driver (int in_fd, FILE * Trace, char *file_name)
{
  int pass, num = 1;

  char *buffer = Xmalloc (buf_size);
  unsigned long num_docs = 0;
  unsigned long block_bytes = 0;
  register int buf_left = buf_size;
  register char *look_pos = buffer;
  register char *end_pos = buffer;

  ProgTime StartTime, InitTime, ProcTime, DoneTime;

  GetTime (&StartTime);

  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
      {
	pass_data *pd = &PassData[pass];
#ifdef HAVE_TIMES
	struct tms tims;
	times (&tims);
	pd->init_time -= tims.tms_utime + tims.tms_stime;
#elif defined(HAVE_GETRUSAGE)  /* [RPAP - Feb 97: WIN32 Port] */
	struct rusage ru;

	getrusage (RUSAGE_SELF, &ru);
	pd->init_time.tv_sec -= ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	pd->init_time.tv_usec -= ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
#endif
	if (pd->init (file_name) == COMPERROR)
	  FatalError (1, "Error during init of \"%s\"", pd->name);

#ifdef HAVE_TIMES
	times (&tims);
	pd->init_time += tims.tms_utime + tims.tms_stime;
#elif defined(HAVE_GETRUSAGE)  /* [RPAP - Feb 97: WIN32 Port] */
	getrusage (RUSAGE_SELF, &ru);
	pd->init_time.tv_sec += ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	pd->init_time.tv_usec += ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	time_normalise (&pd->init_time);
#endif
      }

  GetTime (&InitTime);
  for (;;)
    {
      int len = 0;
      char *base = look_pos;

      while (look_pos != end_pos && *look_pos != TERMRECORD)
	look_pos++;

      while (look_pos == end_pos)
	{
	  if (buf_left < MIN_BUF)
	    {
	      bcopy (base, buffer, end_pos - base);
	      look_pos = buffer + (end_pos - base);
	      buf_left = buf_size - (end_pos - base);
	      end_pos = look_pos;
	      base = buffer;
	    }
	  if (buf_left)
	    {
    	      num = read (in_fd, end_pos, buf_left);
	      if (num < 0) num = 0; /* RJM - quick hack :-) */
	      if (num == 0)
		if ((in_fd = open_next_file (in_fd)) != -1)
		  num = read (in_fd, end_pos, buf_left);
	      bytes_received += num;
	      buf_left -= num;
	      end_pos += num;
	    }
	  while (look_pos < end_pos && *look_pos != TERMRECORD)
	    look_pos++;
	  if (buf_left == 0 && base == buffer && look_pos == end_pos)
	    {
	      Message ("Unable to find document terminator (i.e ^B)"
		       " in the document");
	      FatalError (1, "The document is in excess of %d chars long",
			  look_pos - base);
	    }
	  if (!num)
	    break;
	}
      len = look_pos++ - base;

      if (!num && base == end_pos)
	break;

      bytes_processed += len;

#ifndef QUIET
      if (!len)
	Message ("Warning : Processing zero length document");
#endif

      for (pass = 0; pass < MAX_PASSES; pass++)
	if (Passes & (1 << pass))
	  {
	    register pass_data *pd = &PassData[pass];

#ifdef HAVE_TIMES
	    struct tms tims;
	    times (&tims);
	    pd->process_time -= tims.tms_utime + tims.tms_stime;
#elif defined(HAVE_GETRUSAGE)  /* [RPAP - Feb 97: WIN32 Port] */
	    struct rusage ru;
	    register struct timeval *tv = &pd->process_time;

	    getrusage (RUSAGE_SELF, &ru);
	    tv->tv_sec -= ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	    tv->tv_usec -= ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
#endif

	    if (pd->process ((u_char *) base, len) == COMPERROR)
	      {
		Message ("Error during processing of \"%s\"", pd->name);
		if (Dump || Trace)
		  {
		    int i;
		    FILE *f = Trace ? Trace : stderr;
		    fprintf (f, "-=- * -=- * -=- * -=- * -=- * -=- * -=-\n");
		    for (i = 0; i < len; i++)
		      {
			char ch = base[i];
			if (ch == '\1' || ch == '\2')
			  ch = '\n';
			putc (ch, f);
		      }
		    fprintf (f, "-=- * -=- * -=- * -=- * -=- * -=- * -=-\n");
		  }
		if (Trace)
		  fprintf (Trace, "%11" ULL_FS " bytes |%7lu docs | %s\n",
			   bytes_processed, num_docs,
			   ElapsedTime (&StartTime, NULL));
		exit (1);
	      }

#ifdef HAVE_TIMES
	    times (&tims);
	    pd->process_time += tims.tms_utime + tims.tms_stime;
#elif defined(HAVE_GETRUSAGE)  /* [RPAP - Feb 97: WIN32 Port] */
	    getrusage (RUSAGE_SELF, &ru);
	    tv->tv_sec += ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	    tv->tv_usec += ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
#endif
	  }
      num_docs++;
      if (Trace)
	{
	  block_bytes += (look_pos - base);
	  if (block_bytes >= trace)
	    {
#ifdef HAVE_MALLINFO
	      struct mallinfo mi;
	      mi = mallinfo ();
	      block_bytes -= trace;
	      fprintf (Trace, "%11" ULL_FS " bytes |%7lu docs |%7.3f Mb | %s\n",
		       bytes_processed, num_docs, mi.arena / 1024.0 / 1024.0,
		       ElapsedTime (&StartTime, NULL));
#else
	      block_bytes -= trace;
	      fprintf (Trace, "%11" ULL_FS " bytes |%7lu docs | %s\n",
		       bytes_processed, num_docs,
		       ElapsedTime (&StartTime, NULL));
#endif
	    }
	}
      if (!num && look_pos - 1 == end_pos)
	break;
    }

#ifndef HAVE_TIMES
  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
      time_normalise (&PassData[pass].process_time);
#endif

  GetTime (&ProcTime);

  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
      {
	pass_data *pd = &PassData[pass];
#ifdef HAVE_TIMES
	struct tms tims;
	times (&tims);
	pd->done_time -= tims.tms_utime + tims.tms_stime;
#elif defined(HAVE_GETRUSAGE)  /* [RPAP - Feb 97: WIN32 Port] */
	struct rusage ru;

	getrusage (RUSAGE_SELF, &ru);
	pd->done_time.tv_sec -= ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	pd->done_time.tv_usec -= ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
#endif
	if (pd->done (file_name) == COMPERROR)
	  FatalError (1, "Error during done of \"%s\"", pd->name);

#ifdef HAVE_TIMES
	times (&tims);
	pd->done_time += tims.tms_utime + tims.tms_stime;
#elif defined(HAVE_GETRUSAGE)  /* [RPAP - Feb 97: WIN32 Port] */
	getrusage (RUSAGE_SELF, &ru);
	pd->done_time.tv_sec += ru.ru_utime.tv_sec + ru.ru_stime.tv_sec;
	pd->done_time.tv_usec += ru.ru_utime.tv_usec + ru.ru_stime.tv_usec;
	time_normalise (&pd->done_time);
#endif
      }
  if (Trace)
    {
#ifdef HAVE_MALLINFO
      struct mallinfo mi;
      mi = mallinfo ();
      fprintf (Trace, "%11" ULL_FS " bytes |%7lu docs |%7.3f Mb | %s\n",
	       bytes_processed, num_docs, mi.arena / 1024.0 / 1024.0,
	       ElapsedTime (&StartTime, NULL));
#else
      fprintf (Trace, "%11" ULL_FS " bytes |%7lu docs | %s\n",
	       bytes_processed, num_docs,
	       ElapsedTime (&StartTime, NULL));
#endif
    }

  GetTime (&DoneTime);

  Message ("");
  Message ("%10s      :    init         process        done", "");
  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
      {
	pass_data *pd = &PassData[pass];
	char it[15], pt[15], dt[15];
#ifdef HAVE_TIMES
	strcpy (it, cputime_string (pd->init_time));
	strcpy (pt, cputime_string (pd->process_time));
	strcpy (dt, cputime_string (pd->done_time));
#else
	strcpy (it, cputime_string (&pd->init_time));
	strcpy (pt, cputime_string (&pd->process_time));
	strcpy (dt, cputime_string (&pd->done_time));
#endif
	Message ("%-10s      : %s   %s   %s", pd->name, it, pt, dt);
      }
  Message ("");
  Message ("Init time       : %s", ElapsedTime (&StartTime, &InitTime));
  Message ("Process time    : %s", ElapsedTime (&InitTime, &ProcTime));
  Message ("Done time       : %s", ElapsedTime (&ProcTime, &DoneTime));
  Message ("Total time      : %s", ElapsedTime (&StartTime, &DoneTime));
  Message ("Documents       : %u", num_docs);
  Message ("Bytes received  : %" ULL_FS, bytes_received);
  Message ("Bytes processed : %" ULL_FS, bytes_processed);
  Message ("Process Rate    : %.1f kB per cpu second",
   (double) bytes_processed / (ProcTime.CPUTime - InitTime.CPUTime) / 1024);
  free (buffer);
}



int main (int argc, char **argv)
{
  int ch, in_fd;
  char *filename = NULL;
  FILE *Trace = NULL;

  msg_prefix = argv[0];

  opterr = 0;
  while ((ch = getopt (argc, argv, "hC:WGSD123f:d:b:T:I:t:m:N:c:n:s:a:M:")) != -1)
    {
      switch (ch)
	{
	case 'G':
	  SkipSGML = 1;
	  break;
	case 'S':
	  Passes |= SPECIAL;
	  break;
	case '1':
	  InvfLevel = 1;
	  break;
	case '2':
	  InvfLevel = 2;
	  break;
	case '3':
	  InvfLevel = 3;
	  break;
	case 'f':
	  filename = optarg;
	  break;
	case 'n':
	  trace_name = optarg;
	  break;
	case 'D':
	  Dump = 1;
	  break;
	case 'W':
	  MakeWeights = 1;
	  break;
	case 'd':
	  set_basepath (optarg);
	  break;
	case 'a':
	  stemmer_num = stemmernumber (optarg);
	  break;
	case 's':
	  stem_method = atoi (optarg) & STEMMER_MASK;
	  break;
	case 'b':
	  buf_size = atoi (optarg) * 1024;
	  break;
	case 'C':
	  comp_stat_point = atoi (optarg) * 1024;
	  break;
	case 'c':
	  ChunkLimit = atoi (optarg);
	  break;
	case 'm':
	  invf_buffer_size = (int) (atof (optarg) * 1024 * 1024);
	  break;
	case 'I':
	case 'N': /* N kept for compatability */
	  if (*optarg == '1')
	    Passes |= IVF_PASS_1;
	  else if (*optarg == '2')
	    Passes |= IVF_PASS_2;
	  else
	    usage ("Invalid pass number");
	  break;
	case 'T':
	  if (*optarg == '1')
	    Passes |= TEXT_PASS_1;
	  else if (*optarg == '2')
	    Passes |= TEXT_PASS_2;
	  else
	    usage ("Invalid pass number");
	  break;
	case 't':
	  trace = (unsigned long) (atof (optarg) * 1024 * 1024);
	  break;
	case 'M':
	  SetEnv ("maxnumeric", optarg, NULL);
	  break;
	case 'h':
	case '?':
	  usage (NULL);
	}
    }

  if (!filename || *filename == '\0')
    FatalError (1, "A document collection name must be specified.");

  if (buf_size < MIN_BUF)
    FatalError (1, "The buffer size must exceed 1024 bytes.");

  if ((Passes & (IVF_PASS_1 | IVF_PASS_2)) == (IVF_PASS_1 | IVF_PASS_2))
    FatalError (1, "I1 and I2 cannot be done simultaneously.");

  if ((Passes & (TEXT_PASS_1 | TEXT_PASS_2)) == (TEXT_PASS_1 | TEXT_PASS_2))
    FatalError (1, "T1 and T2 cannot be done simultaneously.");

  if (!Passes)
    FatalError (1, "S, T1, T2, I1 or I2 must be specified.");

  if (optind < argc)
    {
      if ((in_fd = open (argv[optind], O_RDONLY)) == -1)
	FatalError (1, "Cannot open %s", argv[optind]);
      files = &argv[optind + 1];
      num_files = argc - (optind + 1);
    }
  else
    in_fd = 0;			/* stdin */


  if (trace)
    {
      if (!trace_name)
	trace_name = make_name (filename, TRACE_SUFFIX, NULL);
      if (!(Trace = fopen (trace_name, "a")))
	Message ("Unable to open \"%s\". No tracing will be done.", trace_name);
      else
	setbuf (Trace, NULL);
    }
  else
    Trace = NULL;

  if (comp_stat_point)
    {
      char *name = make_name (filename, COMPRESSION_STATS_SUFFIX, NULL);
      if (!(Comp_Stats = fopen (name, "wb")))  /* [RPAP - Feb 97: WIN32 Port] */
	Message ("Unable to open \"%s\". No comp. stats. will be generated.",
		 name);
    }


  if (Trace)
    {
      int i;
      fprintf (Trace, "\n\n\t\t-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n");
      for (i = 0; i < argc; i++)
	fprintf (Trace, "%s ", argv[i]);
      fprintf (Trace, "\n\n");
    }

  driver (in_fd, Trace, filename);

  if (Trace)
    fclose (Trace);

  if (Comp_Stats)
    fclose (Comp_Stats);

  return 0;
}
