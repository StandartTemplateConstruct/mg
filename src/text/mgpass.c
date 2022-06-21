/**************************************************************************
 *
 * mgpass.cpp -- Driver for the various passes -

	V1 -  removed all the pipe processing and replaced with
	      code to directly explore a directory of files.
	V2 -  rebuilt to extract non text files from web browser
	      'catch' file.  Also to display progress count
			   GH/WJR

 * Copyright (C) 1994  Neil Sharman, ..
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
 * $Id: mgpass.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

#include "memlib.h"
#include "messages.h"

#include "longlong.h"

#include "mg_files.h"
#include "mg.h"
#include "build.h"
#include "text.h"
#include "stemmer.h"

/*
   $Log$
   Revision 1.1  2003/02/20 21:18:23  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.2  2001/09/20 02:32:00  cs025
   a few fixes for compiling under windows with new long long stuff in mg

   Revision 1.1  1999/08/10 21:17:39  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.5  1998/12/02 01:32:40  sjboddie
   fixed 1 missing semi-colon..

   Revision 1.4  1998/12/02 00:57:10  rjmcnab

   Fixed to compile with the changes to the stemming.

   Revision 1.3  1998/11/24 03:39:23  rjmcnab

   Fixed a small error in the windows compilation

   Revision 1.2  1998/11/24 01:29:39  rjmcnab

   Fixed a few problems with the windows build

   Revision 1.1  1998/11/17 09:34:12  rjmcnab
   *** empty log message ***

   * Revision 1.3  1994/10/20  03:56:57  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:52  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mgpass.c 16583 2008-07-29 10:20:36Z davidb $";

#define MAX_PASSES 5

#define SPECIAL 1
#define TEXT_PASS_1 2
#define TEXT_PASS_2 4
#define IVF_PASS_1 8
#define IVF_PASS_2 16

#define MIN_BUF 8192
#define path_length 256

unsigned long buf_size = 3 * 1024 * 1024;       /* 3Mb */
unsigned long invf_buffer_size = 5 * 1024 * 1024;       /* 5Mb */
unsigned long ChunkLimit = 0;
char InvfLevel = 2;
char SkipSGML = 0;
char MakeWeights = 0;
FILE *Comp_Stats = NULL;
int comp_stat_point = 0;
mg_ullong bytes_processed = 0;
int num_docs = 0;
mg_ullong bytes_received = 0;
int stem_method = 0;
int stemmer_num = 0; /* default to the Lovin stemmer */

static char Passes = 0;
static unsigned long trace = 0;
static int Dump = 0;
static char **files = NULL;
static int num_files = 0;
static char *trace_name = NULL;

static char dirname[path_length], wildname[path_length];
static int by_para = 0, recurse = 0, html_catch = 0;
static char *buffer;
char *line_start, *data_end, *base, *scan;

typedef struct pass_data
  {
    char *name;
    int (*init) (char *);
    int (*process) (u_char *, int);
    int (*done) (char *);
  }
pass_data;

static pass_data PassData[MAX_PASSES] =
{
  {"special", init_special, process_special, done_special},
  {"text.pass1", init_text_1, process_text_1, done_text_1},
  {"text.pass2", init_text_2, process_text_2, done_text_2},
  {"ivf.pass1", init_ivf_1, process_ivf_1, done_ivf_1},
  {"ivf.pass2", init_ivf_2, process_ivf_2, done_ivf_2},
};

static char *usage_str = "\nUSAGE:\n"
"  %s [-h] [-G] [-D] [-1|-2|-3] [-T1] [-T2] [-I1] [-I2] [-N1]\n"
"  %*s [-N2] [-W] [-S] [-b buffer-size] [-d dictionary-directory]\n"
"  %*s [-t trace-point Mb] [-m invf-memory] [-c chunk-limit]\n"
"  %*s [-n trace-name] [-C comp-stat-size] [-s stem_method] -f doc-collection-name\n"
"  %*s [source directory\\] [source file]\n";

