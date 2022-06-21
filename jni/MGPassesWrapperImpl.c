/*
 *    MGPassesWrapperImpl.c
 *    Copyright (C) 2002 New Zealand Digital Library, http://www.nzdl.org
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <jni.h>
#include <assert.h>
#include "sysfuncs.h"
#include "org_greenstone_mg_MGPassesWrapper.h"

#include "mg_passes_4jni.h"
#include "mg_files.h"

/* if we need to use java objects, we should initialise their field ids here*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_initIDs(JNIEnv *j_env, jclass j_cls)
{
  return;
}

JNIEXPORT jboolean JNICALL
Java_org_greenstone_mg_MGPassesWrapper_initCSide(JNIEnv *j_env, jobject j_obj)
{
  clear_variables();
  return 1;  /* true - no errors */
}  

/* add a pass type T1, T2, I1, I2, S */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_addPass(JNIEnv *j_env, 
					       jobject j_obj, 
					       jchar j_pass_type,
					       jchar j_pass_num)
{
  /* get the level as a c char */
  const char pass_type = j_pass_type;
  const char pass_num = j_pass_num;
  add_pass(pass_type, pass_num);
  
}

/* Set the filename */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_setFileName(JNIEnv *j_env, 
						   jobject j_obj,
						   jstring j_filename)
{
  /* Get the filename as a C string */
  const char* filename = (*j_env)->GetStringUTFChars(j_env, j_filename, NULL);
 
  assert(filename != NULL);
  set_filename(filename);
  
  /* Release the string */
 (*j_env)->ReleaseStringUTFChars(j_env, j_filename, filename);

}

/* Set the base path */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_setBasePath(JNIEnv *j_env, 
						   jobject j_obj,
						   jstring j_basepath)
{
  /* Get the base_path as a C string */
  const char* basepath = (*j_env)->GetStringUTFChars(j_env, j_basepath, NULL);
  assert(basepath != NULL);
 
  set_basepath(basepath);
  
  /* Release the string */
  (*j_env)->ReleaseStringUTFChars(j_env, j_basepath, basepath);

}

/* set the level for the inverted file */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_setInvfLevel(JNIEnv *j_env, 
						    jobject j_obj, 
						    jchar j_level)
{
  /* get the level as a c char */
  const char level = j_level;
  set_invf_level(level);
  
}

/* set the stemmer and stem method */
JNIEXPORT void JNICALL 
Java_org_greenstone_mg_MGPassesWrapper_setStemOptions(JNIEnv *j_env, 
						      jobject j_obj,
						      jstring j_stemmer, 
						      jint j_method)
{

  const char* stemmer = (*j_env)->GetStringUTFChars(j_env, j_stemmer, NULL);
  int method = j_method;

  assert(stemmer != NULL);
  set_stem_options(stemmer, method);
  
  /* Release the string */
  (*j_env)->ReleaseStringUTFChars(j_env, j_stemmer, stemmer);
 }

/** Specify  the size of the document buffer in kilobytes.
    If any document is larger than  bufsize,  the  program
    will  abort with an error message. 
*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_setBufferSize(JNIEnv *j_env, 
						     jobject j_obj, 
						     jlong j_bufsize){
  long buffer = j_bufsize;
  set_buffer_size(buffer);
}
    
/** Maximum amount of memory to use for  the index  pass-2  file
    inversion  in  megabytes.
*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_setInversionMemLimit(JNIEnv *j_env, 
							    jobject j_obj, 
							    jint j_limit) {
  int limit = j_limit;
  set_inversion_limit(limit);
}

/** If true, treat   SGML  tags  as  non-words  when  building  the
    inverted file.
*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_ignoreSGMLTags(JNIEnv *j_env, 
						      jobject j_obj, 
						      jboolean j_ignore){
  int ignore = j_ignore;
  ignore_sgml_tags(ignore);
}

/** if mg_passes fails, the document that caused the failure will be
    output to the trace file or STDERR. 
*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_dumpFailedDocument(JNIEnv *j_env, 
							  jobject j_obj, 
							  jboolean j_dump) {
  int dump = j_dump;
  dump_failed_document(dump);
}

/** output statistics on the compression performance to  a  file  
    called  *.compression.stats.    frequency  specifies  the  interval
    (in kilobytes of source text) between outputting each line of 
    statistics. 
*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_outputCompStats(JNIEnv *j_env, 
						       jobject j_obj, 
						       jint j_frequency){
  int comp_stat_point = j_frequency;
  set_comp_stat_point(comp_stat_point);
  
}
/** activate tracing, a line will be output every tracepos input bytes */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_enableTracing(JNIEnv *j_env, 
						     jobject j_obj, 
						     jint j_tracepos){
  int tracepos = j_tracepos;
  set_trace_point(tracepos);
}

/** specify the name of the trace file */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGPassesWrapper_setTraceFile(JNIEnv *j_env, 
						     jobject j_obj, 
						     jstring j_tracefile){

  const char* tracefile = (*j_env)->GetStringUTFChars(j_env, j_tracefile, NULL);
  assert(tracefile != NULL);
  set_trace_file(tracefile);
  /* Release the string */
  (*j_env)->ReleaseStringUTFChars(j_env, j_tracefile, tracefile);
}

/* initialise the pass through the documents. must be called after all
   the set methods 
*/
JNIEXPORT jboolean JNICALL 
Java_org_greenstone_mg_MGPassesWrapper_init(JNIEnv *j_env, 
					    jobject j_obj) {

  init_driver();
  return 1;
}


/* process one document */
JNIEXPORT jboolean JNICALL 
Java_org_greenstone_mg_MGPassesWrapper_processMGDocument(JNIEnv *j_env, 
						       jobject j_obj,
						       jbyteArray j_doc_text) {
  /* Get the text as a C string */
  int length = (*j_env)->GetArrayLength(j_env, j_doc_text);
  u_char * text_buffer = (u_char *)(*j_env)->GetByteArrayElements(j_env, j_doc_text, NULL);
  process_document(text_buffer, length);
  /* Release the string */
  (*j_env)->ReleaseByteArrayElements(j_env, j_doc_text, text_buffer,0);
  return 1;
}

/* finalise the pass through the documents */
JNIEXPORT jboolean JNICALL 
Java_org_greenstone_mg_MGPassesWrapper_finish(JNIEnv *j_env, 
					    jobject j_obj) {
  
  finalise_driver();
  return 1;
}

/** get the exit value once finished */
JNIEXPORT jint JNICALL 
Java_org_greenstone_mg_MGPassesWrapper_exitValue(JNIEnv *j_env, 
					    jobject j_obj) {

  return get_exit_value();
}

