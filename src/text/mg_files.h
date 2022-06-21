/**************************************************************************
 *
 * mg_files.h -- Routines for handling files for the auxillary programs
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
 * $Id: mg_files.h,v 1.2 1994/09/20 04:41:48 tes Exp $
 *
 **************************************************************************/

#ifndef MG_FILES_H
#define MG_FILES_H

#include "sysfuncs.h"

/* Magic numbers for the different types of files */

#define GEN_MAGIC(a,b,c,d) ((unsigned long)(((a)<<24) + ((b)<<16) + \
					    ((c)<<8) + (d)))

#define MAGIC_XXXX		GEN_MAGIC('M','G', 0 , 0)
#define MAGIC_STATS_DICT	GEN_MAGIC('M','G','S','D')
#define MAGIC_AUX_DICT		GEN_MAGIC('M','G','A','D')
#define MAGIC_FAST_DICT		GEN_MAGIC('M','G','F','D')
#define MAGIC_DICT		GEN_MAGIC('M','G','D', 0 )
#define MAGIC_STEM_BUILD	GEN_MAGIC('M','G','S', 0 )
#define MAGIC_HASH		GEN_MAGIC('M','G','H', 0 )
#define MAGIC_STEM		GEN_MAGIC('M','G','s', 0 )
#define MAGIC_CHUNK		GEN_MAGIC('M','G','C', 0 )
#define MAGIC_CHUNK_TRANS	GEN_MAGIC('M','G','c', 0 )
#define MAGIC_TEXT		GEN_MAGIC('M','G','T', 0 )
#define MAGIC_TEXI		GEN_MAGIC('M','G','t', 0 )
#define MAGIC_TEXI_WGT		GEN_MAGIC('M','G','t','W')
#define MAGIC_INVF		GEN_MAGIC('M','G','I', 0 )
#define MAGIC_INVI		GEN_MAGIC('M','G','i', 0 )
#define MAGIC_WGHT		GEN_MAGIC('M','G','W', 0 )
#define MAGIC_WGHT_APPROX	GEN_MAGIC('M','G','w', 0 )
#define MAGIC_PARAGRAPH		GEN_MAGIC('M','G','P', 0 )

#define IS_MAGIC(a) ((((u_long)(a)) & 0xffff0000) == MAGIC_XXXX)


/* err_mode values for open_file and create_file */
#define MG_ABORT 0
#define MG_MESSAGE 1
#define MG_CONTINUE 2





/* File suffixes */


/* The compression dictionary built by txt.pass1 */
#ifdef SHORT_SUFFIX
# define TEXT_STATS_DICT_SUFFIX	 ".tsd"
#else
# define TEXT_STATS_DICT_SUFFIX	 ".text.stats"
#endif

/* The compression dictionary built by text.pass1 and comp_dict.process */
#ifdef SHORT_SUFFIX
# define TEXT_DICT_SUFFIX	 ".td"
#else
# define TEXT_DICT_SUFFIX	 ".text.dict"
#endif

/* The compression dictionary built by mg_make_fast_dict */
#ifdef SHORT_SUFFIX
# define TEXT_DICT_FAST_SUFFIX	 ".tdf"
#else
# define TEXT_DICT_FAST_SUFFIX	 ".text.dict.fast"
#endif

/* The auxilary dictionary built by text.pass2 */
#ifdef SHORT_SUFFIX
# define TEXT_DICT_AUX_SUFFIX	 ".tda"
#else
# define TEXT_DICT_AUX_SUFFIX	 ".text.dict.aux"
#endif

/* The compressed text build by text.pass2 */
#ifdef SHORT_SUFFIX
# define TEXT_SUFFIX		 ".t"
#else
# define TEXT_SUFFIX		 ".text"
#endif

/* The combined compressed text index and document weight file */
#ifdef SHORT_SUFFIX
# define TEXT_IDX_WGT_SUFFIX	 ".tiw"
#else
# define TEXT_IDX_WGT_SUFFIX	 ".text.idx.wgt"
#endif

/* The compressed text index file */
#ifdef SHORT_SUFFIX
# define TEXT_IDX_SUFFIX	 	".ti"
#else
# define TEXT_IDX_SUFFIX	 	".text.idx"
#endif