static void usage (char *err)
{
  if (err) Message (err);
  fprintf (stderr, usage_str, msg_prefix, strlen (msg_prefix), "",
    strlen (msg_prefix), "", strlen (msg_prefix), "");
  exit (1);
}

void do_process(char *buffer, int num)
{
  int pass;
  if (num == 0) Message ("Warning : Processing zero length document");
  num_docs++;
  bytes_processed += num;
  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
	if (PassData[pass].process((u_char *)buffer, num) == COMPERROR)
	   FatalError(1, "Error during file processing");
}

int refill(int in_fd)
{
  int num, bitleft;
  bitleft = data_end - base;
  memmove(buffer, base, bitleft);
  if (buf_size - bitleft < MIN_BUF)
    FatalError(1, "Paragraph too big for buffer");
  num = read(in_fd, &buffer[bitleft], buf_size - bitleft);
  line_start -= (base - buffer);
  scan -= (base - buffer);
  base = buffer;
  data_end = buffer + bitleft + num;
  if (num > 0) return 1; else return 0;
}

char *scanpara(int in_fd)
{
  int num, blank, in_blank, at_end;
  num = read(in_fd,buffer,buf_size);
  in_blank = 1;  base = buffer;  line_start = buffer;  scan = buffer;
  data_end = buffer + num;  at_end = 0;
  for (;;) {
    blank = 1;  line_start = scan;                   // Get a line
    while (scan < data_end  &&  *scan != '\n')
      if (*scan++ > ' ') blank = 0;
    if (scan >= data_end) {
      if (refill(in_fd)) {
	while (scan < data_end  &&  *scan != '\n')
	  if (*scan++ > ' ') blank = 0;
	if (scan < data_end) scan++;
	}
      else at_end = 1;
      }
    else scan++;
    if (line_start < scan) {                         // If we have a line
      if (in_blank) {
	if (!blank) { in_blank = 0; base = line_start; }
	}
      else {
	if (blank) {
	  do_process(base, line_start - base);
	  in_blank = 1;  base = line_start;
	  }
	}
      }
    if (at_end) break;
    }
  if (in_blank) base = scan;
  if (scan + 2 <= buffer + buf_size) {
    *scan++ = 26;  *scan++ = 10;
    }
  if (base < scan) do_process(base, scan - base);
  return NULL;
}

char *scanfile(int in_fd)
{
  int num = read(in_fd,buffer,buf_size);  /*expect to read the whole file*/
  if (num < 0) return "file locked";
  if (num >= buf_size-1) return "file too long";
  do_process(buffer, num);
  return NULL;
}

void search(char *dname, char *fname)
{
  long dirtag;  struct _finddata_t dirinfo;  int in_fd;  char *res;
  char search_name[path_length], found_name[path_length];

  sprintf(search_name, "%s%s", dname, fname);    /*Scan files*/
  dirtag = _findfirst(search_name, &dirinfo);
  if (dirtag >= 0) {
    do {
      if ((dirinfo.attrib & (_A_SUBDIR | _A_HIDDEN | _A_SYSTEM)) == 0) {
	sprintf(found_name,"%s%s",dname,dirinfo.name);
	in_fd = open(found_name,O_RDONLY|O_BINARY);
	if (in_fd >= 0) {
	  if (by_para)
	    res = scanpara(in_fd);
	  else
	    res = scanfile(in_fd);
	  if (res != NULL) {
	    Message("Error %s in processing file %s\n", res, found_name);
	    exit(1);
	    }
	  close(in_fd);
	  }
	}
      } while (_findnext(dirtag, &dirinfo) == 0);
    _findclose(dirtag);
    }

  if (recurse == 0) return;

  sprintf(search_name, "%s*.*", dname);          /*Look for subdirs*/
  dirtag = _findfirst(search_name, &dirinfo);
  if (dirtag < 0) return;
  do {
    if ( ((dirinfo.attrib & (_A_HIDDEN | _A_SYSTEM)) == 0)  &&
	 ((dirinfo.attrib & _A_SUBDIR) != 0)  &&
	 strcmp(dirinfo.name,".") != 0 &&
	 strcmp(dirinfo.name,"..") != 0) {
      sprintf(found_name,"%s%s",dname,dirinfo.name);
      strcat(found_name,"\\");
      search(found_name,fname);
      }
    } while (_findnext(dirtag, &dirinfo) == 0);
  _findclose(dirtag);
}

