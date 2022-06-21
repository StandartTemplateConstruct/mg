/**************************************************************************
 *
 * local_strings -- provides some extra string routines
 * Copyright (C) 1994  Authors
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

#include "sysfuncs.h"

#include "local_strings.h"
#include "memlib.h"

/* =========================================================================
 * Function: arg_atoi
 * Description: 
 *      Converts an ascii string to an integer
 *      but allows a scale string immediately following.
 *      i.e. "10Mb" or "20Kb" or "40Megabytes"
 *      There must be no gap between the last digit and the scale.
 *      Only the fist letter is significant.
 *      K = Kilo
 *      M = Mega
 *      G = Giga
 * Input: 
 *      string of form: xxxxxxK or xxxxxxxM or xxxxxxxG
 *      where x is a digit
 * Output: 
 *      a scaled integer
 * ========================================================================= */

#define KILO 	1024
#define MEGA	1024*1024
#define GIGA	1024*1024*1024

int 
arg_atoi (char *str)
{
  char *str_ptr = str;
  char ch = '\0';
  long scale = 1;

  /* find any scale letter immediately after digits */
  ch = *str_ptr;
  while (!isalpha (ch) && (ch != '\0'))
    ch = *str_ptr++;

  /* replace scale letter by end-of-string character */
  *str_ptr = '\0';

  /* set scale appropriately */
  switch (ch)
    {
    case 'K':
      scale = KILO;
      break;
    case 'M':
      scale = MEGA;
      break;
    case 'G':
      scale = GIGA;
      break;
    default:
      scale = 1;
      break;
    }

  return (atoi (str) * scale);

}

/* ===============================================================================
 * Function: compare
 * Description: Compare two strings. 
 * Input: Two binary strings, first byte of string indicates length. 
 * Output: Like standard strcmp.
 *      = 0 if s1 = s2
 *      < 0 if s1 < s2
 *      > 0 if s1 > s2
 * =============================================================================== */
int 
compare (u_char * s1, u_char * s2)
{
  int l1 = *s1, l2 = *s2;
  int l = (l1 < l2) ? l1 : l2;

  while (l--)
    if (*++s1 != *++s2)
      return (*s1 - *s2);
  return (l1 - l2);

}

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static unsigned char casecharmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

/* ===============================================================================
 * Function: casecompare   [RPAP - Jan 97: Stem Index Change]
 * Description: Compare two strings in dictionary order
 * Input: Two binary strings, first byte of string indicates length. 
 * Output:
 *      = 0 if s1 = s2
 *      < 0 if s1 < s2
 *      > 0 if s1 > s2
 * =============================================================================== */
int 
casecompare (u_char * s1, u_char * s2)
{
  int l1 = *s1, l2 = *s2;
  int l = (l1 < l2) ? l1 : l2;
  int pos = 0;
  register int diff = 0;

  while (l--)
    if ((diff = casecharmap[*++s1] - casecharmap[*++s2]) != 0)
      return (diff);
    else if ((diff = *s1 - *s2) != 0)
      if (!pos)
	pos = diff;

  return ((l1 - l2) ? (l1 - l2) : (pos));

}

/* =========================================================================
 * Function: copy_string
 * Description: 
 *      Copies a string with the len in 1st byte
 * Input: 
 * Output: 
 * ========================================================================= */

u_char *
copy_string (u_char * src_str)
{
  u_char *copy = Xmalloc (src_str[0] + 1);

  if (!copy)
    return NULL;

  bcopy ((char *) src_str, (char *) (copy), src_str[0] + 1);
  return copy;

}


/* ===============================================================================
 * Function: prefixlen
 * Description: Returns length of common prefix string between s1 and s2.
 * Input: two binary strings
 * Output: prefix length
 * =============================================================================== */
int 
prefixlen (u_char * s1, u_char * s2)
{
  int l = (*s1 < *s2) ? *s1 : *s2;
  int i = 0;

  while (i < l && *++s1 == *++s2)
    i++;
  return (i);
}


