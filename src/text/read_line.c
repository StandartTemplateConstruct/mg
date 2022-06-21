/**************************************************************************
 *
 * read_line.c -- Input line reading routines for mgquery
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
 * $Id: read_line.c,v 1.2 1997/08/04 02:43:27 wew Exp wew $
 *
 **************************************************************************/

#include "sysfuncs.h"
#include "memlib.h"

#ifdef WITH_GNU_READLINE
#include "readline/readline.h"
#include "readline/chardefs.h"
#if 0
#include "readline/history.h"  /* dropped from recent versions of GNU readline */
#endif
#endif

#include "globals.h"
#include "environment.h"
#include "read_line.h"



#ifdef WITH_GNU_READLINE
int rl_bind_key (int, int (*)());


void 
Init_ReadLine (void)
{
  /* Make TAB just insert itself rather than do a file name completion */
  rl_bind_key (TAB, rl_insert);
}
#else
static FILE *rl_instream;
static FILE *rl_outstream;

void 
Init_ReadLine (void)
{
  rl_instream = stdin;
  rl_outstream = stdout;
}
#endif




/* WritePrompt() */
/* Write out a prompt if user is a TTY */
void 
WritePrompt (void)
{
  if (isatty (fileno (InFile)))
    {
      if (!BooleanEnv (GetEnv ("expert"), 0))
	fprintf (stderr, "Enter a command or query (.quit to terminate, .help for assistance).\n");
    }
}

#ifdef WITH_GNU_READLINE
static void memory_error_and_abort ();

voidstar
xmalloc (bytes)
     size_t bytes;
{
  voidstar temp = (char *) Xmalloc (bytes);

  if (!temp)
    memory_error_and_abort ();
  return (temp);
}

voidstar
xrealloc (pointer, bytes)
     voidstar pointer;
     size_t bytes;
{
  voidstar temp;

  if (!pointer)
    temp = (voidstar) xmalloc (bytes);
  else
    temp = (voidstar) Xrealloc (pointer, bytes);

  if (!temp)
    memory_error_and_abort ();

  return (temp);
}

static void
memory_error_and_abort ()
{
  fprintf (stderr, "history: Out of virtual memory!\n");
  abort ();
}
#endif


#ifndef WITH_GNU_READLINE

static char *
readline (char *pmt)
{
  static char buf[MAXLINEBUFFERLEN + 1];
  char *s;

  fprintf (rl_outstream, "%s", pmt);
  s = fgets (buf, sizeof (buf), rl_instream);
  if (s)
    {
      char *s1 = strrchr (s, '\n');
      if (s1 && *(s1 + 1) == '\0')
	*s1 = '\0';
    }
  return s ? Xstrdup (s) : NULL;
}
#endif

/*
 * This routine returns a pointer to the users entered line 
 *
 */
static char *
GetLine (char *pmt)
{
  static char *the_line = NULL;
  if (the_line)
    Xfree (the_line);
  the_line = NULL;
  rl_instream = InFile;
  if (isatty (fileno (InFile)))
    {
      fputc ('\r', stderr);
      if (!isatty (fileno (OutFile)))
	rl_outstream = stderr;
      else
	rl_outstream = OutFile;
      the_line = readline (pmt);
    }
  else
    {
      if (isatty (fileno (OutFile)))
	{
	  the_line = readline (pmt);
	  fprintf (stderr, "%s\n", the_line ? the_line : "");
	}
      else
	{
	  the_line = readline ("");
	  if (the_line)
	    fprintf (stderr, "%s%s\n", pmt, the_line);
	}
    }
#ifdef WITH_GNU_READLINE
  if (the_line && *the_line)
    add_history (the_line);
#endif
  return (the_line);
}






char *
GetMultiLine (void)
{
  static char *line = NULL;
  char *s;
  if (line)
    Xfree (line);
  line = NULL;
  if (!(s = GetLine ("> ")))
    return (NULL);
  if (!(line = Xstrdup (s)))
    {
      fprintf (stderr, "Unable to allocate memory for the line\n");
      abort ();
    }

  while ((s = strrchr (line, '\\')) && *(s + 1) == '\0')
    {
      char *new;
      *strrchr (line, '\\') = '\0';
      if (!(s = GetLine ("? ")))
	return (NULL);
      if (!(new = Xmalloc (strlen (line) + strlen (s) + 2)))
	{
	  fprintf (stderr, "Unable to allocate memory for the line\n");
	  abort ();
	}
      strcpy (new, line);
      strcat (new, "\n");
      strcat (new, s);
      Xfree (line);
      line = new;
    }
  return (line);
}
