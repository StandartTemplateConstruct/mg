/**************************************************************************
 *
 * messages.h -- Message and error functions
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
 * $Id: messages.h,v 1.2 1994/09/20 04:20:06 tes Exp $
 *
 **************************************************************************/

#ifndef H_MESSAGES
#define H_MESSAGES

#ifdef __GNUC__
#define VOLATILE volatile
#else
#define VOLATILE
#endif

extern char *msg_prefix;

void VOLATILE FatalError (int ExitCode, const char *fmt,...);

void Message (const char *fmt,...);

#endif
