/**************************************************************************
 *
 * mg.special.c -- Special pass for mg_passes
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
 * $Id: mg.special.c 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#include "sysfuncs.h"

#include "mg.h"
#include "build.h"


/*
   $Log$
   Revision 1.1  2003/02/20 21:18:24  mdewsnip
   Addition of MG package for search and retrieval

   Revision 1.1  1999/08/10 21:18:03  sjboddie
   renamed mg-1.3d directory mg

   Revision 1.1  1998/11/17 09:34:51  rjmcnab
   *** empty log message ***

   * Revision 1.3  1994/10/20  03:56:53  tes
   * I have rewritten the boolean query optimiser and abstracted out the
   * components of the boolean query.
   *
   * Revision 1.2  1994/09/20  04:41:44  tes
   * For version 1.1
   *
 */

static char *RCSID = "$Id: mg.special.c 16583 2008-07-29 10:20:36Z davidb $";


FILE *fp;


int 
init_special (char *FileName)
{
  char FName[200];
  sprintf (FName, "%s%s", FileName, ".docnums");
  if (!(fp = fopen (FName, "w")))
    return (COMPERROR);
  return (COMPALLOK);
}




int 
process_special (u_char * s_in, int l_in)
{
  static int un = 1;
  char *s_done = (char *) (s_in + l_in);
  for (; (char *) s_in != s_done; s_in++)
    if (*s_in == '<')
      {
	if (strncmp ((char *) s_in, "<DOCNO>", 7) == 0)
	  {
	    char *s_end = (char *) (s_in + 1);
	    for (; s_end != s_done; s_end++)
	      if (*s_end == '<')
		if (strncmp (s_end, "</DOCNO>", 8) == 0)
		  break;
	    if (s_end == s_done)
	      {
		fprintf (fp, "NO DOC ID %d\n", un++);
		return COMPALLOK;
	      }
	    s_in += 7;
	    while (s_end != (char *) s_in &&
		   (*(s_end - 1) == ' ' || *(s_end - 1) == '\n'))
	      s_end--;
	    while ((char *) s_in != s_end &&
		   (*s_in == ' ' || *s_in == '\n'))
	      s_in++;
	    for (; (char *) s_in != s_end; s_in++)
	      putc (*s_in, fp);
	    putc ('\n', fp);
	    return (COMPALLOK);
	  }
      }
  fprintf (fp, "NO DOC ID %d\n", un++);
  return (COMPALLOK);
}








int 
done_special (char *FileName)
{
  fclose (fp);
  return COMPALLOK;
}
