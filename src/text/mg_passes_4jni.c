/**************************************************************************
 *
 * mg_passes_4jni.c -- Driver for the various passes
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
 * $Id: mg_passes_4jni.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/
/* this needs to come first */
#include "sysfuncs.h"

#ifdef HAVE_MALLINFO
# include <malloc.h> 
#endif

#include "mg_passes_4jni.h"
#include "memlib.h"
#include "messages.h"
#include "timing.h"

#include "longlong.h"
#include "stemmer.h"

#include "mg_files.h"
#include "mg.h"
#include "build.h"
#include "text.h"

#include "words.h"
#include "environment.h"

static char *RCSID = "$Id: mg_passes_4jni.c 16583 2008-07-29 10:20:36Z davidb $";

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
FILE * Trace;
char * filename;
unsigned long num_docs = 0;
unsigned long block_bytes = 0;

static char Passes = 0;
static unsigned long trace = 0;
static int Dump = 0;
static char **files = NULL;
static int num_files = 0;
static char *trace_name = NULL;

int mg_passes_exit_value = 0;

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


/* clear all the settings from one mg_passes run to the next */
void clear_variables() {

  buf_size = 3 * 1024 * 1024;	/* 3Mb */
  invf_buffer_size = 5 * 1024 * 1024;	/* 5Mb */
  ChunkLimit = 0;
  InvfLevel = 2;
  SkipSGML = 0;
  MakeWeights = 0;
  Comp_Stats = NULL;
  comp_stat_point = 0;
  bytes_processed = 0;
  bytes_received = 0;
  stemmer_num = 0; /* default to the lovin stemmer */
  stem_method = 0;
  Trace = NULL;
  filename = NULL;
  num_docs = 0;
  block_bytes = 0;
  
  Passes = 0;
  trace = 0;
  Dump = 0;
  files = NULL;
  num_files = 0;
  trace_name = NULL;
  
  mg_passes_exit_value = 0;
}

/* ################################################## */
/* the following are methods to set all the variables that used to be
   set by command line args */

/* -S, -T1, -T2, -I1, -I2, args to mg_passes */
void add_pass (char pass_type, char pass_num) {

  switch(pass_type) {
  case 'S':
    Passes |= SPECIAL;
    break;
  case 'I':
  case 'N':
    if (pass_num == '1')
      Passes |= IVF_PASS_1;
    else if (pass_num == '2')
      Passes |= IVF_PASS_2;
    else
      fprintf(stderr, "Invalid pass number %c for pass type %c\n", pass_num, pass_type);
    break;
  case 'T':
    if (pass_num == '1')
      Passes |= TEXT_PASS_1;
    else if (pass_num == '2')
      Passes |= TEXT_PASS_2;
    else
      fprintf(stderr, "Invalid pass number %c for pass type %c\n", pass_num, pass_type);
    break;
  }
}

/* -D arg to mg_passes */
void dump_failed_document(int dump) {
  Dump = dump;
}

/* -G arg to mg_passes */
void ignore_sgml_tags(int ignore) {
  if (ignore) {
    SkipSGML = 1;
  } else {
    SkipSGML = 0;
  }
}

/* -b arg to mg_passes */
void set_buffer_size(long size) {
  buf_size = size * 1024;
  if (buf_size <  MIN_BUF) {
    buf_size = MIN_BUF;
  }
}

/* -c arg to mg_passes */
void set_chunk_limit(long chunk_limit) {
  ChunkLimit = chunk_limit;
}

/* -C arg to mg_passes */
void set_comp_stat_point(int stat_point) {
  comp_stat_point = stat_point * 1024;
}

/* -f arg to mg_passes */
void set_filename(const char * filen) {
  if (filename) {
    Xfree (filename);
    filename = NULL;
  }
  filename = Xstrdup (filen);  
}

/* -m arg to mg_passes */
void set_inversion_limit(int limit) {
  invf_buffer_size = limit * 1024 * 1024;
}

