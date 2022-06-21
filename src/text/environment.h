/**************************************************************************
 *
 * environment.h -- mgquery environment functions
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
 * $Id: environment.h,v 1.3 1994/10/20 03:56:43 tes Exp $
 *
 **************************************************************************/


#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#define OUTPUT_TEXT     'T'
#define OUTPUT_HEADERS  'H'
#define OUTPUT_SILENT   'S'
#define OUTPUT_EXTRAS   'E'
#define OUTPUT_HILITE   'I'
#define OUTPUT_COUNT    'C'
#define OUTPUT_DOCNUMS  'D'

#define QUERY_RANKED    'R'
#define QUERY_APPROX    'A'
#define QUERY_BOOLEAN   'B'
#define QUERY_DOCNUMS   'D'


char get_output_type (void);
char get_query_type (void);

extern char *ConstraintErrorStr;
int SetEnv (char *name, char *data, char *(*Constraint) (char *, char *));
char *GetEnv (char *name);
char *GetDefEnv (char *name, char *def);
int UnsetEnv (char *name, int Force);
char *de_escape_string (char *s);
int PushEnv (void);
int PopEnv (void);
int EnvStackHeight (void);
char *GetEnvName (int i);
int BooleanEnv (char *data, int def);
long IntEnv (char *data, long def);

void InitEnv (void);
void UninitEnv (void);

#endif
