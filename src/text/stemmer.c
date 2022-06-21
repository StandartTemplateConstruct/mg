/**************************************************************************
 *
 * stemmer.c -- The stemmer/case folder
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
 * $Id: stemmer.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "stemmer.h"

#include "lovinstem.h"
#include "simplefrenchstem.h"
#include "unitool.h"

/*
   $Log$
   Revision 1.2  2004/06/22 04:17:06  kjdon
   make stemmerdescription a const

   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:23  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.3  1998/12/17 09:12:54  rjmcnab

   Altered mg to process utf-8 encoded Unicode. The main changes
   are in the parsing of the input, the casefolding, and the stemming.

   Revision 1.2  1998/11/25 07:55:51  rjmcnab

   Modified mg to that you can specify the stemmer you want
   to use via a command line option. You specify it to
   mg_passes during the build process. The number of the
   stemmer that you used is stored within the inverted
   dictionary header and the stemmed dictionary header so
   the correct stemmer is used in later stages of building
   and querying.

   Revision 1.1  1998/11/17 09:35:42  rjmcnab
   *** empty log message ***

   * Revision 1.3  1994/10/20  03:57:05  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:42:10  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: stemmer.c 16583 2008-07-29 10:20:36Z davidb $";


#define LOVINSTEMMER        0
#define SIMPLEFRENCHSTEMMER 1


/* decode the utf-8 encoded unicode, casefold and then recode 
 * making sure the final length doesn't exceed the original
 * length */
static void unicode_casefold (u_char *word) {
  unsigned short out[256]; /* temp space */
  int i;
  int len;

  /* decode */
  utf8_word_to_unicode (word, out, 255);
  len = out[0];

  /* casefold and simplify-fold */
  for (i=0; i<len; i++) {
    out[i+1] = unicode_tosimplified(unicode_tolower(out[i+1]));
  }

  /* re-code */
  unicode_to_utf8_word (out, word, word[0]+1);
}


int stemmernumber (const u_char *stemmerdescription) {
  u_char descript[MAX_STEM_DESCRIPTION_LEN];
  int i;

  /* copy and case-fold the description */
  for (i=0; (stemmerdescription[i] != '\0') && 
	 (i < MAX_STEM_DESCRIPTION_LEN-1); i++)
    descript[i] = tolower (stemmerdescription[i]);
  descript[i] = '\0';

  /* map the description to its number */

  if ((strcmp (descript, "0") == 0) ||
      (strcmp (descript, "english") == 0) ||
      (strcmp (descript, "lovin") == 0))
    return LOVINSTEMMER;

  if ((strcmp (descript, "1") == 1) ||
      (strcmp (descript, "french") == 0) ||
      (strcmp (descript, "simplefrench") == 0))
    return SIMPLEFRENCHSTEMMER;

  return -1;
}



/*
 * Method 0 - Do not stem or case fold.
 * Method 1 - Case fold.
 * Method 2 - Stem.
 * Method 3 - Case fold and stem.
 *
 * The stemmer number should be obtained using
 * the stemmernumber function above.
 */
void 
stemmer (int method, int stemmer, u_char *word) {
  if (method & 1) {
    unicode_casefold (word);
  }

  if (method & 2) {
    switch (stemmer) {
    case LOVINSTEMMER: lovinstem (word);
      break;
    case SIMPLEFRENCHSTEMMER: simplefrenchstem (word);
      break;
    }
  }
}
