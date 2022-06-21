#include "words.h"


/* Takes the place of the old INAWORD macro. It determines
   whether a given place in a UTF-8 encoded Unicode string
   is part of a word. */
int inaword (const u_char *here, const u_char *end) {
  unsigned short c;
  if (parse_utf8_char(here, end, &c) > 0) return is_unicode_letdig (c);
  return 0;
}

/* Return a the UTF-8 encoded Unicode string with begining
   unicode spaces skipped. */
u_char *skipspace(u_char *here, u_char *end)
{
  unsigned short c;	
  int length;
  while(here != end) {
    length = parse_utf8_char(here, end, &c);
    if (length == 0 || !is_unicode_space(c)) break;
    here += length;
  }
  return here;
}

/* It determines whether a given place in a UTF-8 encoded 
   Unicode string is a unicode space. */
int isaspace (const u_char *here, const u_char *end)
{
  unsigned short c;
  if (parse_utf8_char(here, end, &c) > 0) return is_unicode_space(c);
  return 0;
}
