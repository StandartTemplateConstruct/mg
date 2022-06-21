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
 * $Id: words.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/


#include "sysfuncs.h"
#include "unitool.h"

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

/*#define MAXNUMERIC	4*/

	/* Maximum number of numeric characters permitted in a word.
	   This avoids long sequences of numbers creating just one
	   word occurrence for each number. At most 10,000 all numeric
	   words will be permitted. */

/* [RPAP - Jan 97: Stem Index Change] */
#define MAXPARAMLEN     20
        /* Maximum number of bytes to read for a parameter value for a
	   term in a query. */
#define WEIGHTPARAM     '/'
#define STEMPARAM       '#'

/* [RJM 07/97: Ranked Required Terms] */
#define MUSTMATCHPARAM     '+'


#define PESINAWORD(c)      (isalnum(c) || ((c) >= 0x80 && (c) <= 0xff))
        /* The definition of what characters are permitted in a word.
	   This macro is pessimistic, you cannot tell from a particular
	   byte above 0x80 whether it is a character or not. This function
	   is needed by various functions relating to huffman coding
	   where frequency counts need to be primed, it should not be
	   used in parsing the UTF-8 encoded input. */

int inaword (const u_char *here, const u_char *end);
        /* Takes the place of the old INAWORD macro. It determines
	   whether a given place in a UTF-8 encoded Unicode string
	   is part of a word. */

int isaspace (const u_char *here, const u_char *end);
        /* It determines whether a given place in a UTF-8 encoded 
	   Unicode string is a unicode space. */
 
u_char *skipspace(u_char *here, u_char *end);
        /* Return a the UTF-8 encoded Unicode string with beginning 
	   unicode spaces skipped. */
 

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
#define PARSE_WORD(Word, s_in, end)                                \
  do {                                                             \
    register int charlength = 0;                                   \
    register u_char *wptr = (Word)+1;                              \
    register int length = 0;                                       \
    register int numeric = 0;                                      \
    unsigned short c;                                              \
    register int maxnumeric = IntEnv (GetEnv ("maxnumeric"), 4);   \
                                                                   \
    charlength = parse_utf8_char((s_in),(end),&c);                 \
                                                                   \
    while (length+charlength <= MAXWORDLEN && charlength > 0 &&    \
	   (is_unicode_letter(c) || (is_unicode_digit(c) &&        \
				     ++numeric <= maxnumeric))) {  \
      while (charlength-- > 0) {                                   \
        *wptr++ = *(s_in)++; ++length;                             \
      }                                                            \
      charlength = parse_utf8_char((s_in),(end),&c);               \
    }                                                              \
    *(Word) = length;                                              \
  }while(0)

  /*  
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
	      ++length;                                              \
	      c = *++(s_in);                                         \
	    }                                                        \
	  *(Word) = length;                                          \
  }while(0)
  */


/* =========================================================================
 * Macro: PARSE_NON_WORD
 * Description: 
 *      Extract a non-word out for storing compressed text
 * Input: as above
 * Output: as above
 * ========================================================================= */
#define PARSE_NON_WORD(Word, s_in, end)                            \
  do {                                                             \
    register int charlength = 0;                                   \
    register u_char *wptr = (Word)+1;                              \
    register int length = 0;                                       \
    unsigned short c;                                              \
                                                                   \
    charlength = parse_utf8_char((s_in),(end),&c);                 \
                                                                   \
    while (length+charlength <= MAXWORDLEN && charlength > 0 &&    \
	   !is_unicode_letdig(c)) {                                \
      while (charlength-- > 0) {                                   \
        *wptr++ = *(s_in)++; ++length;                             \
      }                                                            \
      charlength = parse_utf8_char((s_in),(end),&c);               \
    }                                                              \
    *(Word) = length;                                              \
  }while(0)

    /*
#define PARSE_NON_WORD(Word, s_in, end)                            \
  do {                                                             \
	  register u_char  *wptr = (Word)+1;                       \
	  register int    length = 0;                              \
	  register int    c = *(s_in);                             \
                                                                   \
	  while( length < MAXWORDLEN && !INAWORD(c) && (s_in)<=(end) ) \
	    {                                                      \
	      *wptr++ = c;                                         \
	      ++length;                                            \
	      c = *++(s_in);                                       \
	    }                                                      \
	  *(Word) = length;                                        \
  }while(0)
    */


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
#define PARSE_STEM_WORD(Word, s_in, end)                           \
  do {                                                             \
    register int charlength = 0;                                   \
    register u_char *wptr = (Word)+1;                              \
    register int length = 0;                                       \
    register int numeric = 0;                                      \
    unsigned short c;                                              \
    register int maxnumeric = IntEnv (GetEnv ("maxnumeric"), 4);   \
                                                                   \
    charlength = parse_utf8_char((s_in),(end),&c);                 \
                                                                   \
    while (length+charlength <= MAXSTEMLEN && charlength > 0 &&    \
	   (is_unicode_letter(c) || (is_unicode_digit(c) &&        \
				     ++numeric <= maxnumeric))) {  \
      while (charlength-- > 0) {                                   \
        *wptr++ = *(s_in)++; ++length;                             \
      }                                                            \
      charlength = parse_utf8_char((s_in),(end),&c);               \
    }                                                              \
    *(Word) = length;                                              \
  }while(0)
    /*
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
	  ++length;                                           \
	  c = *++(s_in);                                      \
	}                                                     \
      *(Word) = length;                                       \
    }while(0)
    */


