/**************************************************************************
 *
 * commands.c -- mgquery command processing functions
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
 * $Id: commands.c,v 1.2 1994/09/20 04:41:22 tes Exp $
 *
 **************************************************************************/

/*
   $Log: commands.c,v $
   * Revision 1.2  1994/09/20  04:41:22  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: commands.c,v 1.2 1994/09/20 04:41:22 tes Exp $";

#include "sysfuncs.h"
#include "memlib.h"
#include "messages.h"

#include "environment.h"
#include "mg.h"
#include "locallib.h"
#include "backend.h"		/* for qd */
#include "term_lists.h"		/* for MAXTERMSTRLEN */


extern char *PathName;
extern FILE *OutFile, *InFile;
extern int OutPipe, InPipe;
extern int Quitting;

char *CommandsErrorStr = NULL;

/* [TS: June/95] */
/* current qd for use by any commands */
static query_data *the_qd = NULL;


#include "help.mg.h"
#include "warranty.h"
#include "conditions.h"

static void 
CmdQuit (void)
{
  Quitting = 1;
}

static void 
CmdPush (void)
{
  PushEnv ();
}


static void 
CmdPop (void)
{
  PopEnv ();
  if (EnvStackHeight () == 1)
    PushEnv ();
}

static void 
CmdHelp (void)
{
  FILE *more;
  int i;
  if (!(more = popen (GetDefEnv ("pager", "more"), "w")))
    {
      fprintf (stderr, "ERROR : Unable to run more\n");
      return;
    }
  for (i = 0; i < sizeof (help_str) / sizeof (help_str[0]); i++)
    fputs (help_str[i], more);
  pclose (more);
}


static void 
CmdWarranty (void)
{
  FILE *more;
  int i;
  if (!(more = popen (GetDefEnv ("pager", "more"), "w")))
    {
      fprintf (stderr, "ERROR : Unable to run more\n");
      return;
    }
  for (i = 0; i < sizeof (warranty_str) / sizeof (warranty_str[0]); i++)
    fputs (warranty_str[i], more);
  pclose (more);
}


static void 
CmdConditions (void)
{
  FILE *more;
  int i;
  if (!(more = popen (GetDefEnv ("pager", "more"), "w")))
    {
      fprintf (stderr, "ERROR : Unable to run more\n");
      return;
    }
  for (i = 0; i < sizeof (cond_str) / sizeof (cond_str[0]); i++)
    fputs (cond_str[i], more);
  pclose (more);
}

static void 
CmdReset (void)
{
  while (EnvStackHeight () > 1)
    PopEnv ();
  PushEnv ();
}

static void 
CmdDisplay (void)
{
  int i;
  size_t l = 0;
  char *name;
  for (i = 0; (name = GetEnvName (i)) != NULL; i++)
    if (strlen (name) > l)
      l = strlen (name);

  for (i = 0; (name = GetEnvName (i)) != NULL; i++)
    printf ("%-*s = \"%s\"\n", (int) l, name, GetEnv (name));
}

static void 
CmdQueryTerms (void)
{
  static char terms_str[MAXTERMSTRLEN + 1];

  if (the_qd && the_qd->TL)
    {
      ConvertTermsToString (the_qd->TL, terms_str);
      printf ("%s\n", terms_str);
      fflush (stdout);
    }

}


static void 
CmdSet (char *name, char *value)
{
  if (!*name)
    {
      CommandsErrorStr = "Blank names are not permitted";
      return;
    }
  switch (SetEnv (name, value, NULL))
    {
    case -1:
      CommandsErrorStr = "Out of memory\n";
      break;
    case -2:
      CommandsErrorStr = ConstraintErrorStr;
      break;
    }
}

static void 
CmdUnset (char *name)
{
  if (!*name)
    {
      CommandsErrorStr = "Blank names are not permitted";
      return;
    }
  if (UnsetEnv (name, 0) == -1)
    CommandsErrorStr = "Specified name does not exist";
}






