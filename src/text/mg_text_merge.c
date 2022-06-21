/**************************************************************************
 *
 * mg_text_merge.c --- merge  *.text, *.text.idx files
 *                     part of the mgmerge utility
 * Copyright (C) 1995 Shane Hudson (shane@cosc.canterbury.ac.nz)
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
 * $Id$
 * Last edited:  November 11 1994
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "locallib.h"
#include "messages.h"
#include "timing.h"

#include "mg.h"
#include "mg_merge.h"
#include "mg_files.h"
#include "text.h"


/**** GLOBALS ****/
FILE *text[3], *idx[3];

typedef char FileName[256];
FileName old_name, new_name, merge_name;

long magicsize;			/* == where the header in a file begins */

compressed_text_header cth[3];

/*=======================================================================
 * init_merge_text(): open files, set up global variables, etc
 *=======================================================================*/
int 
init_merge_text ()
{

  /* open .text files */
  text[OLD] = open_file (old_name, TEXT_SUFFIX, "r+",
			 MAGIC_TEXT, MG_ABORT);
  magicsize = ftell (text[OLD]);
  fread (&cth[OLD], sizeof (cth[OLD]), 1, text[OLD]);

  text[NEW] = open_file (new_name, TEXT_SUFFIX, "rb+",
			 MAGIC_TEXT, MG_ABORT);
  fread (&cth[NEW], sizeof (cth[NEW]), 1, text[NEW]);

  /* open .text.idx files */
  idx[OLD] = open_file (old_name, TEXT_IDX_SUFFIX, "rb+",
			MAGIC_TEXI, MG_ABORT);
  fread (&cth[OLD], sizeof (cth[OLD]), 1, idx[OLD]);

  idx[NEW] = open_file (new_name, TEXT_IDX_SUFFIX, "rb+",
			MAGIC_TEXI, MG_ABORT);
  fread (&cth[NEW], sizeof (cth[NEW]), 1, idx[NEW]);

  idx[MERGE] = create_file (merge_name, TEXT_IDX_SUFFIX, "wb",
			    MAGIC_TEXI, MG_ABORT);
  return OK;
}


/*=======================================================================
 * process_merge_text(): merge the files
 *=======================================================================*/
int 
process_merge_text (void)
{
  int i;
  u_long data, offset;
  byte c;

  /* update and write merged header to .text and .text.idx files */
  /* they have the exact same header */
  cth[MERGE].num_of_docs = cth[OLD].num_of_docs
    + cth[NEW].num_of_docs;
  cth[MERGE].num_of_bytes = cth[OLD].num_of_bytes
    + cth[NEW].num_of_bytes;
  cth[MERGE].num_of_words = cth[OLD].num_of_words;
  cth[MERGE].length_of_longest_doc =
    (cth[OLD].length_of_longest_doc > cth[NEW].length_of_longest_doc
     ? cth[OLD].length_of_longest_doc
     : cth[NEW].length_of_longest_doc);
  cth[MERGE].ratio = ((cth[OLD].num_of_bytes * cth[OLD].ratio) +
		      (cth[NEW].num_of_bytes * cth[NEW].ratio))
    / cth[MERGE].num_of_bytes;
  fwrite (&cth[MERGE], sizeof (cth[MERGE]), 1, idx[MERGE]);
  fseek (text[OLD], magicsize, 0);
  fwrite (&cth[MERGE], sizeof (cth[MERGE]), 1, text[OLD]);

  /*
   *  Update *.text.idx:  need to know where each new doc starts
   *  in the appended .text file
   */
  for (i = 0; i < cth[OLD].num_of_docs; i++)
    {
      fread (&data, sizeof (u_long), 1, idx[OLD]);
      fwrite (&data, sizeof (u_long), 1, idx[MERGE]);
    }

  /* offset is the amount to add to each entry from idx[NEW] */
  fread (&offset, sizeof (u_long), 1, idx[OLD]);
  offset -= (4 + sizeof (cth[OLD]));	/* 4 for the magic number */

  for (i = 0; i < cth[NEW].num_of_docs; i++)
    {
      fread (&data, sizeof (u_long), 1, idx[NEW]);
      data += offset;
      fwrite (&data, sizeof (u_long), 1, idx[MERGE]);
    }
  /* write last u_long in idx[MERGE] (= length of file) */
  fread (&data, sizeof (u_long), 1, idx[NEW]);
  data += offset;
  fwrite (&data, sizeof (u_long), 1, idx[MERGE]);

/******* update .text *******/
  /* simply cat's the files together, except for the headers 
   * and magic numbers, of course
   */
  fseek (text[OLD], 0L, 2);
  while (!feof (text[NEW]))
    {
      fread (&c, sizeof (c), 1, text[NEW]);
      if (!feof (text[NEW]))
	fwrite (&c, sizeof (c), 1, text[OLD]);
    }

  return OK;
}



/*=======================================================================
 * done_merge_text(): close files.
 *=======================================================================*/
int 
done_merge_text (void)
{
  fclose (idx[MERGE]);
  fclose (text[OLD]);
  fclose (idx[OLD]);
  fclose (text[NEW]);
  fclose (idx[NEW]);

  fprintf (stderr, "mg_text_merge: %ld documents added to %s\n",
	   cth[NEW].num_of_docs, merge_name);

  return OK;
}


/*=======================================================================
 * usage()
 *=======================================================================*/
void 
usage (char *progname)
{
  fprintf (stderr, "usage:  %s -f collection_name\n", progname);
  exit (1);
}


/*=======================================================================
 * main()
 *=======================================================================*/
int 
main (int argc, char *argv[])
{
  char *progname;
  ProgTime start;
  int ch;			/* for command line processing */

  progname = argv[0];
  msg_prefix = argv[0];
  merge_name[0] = '\0';

  while ((ch = getopt (argc, argv, "f:d:h")) != -1)
    switch (ch)
      {
      case 'f':
	strcpy (merge_name, optarg);
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'h':
      case '?':
      default:
	usage (progname);
      }

  if (merge_name[0] == '\0')
    usage (progname);
  strcpy (old_name, merge_name);
  strcat (old_name, ".old");
  strcpy (new_name, merge_name);
  strcat (new_name, ".new");

  GetTime (&start);
  init_merge_text ();
  process_merge_text ();
  done_merge_text ();
  Message ("%s\n", ElapsedTime (&start, NULL));
  exit (0);
}
