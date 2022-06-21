#include "sysfuncs.h"

#include "filestats.h"
#include "netorder.h"
#include "memlib.h"
#include "messages.h"
#include "huffman.h"
#include "bitio_m_mem.h"
#include "bitio_m.h"
#include "bitio_stdio.h"
#include "huffman_stdio.h"

#include "mg_files.h"
#include "mg.h"
#include "backend.h"  /* compressed_dict struct */
#include "text_get.h"
#include "words.h"

#define EOD         '\002'
#define EOP         '\003'

/* These are defined in mgquery, and they need to be here for backend.o */
/* Messy I know but this was meant to be a quick hack :-) */
#if defined(PARADOCNUM) || defined(NZDL)
int *Paragraph = NULL;
int Documents = 0;
#endif

int
main (int argc, char **argv)
{
  FILE *text, *text_idx, *para;
  File *text_comp_dict = NULL, *text_aux_dict = NULL, *text_fast_comp_dict = NULL;

  int ch;
  unsigned long start, finish, len;
  u_long total_chars = 0;
  u_char *c_buffer = 0;
  int buf_len = -1;
  u_char *uc_buffer = 0;
  int ULen;
  compression_dict *cd = NULL;
  compressed_text_header cth;
  u_long doc;
  int count;
  char *filename = "";
  char path[512];
  opterr = 0;
  msg_prefix = argv[0];

  while ((ch = getopt (argc, argv, "f:d:h")) != -1)
    switch (ch)
      {
      case 'f':		/* input file */
	filename = optarg;
	break;
      case 'd':
	set_basepath (optarg);
	break;
      case 'h':
      case '?':
	fprintf (stderr, "usage: %s [-h] [-d directory] -f name\n",
		 argv[0]);
	exit (1);
      }

  /* Open files */
  text = open_file (filename, TEXT_SUFFIX, "rb", MAGIC_TEXT, MG_ABORT);
  fread ((char *)&cth, sizeof (cth), 1, text);

  text_idx = open_file (filename, TEXT_IDX_SUFFIX, "rb", MAGIC_TEXI, MG_ABORT);
  fread ((char *)&cth, sizeof (cth), 1, text_idx);

  NTOHUL(cth.num_of_docs);
  NTOHD(cth.num_of_bytes); /* [RJM 07/97: 4G limit] */
  NTOHUL(cth.num_of_words);
  NTOHUL(cth.length_of_longest_doc);
  NTOHD(cth.ratio);

  para = open_file (filename, INVF_PARAGRAPH_SUFFIX, "rb", MAGIC_PARAGRAPH, MG_CONTINUE);

  sprintf (path, FILE_NAME_FORMAT, get_basepath(), filename, TEXT_DICT_FAST_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
  if (!(text_fast_comp_dict = Fopen (path, "rb", MAGIC_FAST_DICT)))
    {
      sprintf (path, FILE_NAME_FORMAT, get_basepath(), filename, TEXT_DICT_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
      text_comp_dict = Fopen (path, "rb", MAGIC_DICT);
      if (!text_comp_dict)
	FatalError (1, "Could not open compressed dictionary");
      sprintf (path, FILE_NAME_FORMAT, get_basepath(), filename, TEXT_DICT_AUX_SUFFIX);  /* [RPAP - Feb 97: WIN32 Port] */
      text_aux_dict = Fopen (path, "rb", MAGIC_AUX_DICT);
    }

  cd = LoadCompDict (text_comp_dict, text_aux_dict, text_fast_comp_dict);


  /* Uncompress text */

  doc = 1;
  fread ((char *)&start, sizeof (start), 1, text_idx);
  if (para)
    fread ((char *)&count, sizeof (count), 1, para);
  else
    count = 1;

  for (;;)
    {
      fread ((char *) &finish, sizeof (finish), 1, text_idx);
      len = finish - start;
      
      if ((int) len > buf_len)
	{
	  if (c_buffer)
	    {
	      Xfree (c_buffer);
	      Xfree (uc_buffer);
	    }
	  if (!(c_buffer = Xmalloc (len)))
	    FatalError (1, "Cannot allocate memory for compressed buffer");
	  if (!(uc_buffer = Xmalloc ((int) (cth.ratio * 1.01 * len) + 100)))
	    FatalError (1, "Cannot allocate memory for uncompressed buffer");
	  buf_len = len;
	}

      fread (c_buffer, 1, len, text);
      DecodeText (cd, c_buffer, len, uc_buffer, &ULen);
      fwrite (uc_buffer, ULen, sizeof (u_char), stdout);
      fflush (stdout);

      total_chars += ULen;

      if (++doc > cth.num_of_docs)
	{
	  if (para)
	    fputc (EOP, stdout);
	  fputc (EOD, stdout);
	  fflush (stdout);
	  break;
	}

      if (!--count)
	{
	  /* End of document */
	  fputc (EOD, stdout);
	  if (para)
	    fread ((char *)&count, sizeof (count), 1, para);
	  else
	    count = 1;
	}
      else
	fputc (EOP, stdout);
      fflush (stdout);
      start = finish;
    }

  /* Close files */

  fclose (text);
  fclose (text_idx);
  if (para)
    fclose (para);

#if 0
  printf ("\n#Total chars output = %lu\n", total_chars);
#endif

  return 0;
}