/*
 * Do the .input command 
 */
static void 
CmdInput (char *param1, char *param2)
{
  enum
  {
    READ, APPEND, PIPEFROM
  }
  mode;
  FILE *NewInput;
  char *buf, *s;

  /* join the two parameters together */
  buf = Xmalloc (strlen (param1) + strlen (param2) + 1);
  strcpy (buf, param1);
  strcat (buf, param2);
  s = buf;


  while (*s == ' ' || *s == '\t')
    s++;

  if (*s == '<')
    {
      s++;
      mode = READ;
    }
  else if (*s == '|')
    {
      mode = PIPEFROM;
      s++;
    }
  else
    {
      if (InPipe)
	pclose (InFile);
      else if (InFile != stdin)
	fclose (InFile);
      InPipe = 0;
      InFile = stdin;
      Xfree (buf);
      return;
    }

  while (*s == ' ' || *s == '\t')
    s++;

  if (*s)
    {
      if (mode == READ)
	NewInput = fopen (s, "r");
      else
	NewInput = popen (s, "r");
      if (NewInput)
	{
	  if (InPipe)
	    pclose (InFile);
	  else if (InFile != stdin)
	    fclose (InFile);
	  InPipe = (mode == PIPEFROM);
	  InFile = NewInput;
	}
      else
	CommandsErrorStr = "Bad input file/pipe";
    }
  else
    {
      if (InPipe)
	pclose (InFile);
      else if (InFile != stdin)
	fclose (InFile);
      InPipe = 0;
      InFile = stdin;
    }
  Xfree (buf);
}



static void 
CmdOutput (char *param1, char *param2)
{
  enum
  {
    WRITE, APPEND, PIPETO
  }
  mode;
  char *buf, *s;
  FILE *NewOutput;

  /* join the two parameters together */
  buf = Xmalloc (strlen (param1) + strlen (param2) + 1);
  strcpy (buf, param1);
  strcat (buf, param2);
  s = buf;

  while (*s == ' ' || *s == '\t')
    s++;

  if (*s == '>')
    {
      s++;
      mode = APPEND;
      if (*s == '>')
	s++;
      else
	mode = WRITE;
    }
  else if (*s == '|')
    {
      mode = PIPETO;
      s++;
    }
  else
    {
      if (OutPipe)
	pclose (OutFile);
      else if (OutFile != stdout)
	fclose (OutFile);
      OutPipe = 0;
      OutFile = stdout;
      Xfree (buf);
      return;
    }

  while (*s == ' ' || *s == '\t')
    s++;

  if (*s)
    {
      if (mode == PIPETO)
	NewOutput = popen (s, "w");
      else
	NewOutput = fopen (s, mode == WRITE ? "w" : "a");
      if (NewOutput)
	{
	  if (OutPipe)
	    pclose (OutFile);
	  else if (OutFile != stdout)
	    fclose (OutFile);
	  OutPipe = (mode == PIPETO);
	  OutFile = NewOutput;
	}
      else
	CommandsErrorStr = "Bad output file/pipe";
    }
  else
    {
      if (OutPipe)
	pclose (OutFile);
      else if (OutFile != stdout)
	fclose (OutFile);
      OutPipe = 0;
      OutFile = stdout;
    }
  Xfree (buf);
}





static char *
ParseArg (char *line, char *buffer)
{
  if (*line == '\"')
    {
      line++;
      while (*line != '\"' && *line != '\0')
	{
	  if (*line == '\\')
	    *buffer++ = *line++;
	  if (*line != '\0')
	    *buffer++ = *line++;
	}
      if (*line == '\"')
	line++;
    }
  else
    while (*line != ' ' && *line != '\t' && *line != '\0')
      {
	if (*line == '\\')
	  line++;
	if (*line != '\0')
	  *buffer++ = *line++;
      }
  *buffer = '\0';
  while (*line == ' ' || *line == '\t')
    line++;
  return (line);
}



