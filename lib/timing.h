/**************************************************************************
 *
 * timing.h -- Program timing routines
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
 * $Id: timing.h,v 1.2 1994/09/20 04:20:11 tes Exp $
 *
 **************************************************************************/

#ifndef H_TIMING
#define H_TIMING


#ifdef HAVE_TIMES
# if TIME_WITH_SYS_TIME
#  include <sys/times.h>
#  include <time.h>
#  ifndef CLK_TCK
#   define CLK_TCK sysconf(_SC_CLK_TCK)
#  endif
# else
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
#else
# include <sys/time.h>
# include <sys/resource.h>
#endif


typedef struct
  {
    double RealTime, CPUTime;
  }
ProgTime;

double RealTime (void);

double CPUTime (double *user, double *sys);

void GetTime (ProgTime * StartTime);

/* FinishTime may be NULL */
char *ElapsedTime (ProgTime * StartTime, ProgTime * FinishTime);

#ifdef HAVE_TIMES
char *cputime_string (clock_t clk);
#else
void time_normalise (struct timeval *t);
char *cputime_string (struct timeval *t);
#endif

#endif
