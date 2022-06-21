/**************************************************************************
 *
 * messages.c -- Message and error functions
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
 * $Id: messages.c,v 1.1 1994/08/22 00:24:48 tes Exp $
 *
 **************************************************************************/

/*
   $Log: messages.c,v $
   * Revision 1.1  1994/08/22  00:24:48  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: messages.c,v 1.1 1994/08/22 00:24:48 tes Exp $";

#include "sysfuncs.h"

#include <stdarg.h>
#include "messages.h"

char *msg_prefix = "";


void VOLATILE 
FatalError (int ExitCode, const char *fmt,...)
{
  char buf[1024];
  char *s, *pfx;
  va_list args;
  va_start (args, fmt);
  vsprintf (buf, fmt, args);
  s = strrchr (buf, '\n');
  if (!s || *(s + 1) != '\0')
    strcat (buf, "\n");
  pfx = strrchr (msg_prefix, '/');
  pfx = pfx ? pfx + 1 : msg_prefix;
  fprintf (stderr, "%s%s%s", pfx, *pfx ? " : " : "", buf);
  exit (ExitCode);
}



/*
 * This function writes messages to stderr. Due to the fact that I can't 
 * guarantee that the fprintf call is monatomic I have to implement a 
 * semaphore system.
 */
void 
Message (const char *fmt,...)
{
  char buf[1024];
  char *s, *pfx;
  va_list args;
  va_start (args, fmt);
  vsprintf (buf, fmt, args);
  s = strrchr (buf, '\n');
  if (!s || *(s + 1) != '\0')
    strcat (buf, "\n");

  pfx = strrchr (msg_prefix, '/');
  pfx = pfx ? pfx + 1 : msg_prefix;
  fprintf (stderr, "%s%s%s", pfx, *pfx ? " : " : "", buf);

}