char *
ProcessCommands (char *line, query_data * qd)
{
  struct
  {
    char *Command;
    void (*action) (void);
    void (*action1) (char *);
    void (*action2) (char *, char *);
  }
   *dc, dot_command[] =
  {
    {
      "set", NULL, NULL, CmdSet
    }
    ,
    {
      "input", NULL, NULL, CmdInput
    }
    ,
    {
      "output", NULL, NULL, CmdOutput
    }
    ,
    {
      "unset", NULL, CmdUnset, NULL
    }
    ,
    {
      "help", CmdHelp, NULL, NULL
    }
    ,
    {
      "reset", CmdReset, NULL, NULL
    }
    ,
    {
      "display", CmdDisplay, NULL, NULL
    }
    ,
    {
      "push", CmdPush, NULL, NULL
    }
    ,
    {
      "quit", CmdQuit, NULL, NULL
    }
    ,
    {
      "pop", CmdPop, NULL, NULL
    }
    ,
    {
      "warranty", CmdWarranty, NULL, NULL
    }
    ,
    {
      "conditions", CmdConditions, NULL, NULL
    }
    ,
    {
      "queryterms", CmdQueryTerms, NULL, NULL
    }
  };


  char command[512];
  char param1[512], param2[512];
  CommandsErrorStr = NULL;
  the_qd = qd;			/* set globally to be accessed by command functions if needed */
  while (*line == ' ' || *line == '\t')
    line++;
  while (*line == '.')
    {
      int i;
      line++;
      for (i = 0; isalpha (*line) && i < 512; i++, line++)
	command[i] = tolower (*line);
      while (*line == ' ' || *line == '\t')
	line++;
      if (i != 512)
	{
	  command[i] = '\0';
	  for (i = 0, dc = dot_command; i < NUMOF (dot_command); i++, dc++)
	    if (!strcmp (dc->Command, command))
	      {
		if (dc->action)
		  dc->action ();
		else if (dc->action1)
		  {
		    line = ParseArg (line, param1);
		    dc->action1 (param1);
		    break;
		  }
		else if (dc->action2)
		  {
		    line = ParseArg (line, param1);
		    line = ParseArg (line, param2);
		    dc->action2 (param1, param2);
		    break;
		  }
		if (CommandsErrorStr)
		  *line = '\0';
		break;
	      }
	  if (i == NUMOF (dot_command))
	    i = 100;
	}
      if (i == 100)
	{
	  CommandsErrorStr = "Illegal command";
	  *line = '\0';
	}
      while (*line == ' ' || *line == '\t')
	line++;
    }
  return (line);
}





void 
read_mgrc_file (void)
{
  char line[1000];
  char FileName[256];
  int LineNum = 0;
  FILE *mgrc;

  strcpy (FileName, ".mgrc");
  mgrc = fopen (FileName, "r");

  if (!mgrc)
    {
      char *home_path = getenv ("HOME");
      if (home_path)
	{
	  sprintf (FileName, "%s/.mgrc", home_path);
	  mgrc = fopen (FileName, "r");
	}
    }

  if (!mgrc)
    return;

  while (fgets (line, sizeof (line), mgrc))
    {
      char *s, linebuf[1000];
      LineNum++;
      if ((s = strchr (line, '\n')) != NULL)
	*s = '\0';
      strcpy (linebuf, line);
      if ((s = strchr (line, '#')) != NULL)
	*s = '\0';
      if (!*line)
	continue;
      ProcessCommands (line, NULL);
      if (CommandsErrorStr)
	Message ("%s\n%s : ERROR in line %d of %s\n%s : %s\n",
		 linebuf,
		 msg_prefix, LineNum, FileName,
		 msg_prefix, CommandsErrorStr);
    }
  fclose (mgrc);
}
