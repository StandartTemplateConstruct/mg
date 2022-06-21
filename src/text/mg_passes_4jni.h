/**************************************************************************
 *
 * mg_passes_4jni.h -- modified mg_passes for use from jni
 * Copyright (C) 2004 New Zealand Digital Library, http://www.nzdl.org
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
 * $Id: mg_passes_4jni.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/
#ifndef H_MG_PASSES_4JNI
#define H_MG_PASSES_4JNI

/* clear all the settings from one mg_passes run to the next */
void clear_variables();

/* the following are methods to set all the variables that used to be
   set by command line args */

/* -S, -T1, -T2, -I1, -I2, args to mg_passes */
void add_pass(char pass_type, char pass_num);

/* -D arg to mg_passes */
void dump_failed_document(int dump);

/* -G arg to mg_passes */
void ignore_sgml_tags(int ignore);

/* -b arg to mg_passes */
void set_buffer_size(long size);

/* -c arg to mg_passes */
void set_chunk_limit(long chunk_limit);

/* -C arg to mg_passes */
void set_comp_stat_point(int stat_point);

/* -f arg to mg_passes */
void set_filename(const char * filen);

/* -m arg to mg_passes */
void set_inversion_limit(int limit);

/* -1, -2, -3 args to mg_passes */
void set_invf_level(char level);

/* -W arg to mg_passes */
void set_make_weights(int make_w);

/* -M arg to mg_passes */
void set_max_numeric(int max_numeric);

/* -a, -s args to mg_passes */
void set_stem_options(const char * stemmer, int method);

/* -t arg to mg_passes */
void set_trace_point(int tracepos);

/* -n arg to mg_passes */
void set_trace_file(const char * filen);

/* The old driver method has been split into 3:
init_driver, process_document (called numdocs times), 
finalise_driver.
The above set vars methods should all be called before init_driver.
*/

void init_driver ();

void process_document(const u_char *buffer, int len);

void finalise_driver();

#endif
