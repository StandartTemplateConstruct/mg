/**************************************************************************
 *
 * utils.c -- Functions which are common utilities for the image programs
 * Copyright (C) 1994  Stuart Inglis
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
 * $Id: utils.c,v 1.1.1.1 1994/08/11 03:26:14 tes Exp $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "utils.h"


int V = 0;


int 
isEOF (FILE * fp)
{
  int ch;

  ch = getc (fp);
  ungetc (ch, fp);
  if (ch == EOF)
    return 1;
  return 0;
}


int 
getmagicno_short (FILE * fp)
{
  int ch1, ch2;
  ch1 = getc (fp);
  if (ch1 == EOF)
    {
      ungetc (ch1, fp);
      return 0;
    }
  ch2 = getc (fp);
  if (ch2 == EOF)
    {
      ungetc (ch2, fp);
      return 0;
    }
  ungetc (ch2, fp);
  ungetc (ch1, fp);
  return (ch1 << 8) | ch2;
}

int 
getmagicno_short_pop (FILE * fp)
{
  int ch1, ch2;
  ch1 = getc (fp);
  if (ch1 == EOF)
    return 0;
  ch2 = getc (fp);
  if (ch2 == EOF)
    return 0;
  return (ch1 << 8) | ch2;
}

int 
getmagicno_byte (FILE * fp)
{
  int ch1;
  ch1 = getc (fp);
  if (ch1 == EOF)
    {
      ungetc (ch1, fp);
      return 0;
    }
  ungetc (ch1, fp);
  return (ch1);
}


int 
getmagicno_long (FILE * fp)
{
  int ch1, ch2, ch3, ch4;
  ch1 = getc (fp);
  if (ch1 == EOF)
    {
      ungetc (ch1, fp);
      return 0;
    }
  ch2 = getc (fp);
  if (ch2 == EOF)
    {
      ungetc (ch2, fp);
      return 0;
    }
  ch3 = getc (fp);
  if (ch3 == EOF)
    {
      ungetc (ch3, fp);
      return 0;
    }
  ch4 = getc (fp);
  if (ch4 == EOF)
    {
      ungetc (ch4, fp);
      return 0;
    }
  ungetc (ch4, fp);
  ungetc (ch3, fp);
  ungetc (ch2, fp);
  ungetc (ch1, fp);

  return (ch1 << 24) | (ch2 << 16) | (ch3 << 8) | (ch4);
}




void 
error_msg (char *prog, char *message, char *extra)
{
  fprintf (stderr, "%s: %s %s\n", prog, message, extra);
  exit (1);
}

void 
warn (char *prog, char *message, char *extra)
{
  fprintf (stderr, "%s: %s %s\n", prog, message, extra);
}



void 
readline (char str[], FILE * fp)
{
  int i = 0, ch;

  while (((ch = fgetc (fp)) != '\n') && (!feof (fp)))
    str[i++] = ch;
  str[i] = '\0';
}







int 
getint (FILE * fp)
{
  register char ch;
  register unsigned int i = 0;

  do
    {
      ch = getc (fp);
      if (feof (fp))
	return EOF;
    }
  while (ch < '0' || ch > '9');

  do
    {
      i = i * 10 + ch - '0';
      ch = getc (fp);
      if (feof (fp))
	return EOF;
    }
  while (ch >= '0' && ch <= '9');

  return i;
}

/* get_header_int i.e. get an integer from a PNM header ; basically  
   the same as
   above except for comment checking ; above kept because of lower  
   overhead */

unsigned int 
gethint (FILE * fp)
{
  register char ch, prev = '\0';
  register unsigned int i = 0;

  do
    {
      ch = getc (fp);
      if ((ch == '#') && (prev == '\n'))
	while (((ch = getc (fp)) != '\n') && !(feof (fp)));
      prev = ch;
      if (feof (fp))
	return EOF;
    }
  while (ch < '0' || ch > '9');

  do
    {
      i = i * 10 + ch - '0';
      ch = getc (fp);
      if (feof (fp))
	return EOF;
    }
  while (ch >= '0' && ch <= '9');

  return i;
}




void 
magic_write (FILE * fp, u_long magic_num)
{
  if (fwrite (&magic_num, sizeof (magic_num), 1, fp) != 1)
    error_msg ("magic num", "Couldn't write magic number.", "");
}


void 
magic_check (FILE * fp, u_long magic_num)
{
  u_long magic;
  if (fread (&magic, sizeof (magic), 1, fp) != 1 || magic != magic_num)
    error_msg ("magic num", "Incorrect magic number.", "");
}


u_long 
magic_read (FILE * fp)
{
  u_long magic;
  if (fread (&magic, sizeof (magic), 1, fp) != 1)
    error_msg ("magic num", "Couldn't read magic number.", "");
  return magic;
}




int 
isinteger (char s[])
{
  int i = 0;

  for (i = 0; s[i] != '\0'; i++)
    if (!isdigit (s[i]))
      return 0;
  return 1;
}
