/**************************************************************************
 *
 * utils.h -- Functions which are common utilities for the image programs
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
 * $Id: utils.h,v 1.2 1994/09/20 04:42:17 tes Exp $
 *
 **************************************************************************/

#ifndef __UTILS_H
#define __UTILS_H

#include "sysfuncs.h"

extern int V;

#define DEBUG

#define min(a,b) ((a)<(b) ? (a) : (b))
#define max(a,b) ((a)>(b) ? (a) : (b))

#define GEN_MAGIC(a,b,c,d) ((unsigned long)(((a)<<24) + ((b)<<16) + \
                                            ((c)<<8) + (d)))
 
/* --- MG magic file codes [TS:Aug/95] --- */
#define MAGIC_FELICS            GEN_MAGIC('M','G','F','X')
#define MAGIC_BILEVEL           GEN_MAGIC('M','G','B','I')
#define MAGIC_TIC               GEN_MAGIC('M','G','T','C')
#define IS_MAGIC(a) ((((u_long)(a)) & 0xffff0000) == MAGIC_XXXX)
/* --------------------------------------- */

#define MAGIC_P1      0x5031	/* P1   - pbm */
#define MAGIC_P2      0x5032	/* P2   - pgm */
#define MAGIC_P3      0x5033	/* P3   - ppm */
#define MAGIC_P4      0x5034	/* P4   - rawbits pbm */
#define MAGIC_P5      0x5035	/* P5   - rawbits pgm */
#define MAGIC_P6      0x5036	/* P6   - rawbits ppm */

int isEOF (FILE * fp);
int getmagicno_byte (FILE * fp);
int getmagicno_short (FILE * fp);
int getmagicno_short_pop (FILE * fp);
int getmagicno_long (FILE * fp);



void error_msg (char *prog, char *message, char *extra);
void warn (char *prog, char *message, char *extra);

void readline (char str[], FILE * fp);

void magic_write (FILE * fp, u_long magic_num);
void magic_check (FILE * fp, u_long magic_num);
u_long magic_read (FILE * fp);
/* kerry's code */

int getint (FILE * fp);
unsigned int gethint (FILE * fp);
int isinteger (char s[]);

#endif