/* ===============================================================================
 * Function: char2str
 * Description: 
 *      Ensures that non-printable characters are converted to a string
 *      which is printable i.e. it indentifies the character.
 *      e.g. carriage-return = "\n".
 * Input: character (may be non-printable)
 * Output: pointer to static string
 * =============================================================================== */

char *
char2str (u_char c)
{
  static char buf[10];
  switch (c)
    {
    case '\n':
      sprintf (buf, "\\n");
      break;
    case '\b':
      sprintf (buf, "\\b");
      break;
    case '\f':
      sprintf (buf, "\\f");
      break;
    case '\t':
      sprintf (buf, "\\t");
      break;
    default:
	if (c >= ' ' && c <= 126)
	switch (c)
	  {
	  case '\\':
	    sprintf (buf, "\\\\");
	    break;
	  case '\"':
	    sprintf (buf, "\\\"");
	    break;
	  case '\'':
	    sprintf (buf, "\\\'");
	    break;
	  default:
	    buf[0] = c;
	    buf[1] = '\0';
	    break;
	  }
      else
	sprintf (buf, "\\x%02x", c);
      break;
    }
  return buf;
}

/* ===============================================================================
 * Function: word2str
 * Description: 
 *      Converts a string with possible non-printable characters into a string
 *      consisting entirely of printable characters.
 * Input: string, length stored in first byte
 * Output: pointer to static string
 * =============================================================================== */

char *
word2str (u_char * s)
{
  static char buf[1024];
  char *p = buf;
  int i, len = (int) *s++;

  bzero (buf, sizeof (buf));
  for (i = 0; i < len; i++)
    {
      strcpy (p, char2str (s[i]));
      p = strchr (p, '\0');
    }
  return buf;
}


/* =========================================================================
 * Function: de_escape_string
 * Description: 
 *      Turn a string containing \ escapes into real characters
 *      Note this destroys the string s in the process
 * Input: string to de-escape 
 * Output: the de-escaped string
 * ========================================================================= */
char *
de_escape_string (char *s)
{
  char *org = s;
  char *d = s;
  while (*s)
    {
      if (*s == '\\')
	{
	  s++;
	  switch (*s++)
	    {
	    case '\0':
	      *d++ = '\\';
	      s--;
	      break;
	    case '\\':
	      *d++ = '\\';
	      break;
	    case 'b':
	      *d++ = '\b';
	      break;
	    case 'f':
	      *d++ = '\f';
	      break;
	    case 'n':
	      *d++ = '\n';
	      break;
	    case 'r':
	      *d++ = '\r';
	      break;
	    case 't':
	      *d++ = '\r';
	      break;
	    case '\"':
	      *d++ = '\"';
	      break;
	    case '\'':
	      *d++ = '\'';
	      break;
	    case 'x':
	      {
		int num = 0;
		if (isxdigit (*s))
		  {
		    char ch = toupper (*s++);
		    num = ch <= '9' ? ch - '0' : ch - 'A' + 10;
		  }
		if (isxdigit (*s))
		  {
		    char ch = toupper (*s++);
		    num = (num << 4) + (ch <= '9' ? ch - '0' : ch - 'A' + 10);
		  }
		*d++ = num;
		break;
	      }
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	      {
		int num = *(s - 1) - '0';
		if (*s >= '0' && *s <= '7')
		  num = (num << 3) + *s++ - '0';
		if (*s >= '0' && *s <= '7')
		  num = (num << 3) + *s++ - '0';
		*d++ = num;
		break;
	      }
	    }
	}
      else
	*d++ = *s++;
    }
  *d = '\0';
  return (org);
}

/* =========================================================================
 * Function: str255_to_string 
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */

char *
str255_to_string (u_char * str255, char *str)
{
  static char buf[256];
  char *str_base = NULL;
  int len = *str255++;
  int i = 0;

  /* if haven't been given a str to write to then use local buffer */
  if (!str)
    str = &(buf[0]);

  str_base = str;
  for (i = 0; i < len; i++)
    {
      *str++ = *str255++;
    }
  *str = '\0';

  return str_base;
}
