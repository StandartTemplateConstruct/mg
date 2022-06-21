/**************************************************************************
 *
 * simplefrenchstem.c -- a simple french stemmer
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
 **************************************************************************/

#include "simplefrenchstem.h"



/* =========================================================================
 * Function: simplefrenchstem
 * Description: a simple french stemmer
 * Input: a word string with the length in the first byte
 * Output: the stemmed word
 * ========================================================================= */

void simplefrenchstem (unsigned char *word) {      
  unsigned short out[256]; /* temp space */
  unsigned short *wordstart; /* points to first letter of word */
  int last;                  /* last points to the last character */

  /* decode */
  utf8_word_to_unicode (word, out, 255);
  wordstart = out + 1;
  last = out[0]-1;
  

  if (last > 4) {
    if (wordstart[last]=='x') {
      if (wordstart[last-1]=='u' && wordstart[last-2]=='a') {
	wordstart[last-1]='l';
      }
      last--;

    } else {
      if (last>=0 && wordstart[last]=='s') last--;
      if (last>=0 && wordstart[last]=='r') last--;
      if (last>=0 && wordstart[last]=='e') last--;

      /* letter with accent e + ' -- there are two possible encodings  */
      if (last>=0 && wordstart[last]==0xe9) { 
	last--;
      } else if (last>=1 && wordstart[last-1]=='e' && wordstart[last]==0x301) {
	last -= 2; 
      }

      if (last >= 1 && wordstart[last]==wordstart[last-1]) last--;
    }  /* end else */

    out[0] = (unsigned char)(last+1);
  } /* end if (len > 4) */

  /* re-code, make sure the result is not longer than the input */
  unicode_to_utf8_word (out, word, word[0]+1);
}