/* The dictionary of stemmed words build by invf.pass1 and ivf.pass1 */
#ifdef SHORT_SUFFIX
# define INVF_DICT_SUFFIX         ".id"
#else
# define INVF_DICT_SUFFIX         ".invf.dict"
#endif

/* The dictionary of stemmed words build by stem.process */
#ifdef SHORT_SUFFIX
# define INVF_DICT_BLOCKED_SUFFIX ".invf.dict.blocked"
#else
# define INVF_DICT_BLOCKED_SUFFIX ".invf.dict.blocked"
#endif

/* The exact document weights file build by make.weights, invf.pass2,
   or ivf.pass2 */
#ifdef SHORT_SUFFIX
# define WEIGHTS_SUFFIX           ".w"
#else
# define WEIGHTS_SUFFIX           ".weight"
#endif

/* The approximate weights file built by make.weights */
#ifdef SHORT_SUFFIX
# define APPROX_WEIGHTS_SUFFIX    ".wa"
#else
# define APPROX_WEIGHTS_SUFFIX    ".weight.approx"
#endif

/* The inverted file build by invf.pass2 or ivf.pass2 */
#ifdef SHORT_SUFFIX
# define INVF_SUFFIX              ".i"
#else
# define INVF_SUFFIX              ".invf"
#endif

/* The inverted file index build by invf.pass2 or ivf.pass2 */
#ifdef SHORT_SUFFIX
# define INVF_IDX_SUFFIX          ".ii"
#else
# define INVF_IDX_SUFFIX          ".invf.idx"
#endif

/* The inverted file chunk descriptor built by ivf.pass1 */
#ifdef SHORT_SUFFIX
# define INVF_CHUNK_SUFFIX        ".ic"
#else
# define INVF_CHUNK_SUFFIX        ".invf.chunk"
#endif

/* The word index translation file built by ivf.pass1 */
#ifdef SHORT_SUFFIX
# define INVF_CHUNK_TRANS_SUFFIX  ".ict"
#else
# define INVF_CHUNK_TRANS_SUFFIX  ".invf.chunk.trans"
#endif

/* The hashed stemmed dictionary built by make.perf_hash */
#ifdef SHORT_SUFFIX
# define INVF_DICT_HASH_SUFFIX    ".idh"
#else
# define INVF_DICT_HASH_SUFFIX    ".invf.dict.hash"
#endif

/* The paragraph descriptior file built by invf.pass1 or ivf.pass1 */
#ifdef SHORT_SUFFIX
# define INVF_PARAGRAPH_SUFFIX    ".ip"
#else
# define INVF_PARAGRAPH_SUFFIX    ".invf.paragraph"
#endif

/* The trace file build by mg.builder. */
#ifdef SHORT_SUFFIX
# define TRACE_SUFFIX             ".trc"
#else
# define TRACE_SUFFIX             ".trace"
#endif

/* The compression stats file build by mg.builder. */
#ifdef SHORT_SUFFIX
# define COMPRESSION_STATS_SUFFIX ".cs"
#else
# define COMPRESSION_STATS_SUFFIX ".compression.stats"
#endif






/* This sets the base path for all file operations */
void set_basepath (const char *bp);


/* return the currently defined basepath */
char *get_basepath (void);




/* This generates the name of a file. It places the name in the buffer
   specified or if that is NULL it uses a static buffer. */
char *make_name (const char *name, const char *suffix, char *buffer);







/* This will open the specified file and check its magic number.
   Mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *open_named_file (const char *name, const char *mode,
		       u_long magic_num, int err_mode);




/* This will open the specified file and check its magic number.

   err_mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *open_file (const char *name, const char *suffix, const char *mode,
		 u_long magic_num, int err_mode);





/* This will create the specified file and set its magic number.

   Mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *create_named_file (const char *name, const char *mode,
			 u_long magic_num, int err_mode);



/* This will create the specified file and set its magic number.

   err_mode may take on the following values
   MG_ABORT    : causes an error message to be generated and the
   program aborted if there is an error.
   MG_MESSAGE  : causes a message to be generated and a NULL value to
   be returned if there is an error.
   MG_CONTINUE : causes a NULL value to be returned if there is an error.

   On success if returns the FILE *. On failure it will return a NULL value
   and possibly generate an error message, or it will exit the program with
   an error message.   */
FILE *create_file (const char *name, const char *suffix, const char *mode,
		   u_long magic_num, int err_mode);




#endif
