/**************************************************************************
 *
 * timing.c -- Program timing routines
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
 * $Id: timing.c,v 1.1 1994/08/22 00:24:53 tes Exp $
 *
 **************************************************************************/

/*
   $Log: timing.c,v $
   * Revision 1.1  1994/08/22  00:24:53  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: timing.c,v 1.1 1994/08/22 00:24:53 tes Exp $";

#include "sysfuncs.h"
#include "timing.h"


/*
 * Return the time in seconds since 00:00:00 GMT, Jan. 1, 1970 to the
 * best precision possible
 *
 */
double 
RealTime (void)
{
  return ((double) time (NULL));
}



/*
 * Return the amount of system and user CPU used by the process to date to 
 * the best precision possible. If user is non-null then it is initialised to 
 * the user time. If sys is non-null then it is initialised to the system time.
 *
 */
double 
CPUTime (double *user, double *sys)
{

#if defined(HAVE_TIMES)

  struct tms buffer;
  static double clk_tck = 0;
  double u, s;

  times (&buffer);

  if (clk_tck == 0)
    clk_tck = CLK_TCK;

  u = (double) buffer.tms_utime / clk_tck;
  s = (double) buffer.tms_stime / clk_tck;
  if (user)
    *user = u;
  if (sys)
    *sys = s;
  return u + s;

#elif defined(HAVE_GETRUSAGE)

  struct rusage ruse;
  getrusage (RUSAGE_SELF, &ruse);
  if (user)
    *user = (double) ruse.ru_utime.tv_sec +
      (double) ruse.ru_utime.tv_usec / 1000000;
  if (sys)
    *sys = (double) ruse.ru_stime.tv_sec +
      (double) ruse.ru_stime.tv_usec / 1000000;
  return ((double) ruse.ru_utime.tv_sec + ruse.ru_stime.tv_sec +
	((double) ruse.ru_utime.tv_usec + ruse.ru_stime.tv_usec) / 1000000);

#else

  -->Can NOT find a system time routine to use ! !!

#endif

}




/*
 * Get the Real and CPU time and store them in a ProgTime structure
 */
void 
GetTime (ProgTime * StartTime)
{
  StartTime->RealTime = RealTime ();
  StartTime->CPUTime = CPUTime (NULL, NULL);
}




/*
 * Display the Real and CPU time elapsed since the StartTime anf FinishTime 
 * structures were initialised. If FinishTime is NULL then FinishTime is
 * Now.
 */
char *
ElapsedTime (ProgTime * StartTime,
	     ProgTime * FinishTime)
{
  static char buf[50];
  double Real, CPU, hour, min, sec;
  if (!FinishTime)
    {
      Real = RealTime () - StartTime->RealTime;
      CPU = CPUTime (NULL, NULL) - StartTime->CPUTime;
    }
  else
    {
      Real = FinishTime->RealTime - StartTime->RealTime;
      CPU = FinishTime->CPUTime - StartTime->CPUTime;
    }
  hour = floor (CPU / 3600);
  min = floor ((CPU - hour * 3600) / 60);
  sec = CPU - hour * 3600 - min * 60;
  sprintf (buf, "%02.0f:%02.0f:%05.2f cpu, %02d:%02d:%02d elapsed.",
	   hour, min, sec,
	   ((int) ceil (Real)) / 3600,
	   (((int) ceil (Real)) % 3600) / 60, ((int) ceil (Real)) % 60);
  return (buf);
}


#define MILLION 1000000

/* ===============================================================================
 * Function: cputime_string
 * Description:
 *      Prints out given cpu time into a string buffer.
 * Input: cpu-time in clock_t or timeval format
 * Output: pointer to internal buffer 
 * =============================================================================== */

#ifdef HAVE_TIMES
char *
cputime_string (clock_t clk)
{
  static char buf[15];
  double CPU, hour, min, sec;

  CPU = (double) clk / (double) CLK_TCK;
  hour = floor (CPU / 3600);
  min = floor ((CPU - hour * 3600) / 60);
  sec = CPU - hour * 3600 - min * 60;
  sprintf (buf, "%02.0f:%02.0f:%05.2f", hour, min, sec);
  return (buf);
}
#else
char *
cputime_string (struct timeval *t)
{
  static char buf[15];
  double CPU, hour, min, sec;

  CPU = (double) t->tv_sec + (double) t->tv_usec / MILLION;
  hour = floor (CPU / 3600);
  min = floor ((CPU - hour * 3600) / 60);
  sec = CPU - hour * 3600 - min * 60;
  sprintf (buf, "%02.0f:%02.0f:%05.2f", hour, min, sec);
  return (buf);
}
#endif



/* ===============================================================================
 * Function: time_normalise
 * Description:
 *      If the micro-second component is negative or over a million,
 *      then must take the over/under flow and add it to the second component.
 * Usage:
 *      Use this routine when you have just been doing straight addition and
 *      subtraction on the micro-second component. For this could lead to a
 *      negative number or over a million.
 *
 * =============================================================================== */

#ifndef HAVE_TIMES
static void
time_normalise (struct timeval *t)
{
  while (t->tv_usec < 0)
    {
      t->tv_usec += MILLION;
      t->tv_sec--;
    }
  while (t->tv_usec > MILLION)
    {
      t->tv_usec -= MILLION;
      t->tv_sec++;
    }
}
#endif