/* -1, -2, -3 args to mg_passes */
void set_invf_level(char level) {
  switch (level) {
  case '1':
    InvfLevel = 1;
    break;
  case '2':
    InvfLevel = 2;
    break;
  case '3':
    InvfLevel = 3;
    break;
  }
}

/* -W arg to mg_passes */
void set_make_weights(int make_w) {
  MakeWeights = make_w;
}

/* -M arg to mg_passes */
void set_max_numeric(int max_numeric) {
  char data[99];
  sprintf(data, "%d", max_numeric);
  SetEnv ("maxnumeric", data, NULL);
}

/* -a, -s args to mg_passes */
void set_stem_options(const char * stemmer, int method) {
  stemmer_num = stemmernumber (stemmer);
  stem_method = method & STEMMER_MASK;
}

/* -t arg to mg_passes */
void set_trace_point(int tracepos) {
  trace = (unsigned long) (tracepos * 1024 * 1024);
}

/* -n arg to mg_passes */
void set_trace_file(const char * filen) {
  if (trace_name) {
    Xfree (trace_name);
    trace_name = NULL;
  }
  trace_name = Xstrdup (filen);
}

/* ############################################### */
/* The old driver method has been split into 3:
init_driver, process_document (called numdocs times), 
finalise_driver.
The above set vars methods should all be called before init_driver.
*/


ProgTime StartTime, InitTime, ProcTime, DoneTime;

void 
init_driver ()
{
  int pass;
  if (!filename || *filename == '\0') {
    mg_passes_exit_value = 1;
    FatalError (1, "A document collection name must be specified.");
  }

  if ((Passes & (IVF_PASS_1 | IVF_PASS_2)) == (IVF_PASS_1 | IVF_PASS_2)) {
    mg_passes_exit_value = 1;
    FatalError (1, "I1 and I2 cannot be done simultaneously.");

  }
  if ((Passes & (TEXT_PASS_1 | TEXT_PASS_2)) == (TEXT_PASS_1 | TEXT_PASS_2)) {
    mg_passes_exit_value = 1;
    FatalError (1, "T1 and T2 cannot be done simultaneously.");
  }
  if (!Passes) {
    mg_passes_exit_value = 1;
    FatalError (1, "S, T1, T2, I1 or I2 must be specified.");
  }
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
      fprintf (Trace, "\n\n\t\t-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n\n");
      /* print out the args to mg_passes */
      fprintf (Trace, "\n\n");
    }


  GetTime (&StartTime);
  
  for (pass = 0; pass < MAX_PASSES; pass++) {
    if (Passes & (1 << pass)) {
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
      if (pd->init (filename) == COMPERROR) {
	mg_passes_exit_value = 1;
	FatalError (1, "Error during init of \"%s\"", pd->name);
      }
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
  }
  GetTime (&InitTime);
}


void process_document(const u_char *buffer, int len) {
  int pass;
  bytes_processed += len;

#ifndef QUIET
  if (!len)
    Message ("Warning : Processing zero length document");
#endif
  
  for (pass = 0; pass < MAX_PASSES; pass++) {
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
	if (pd->process ((u_char *) buffer, len) == COMPERROR)
	  {
	    Message ("Error during processing of \"%s\"", pd->name);
	    if (Dump || Trace)
	      {
		int i;
		FILE *f = Trace ? Trace : stderr;
		fprintf (f, "-=- * -=- * -=- * -=- * -=- * -=- * -=-\n");
		for (i = 0; i < len; i++)
		  {
		    char ch = buffer[i];
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
  }
  num_docs++;
  if (Trace)
    {
      block_bytes += len;
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
}

void finalise_driver() {
  int pass;
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
	if (pd->done (filename) == COMPERROR) {
	  mg_passes_exit_value = 1;
	  FatalError (1, "Error during done of \"%s\"", pd->name);
	}
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
  //free (buffer);

  if (Trace)
    fclose (Trace);

  if (Comp_Stats)
    fclose (Comp_Stats);

}

int get_exit_value() {
  return mg_passes_exit_value;
}
