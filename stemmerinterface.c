#include "lib/lovinstem.h"
#include <stdio.h>
#include <ctype.h>

int main () {
  char word[256];
  char c;
  int len;

  c = fgetc (stdin);
  while (!feof (stdin)) {
    /* output non words */
    while (!feof (stdin) && !isalnum (c)) {
      fputc (c, stdout);
      c = fgetc (stdin);
    }

    /* get the word */
    len = 0;
    while (!feof (stdin) && isalnum (c) && len < 255) {
      len++;
      word[len] = c;
      c = fgetc (stdin);
    }
    word[0] = len;

    /* if we found a word stem and output it */
    if (len > 0) {
      lovinstem (word);
      len = 0;
      while (len < word[0]) {
        len++;
        fputc(word[len], stdout);
      }
    }
  }
}