static int toobig(int n)
{
  if (n > path_length) {
    printf("Cannot handle urls > %d characters in length\n", path_length);
    exit(1);
    }
  return 0;
}

void scan_catch(char *dname, char *fname)
{
  int in_fd, urllen, conlen, filesize;  char catch_name[path_length];
  int filecount = 0;
  enum { filekind_redirected, filekind_text, filekind_other } filekind;
  sprintf(catch_name, "%s%s", dname, fname);
  in_fd = open(catch_name,O_RDONLY|O_BINARY);
  if (in_fd < 0)
    FatalError(1, "Couldn't open catch file \"%s\"", catch_name);
  for (;;) {
    filecount++;  if (filecount%100 == 0) { printf("%d\r", filecount);  fflush(stdout); }
    if (read(in_fd, &urllen, sizeof(int)) != sizeof(int) || toobig(urllen) ||
	read(in_fd, catch_name, urllen) != urllen ||
	read(in_fd, &conlen, sizeof(int)) != sizeof(int) || toobig(conlen) ||
	read(in_fd, catch_name, conlen) != conlen) break;
    if (conlen >= 1 && *catch_name == '@')
      filekind = filekind_redirected;
    else {
      if (conlen >= 4 && strncmp(catch_name, "text", 4) == 0)
	filekind = filekind_text;
      else
	filekind = filekind_other;
      if (read(in_fd, &filesize, sizeof(int)) != sizeof(int))
	FatalError(1, "File read failed for size field");
      if (filesize > buf_size)
	FatalError(1, "File too large (%d > %d)", filesize, buf_size);
      if (read(in_fd, buffer, filesize) != filesize)
	FatalError(1, "Failed to read file data");
      }
    if (filekind == filekind_text) do_process(buffer, filesize);
    }
  close(in_fd);
}

static void driver (FILE * Trace, char *file_name)
{
  int pass;

  buffer = (char *)Xmalloc (buf_size);

  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
      {
	if (PassData[pass].init (file_name) == COMPERROR)
	  FatalError (1, "Error during init of \"%s\"",PassData[pass].name);
      }

  if (html_catch == 0) search(dirname, wildname);
		  else scan_catch(dirname, wildname);

  for (pass = 0; pass < MAX_PASSES; pass++)
    if (Passes & (1 << pass))
      {
	if (PassData[pass].done (file_name) == COMPERROR)
	  FatalError (1, "Error during done of \"%s\"", PassData[pass].name);
      }


  free (buffer);
}

void main (int argc, char **argv)
{
  int ch;
  char *filename = NULL;
  FILE *Trace = NULL;

  msg_prefix = argv[0];

  opterr = 0;
  while ((ch = getopt (argc, argv, "hC:WHGpSD123f:d:b:T:I:t:m:N:c:n:s:")) != -1)
    {
      switch (ch)
	{
	case 'H':
	  html_catch = 1;
	  break;
	case 'G':
	  SkipSGML = 1;
	  break;
	case 'p':
	  by_para = 1;
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

  if (argc - optind == 1) {
    strcpy(dirname,"");
    strcpy(wildname,argv[optind]);
    }
  else if (argc - optind == 2) {
    strcpy(dirname,argv[optind]);
    strcpy(wildname,argv[optind+1]);
    }
  else FatalError(1, "Finder code requires directory and filespec.");

  if (strrchr(wildname,'*') != NULL  ||  strrchr(wildname,'?') != NULL)
    recurse = 1;

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
      if (!(Comp_Stats = fopen (name, "wb")))
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

  driver (Trace, filename);

  if (Trace)
    fclose (Trace);

  if (Comp_Stats)
    fclose (Comp_Stats);

  exit (0);
}
