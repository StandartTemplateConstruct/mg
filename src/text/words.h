/**************************************************************************
 *
 * words.h -- Macros for parsing out words from the source text
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
 * $Id: words.h,v 1.3 1994/10/20 03:57:13 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"

/*
 * This has been cleaned up by Tim Shimmin. 
 */

/*
 * ---NOTE---
 *
 * "WORD" refers to a word in the compressed text.
 * "STEM" or "STEM_WORD" refers to a word for indexing on
 *
 */

#define MAXWORDLEN	15
	/* Maximum length in bytes of any word or non-word. Note that
	   variations to MAXWORDLEN may have dramatic effects on the rest
	   of the program, as the length and the prefix match are packed
	   together into a four bit nibble, and there is not check that
	   this is possible, i.e., leave MAXWORDLEN alone... */
#define MAXSTEMLEN	255
	/* Maximum length in bytes of any stem. Note that
	   variations to MAXSTEMLEN may have dramatic effects on the rest
	   of the program, , i.e., leave MAXSTEMLEN alone... */
#define MAXNUMERIC	4
	/* Maximum number of numeric characters permitted in a word.
	   This avoids long sequences of numbers creating just one
	   word occurrence for each number. At most 10,000 all numeric
	   words will be permitted. */

#define	INAWORD(c)	(isascii(c) && isalnum(c))
	/* The definition of what characters are permitted in a word
	 */

#define INNUMBER(c)     (isdigit(c)?1:0)

/* =========================================================================
 * Macro: PARSE_WORD
 * Description: 
 *      Extract a word out for compressing text
 * Input: 
 *      s_in = string start in buffer
 *      end = string end in buffer
 * Output: 
 *      Word = extracted word with length in 1st byte
 *      s_in = ptr to next character in buffer yet to be processed
 * ========================================================================= */
#define PARSE_WORD(Word, s_in, end)                                  \
  do {                                                               \
	  register u_char  *wptr = (Word)+1;                         \
	  register int    length = 0;                                \
	  register int    c = *(s_in);                               \
	  register int	numeric = 0;                                 \
                                                                     \
	  while( length < MAXWORDLEN && INAWORD(c) && (s_in)<=(end)) \
	    {                                                        \
	      if ((numeric += INNUMBER(c)) > MAXNUMERIC)             \
		break;                                               \
	      *wptr++ = c;                                           \
	      length++;                                              \
	      c = *++(s_in);                                         \
	    }                                                        \
	  *(Word) = length;                                          \
  }while(0)

/* =========================================================================
 * Macro: PARSE_NON_WORD
 * Description: 
 *      Extract a non-word out for storing compressed text
 * Input: as above
 * Output: as above
 * ========================================================================= */
#define PARSE_NON_WORD(Word, s_in, end)                            \
  do {                                                             \
	  register u_char  *wptr = (Word)+1;                       \
	  register int    length = 0;                              \
	  register int    c = *(s_in);                             \
                                                                   \
	  while( length < MAXWORDLEN && !INAWORD(c) && (s_in)<=(end) ) \
	    {                                                      \
	      *wptr++ = c;                                         \
	      length++;                                            \
	      c = *++(s_in);                                       \
	    }                                                      \
	  *(Word) = length;                                        \
  }while(0)

/* =========================================================================
 * Macro: PARSE_STEM_WORD 
 * Description: 
 *      Extracts out Word.      
 * Input: 
 *      s_in points to 1st letter in buffer to test
 *      end points to last letter in buffer
 * Output: 
 *      s_in is modified to move to next word
 *      Returns Word filled in with length in 1st byte.
 * ========================================================================= */
#define PARSE_STEM_WORD(Word, s_in, end)                      \
  do                                                          \
    {                                                         \
      register u_char  *wptr = (Word)+1;                      \
      register int    length = 0;                             \
      register int    c = *(s_in);                            \
      register int    numeric = 0;                            \
                                                              \
      while ( length < MAXSTEMLEN && INAWORD(c) && (s_in)<=(end)) \
        {                                                     \
 	  if ((numeric += INNUMBER(c)) > MAXNUMERIC)          \
	    break;                                            \
	  *wptr++ = c;                                        \
	  length++;                                           \
	  c = *++(s_in);                                      \
	}                                                     \
      *(Word) = length;                                       \
    }while(0)

/* =========================================================================
 * Macro: PARSE_NON_STEM_WORD 
 * Description: 
 *      Eat up non-word. Do not store non-word.
 *      It is not needed in index only in text !
 *      
 * Input: as above but no Word needed
 * Output: as above
 * ========================================================================= */
#define PARSE_NON_STEM_WORD(s_in, end)           \
  do                                             \
    {                                            \
      while (!INAWORD(*(s_in)) && (s_in)<=(end)) \
	(s_in)++;                                \
    }while(0)

/* =========================================================================
 * Macro: PARSE_NON_STEM_WORD_OR_SGML_TAG 
 * Description: 
 *      Like PARSE_NON_STEM_WORD but also eats up SGML tags
 * Input: as above
 * Output: as above
 * ========================================================================= */
#define PARSE_NON_STEM_WORD_OR_SGML_TAG(s_in, end) \
  do                                               \
    {                                              \
      register int    c = *(s_in);                 \
                                                   \
      while (!INAWORD(c) && (s_in)<=(end))         \
        {                                          \
	  if (c == '<')                            \
            {                                      \
	      while (c != '>' && (s_in)<=(end))    \
		c = *++(s_in);                     \
            }                                      \
	  if ((s_in)<=(end))                       \
	    c = *++(s_in);                         \
	}                                          \
    }while(0)