/* =========================================================================
 * Macro: PARSE_NON_STEM_WORD 
 * Description: 
 *      Eat up non-word. Do not store non-word.
 *      It is not needed in index only in text !
 *      
 * Input: as above but no Word needed
 * Output: as above
 * ========================================================================= */
#define PARSE_NON_STEM_WORD(s_in, end)                             \
  do {                                                             \
    register int charlength = 0;                                   \
    unsigned short c;                                              \
                                                                   \
    charlength = parse_utf8_char((s_in),(end),&c);                 \
                                                                   \
    while (charlength > 0 && !is_unicode_letdig(c)) {              \
      (s_in) += charlength;                                        \
      charlength = parse_utf8_char((s_in),(end),&c);               \
    }                                                              \
  }while(0)

    /*
#define PARSE_NON_STEM_WORD(s_in, end)           \
  do                                             \
    {                                            \
      while (!INAWORD(*(s_in)) && (s_in)<=(end)) \
	(s_in)++;                                \
    }while(0)
    */


/* =========================================================================
 * Macro: PARSE_NON_STEM_WORD_OR_SGML_TAG 
 * Description: 
 *      Like PARSE_NON_STEM_WORD but also eats up SGML tags
 * Input: as above
 * Output: as above
 * ========================================================================= */
#define PARSE_NON_STEM_WORD_OR_SGML_TAG(s_in, end)                 \
  do {                                                             \
    register int charlength = 0;                                   \
    unsigned short c;                                              \
                                                                   \
    charlength = parse_utf8_char((s_in),(end),&c);                 \
                                                                   \
    while (charlength > 0 && !is_unicode_letdig(c)) {              \
      if (c == '<') {                                              \
	while (charlength > 0 && c != '>') {                       \
	  (s_in) += charlength;                                    \
	  charlength = parse_utf8_char((s_in),(end),&c);           \
	}                                                          \
      }                                                            \
      (s_in) += charlength;                                        \
      charlength = parse_utf8_char((s_in),(end),&c);               \
    }                                                              \
  }while(0)

    /*
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
    */


/* =========================================================================
 * Macro: PARSE_OPT_TERM_PARAM     [RPAP - Jan 97: Stem Index Change]
 * Description: 
 *      Extracts out optional paramater for query term.
 *      Needed only in parsing the query line !
 *
 *      Note: that this function has not been converted to use UTF-8
 *            as it should still work as it is (only uses ascii
 *            characters)
 *      
 * Input: as above but no Word needed
 * Output: as above
 * ========================================================================= */
#define PARSE_OPT_TERM_PARAM(Param, type, s_in, end)                       \
  do                                                                       \
    {                                                                      \
	  register u_char  *wptr = (Param);                                \
	  register int    length = 0;                                      \
	  register int    c = *(s_in);                                     \
                                                                           \
          if (c == WEIGHTPARAM || c == STEMPARAM)                          \
	    {                                                              \
	      type = c;                                                    \
	      c = *++(s_in);                                               \
	      while( length < MAXPARAMLEN && isdigit(c) && (s_in)<=(end))  \
		{                                                          \
	           *wptr++ = c;                                            \
	           ++length;                                               \
	           c = *++(s_in);                                          \
	        }                                                          \
	      *wptr = '\0';                                                \
              for (; isdigit(c) && (s_in)<=(end); c = *++(s_in))           \
                ;                                                          \
            }							           \
    }while(0)

/* =========================================================================
 * Macro: PARSE_RANKED_NON_STEM_WORD    [RJM 07/97: Ranked Required Terms]
 * Description: 
 *      Eat up non-word. Do not store non-word.
 *      If come across a match requirement store it in require_match
 *      It is not needed in index only in text !
 *      
 * Input: as above
 * Output: the requirement mode for the next term. -1=must not match,
 *      0=optional match, 1=must match
 * ========================================================================= */
#define PARSE_RANKED_NON_STEM_WORD(require_match, s_in, end)       \
  do {                                                             \
    register int charlength = 0;                                   \
    unsigned short c;                                              \
    (require_match) = 0;                                           \
                                                                   \
    charlength = parse_utf8_char((s_in),(end),&c);                 \
                                                                   \
    while (charlength > 0 && !is_unicode_letdig(c)) {              \
      if (c == MUSTMATCHPARAM) (require_match) = 1;                \
      (s_in) += charlength;                                        \
      charlength = parse_utf8_char((s_in),(end),&c);               \
    }                                                              \
  }while(0)

    /*
#define PARSE_RANKED_NON_STEM_WORD(require_match, s_in, end)         \
  do {                                            \
    (require_match) = 0;                          \
    while (!INAWORD(*(s_in)) && (s_in)<=(end)) {  \
      if (*(s_in) == MUSTMATCHPARAM) {            \
        (require_match) = 1;                      \
      }                                           \
      (s_in)++;                                   \
    }                                             \
  } while (0)
    */
