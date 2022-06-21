/*
 *    MGRetrieveWrapperImpl.c
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


#include "MGWrapperImpl.h"
#include <jni.h>
#include "org_greenstone_mg_MGRetrieveWrapper.h"

#include "backend.h"
#include "environment.h"
#include "text_get.h"
#include "stemmer.h"

#define MAX_INDEXES_CACHED 3

/* copied from mgquery, needed to convert paragraph numbers to document numbers
   for greenstone */
#if defined(PARADOCNUM) ||  defined(NZDL)
static int GetDocNumFromParaNum(query_data *qd, int paranum) {
  int Documents = qd->td->cth.num_of_docs;
  int *Paragraph = qd->paragraph;
  int low = 1, high = Documents;
  int mid = (low+high)/2;

  while ((mid = (low+high)/2) >=1 && mid <= Documents)
    {
      if (paranum > Paragraph[mid])
        low = mid+1;
      else if (paranum <= Paragraph[mid-1])
        high = mid-1;
      else
        return mid;
    }
  FatalError(1, "Bad paragraph number.\n");
  return 0;
}
#endif


/*********************************************
   initialisation stuff
 *********************************************/

/* cached ids for java stuff */
jfieldID FID_mg_data = NULL; /* MGWrapperData */
jfieldID FID_query_result = NULL; /* MGQueryResult */
jmethodID MID_addDoc = NULL; /* MGQueryResult.addDoc() */
jmethodID MID_addTerm = NULL; /* MGQueryResult.addTerm() */
jmethodID MID_addEquivTerm = NULL; /* MGQueryResult.addEquivTerm() */
jmethodID MID_setTotalDocs = NULL; /* MGQueryResult.setTotalDocs() */
jmethodID MID_clearResult = NULL; /* MGQueryResult.clear() */


/*******************************************
   Index caching
 *******************************************/

query_data* cached_indexes[MAX_INDEXES_CACHED] = { NULL };


/* Get the index data necessary to perform a query or document retrieval */
query_data*
loadIndexData(char* base_dir, char* index_path, char* text_path)
{
  char* index_path_name;
  char* text_path_name;
  query_data* qd;
  int i = 0;

  /* Form the path name of the desired indexes */
  index_path_name = (char*) malloc(strlen(base_dir) + strlen(index_path) + 1);
  assert(index_path_name != NULL);
  strcpy(index_path_name, base_dir);
  strcat(index_path_name, index_path);
  printf("Index pathname: %s\n", index_path_name);

  text_path_name = (char*) malloc(strlen(base_dir) + strlen(text_path) + 1);
  assert(text_path_name != NULL);
  strcpy(text_path_name, base_dir);
  strcat(text_path_name, text_path);
  printf("Text pathname: %s\n", text_path_name);

  /* Search through the cached indexes for the desired one */
  while (i < MAX_INDEXES_CACHED && cached_indexes[i] != NULL) {
    printf("(Cached) Pathname: %s\n", cached_indexes[i]->pathname);
    printf("(Cached) Textpathname: %s\n", cached_indexes[i]->textpathname);

    /* Check if the index has already been loaded */
    if ((strcmp(index_path_name, cached_indexes[i]->pathname) == 0) &&
	(strcmp(text_path_name, cached_indexes[i]->textpathname) == 0)) {
      /* Index has already been loaded and cached, so return it */
      printf("Found index!\n");
      free(index_path_name);
      free(text_path_name);
      return cached_indexes[i];
    }

    i++;
  }

  /* Text strings no longer needed */
  free(index_path_name);
  free(text_path_name);

  /* The index is not cached, so load it now */
  qd = InitQuerySystem(base_dir, index_path, text_path, NULL);
  if (!qd) {
    printf("Error: Could not InitQuerySystem()...\n");
    return NULL;
  }

  /* The index loaded OK, so cache it */
  /* This could be a little more sophisticated, eg. replace least frequently used index */
  if (i >= MAX_INDEXES_CACHED)
    i = MAX_INDEXES_CACHED - 1;

  /* Free the index being replaced */
  if (cached_indexes[i] != NULL)
    FinishQuerySystem(cached_indexes[i]);

  /* Cache the loaded index, and return it */
  cached_indexes[i] = qd;
  return cached_indexes[i];
}

/*
 * Class:     org_greenstone_mg_MGRetrieveWrapper
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGRetrieveWrapper_initIDs(JNIEnv *j_env, jclass j_cls)
{
  /* a long-"J" */
  FID_mg_data = (*j_env)->GetFieldID(j_env, j_cls, "mg_data_ptr_", "J");
  assert(FID_mg_data != NULL);

}

/*
 * Class:     org_greenstone_mg_MGRetrieveWrapper
 * Method:    initCSide
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_greenstone_mg_MGRetrieveWrapper_initCSide
(JNIEnv *j_env, jobject j_obj)
{
  /* Allocate a MGWrapperData object to store query parameters */
  MGWrapperData* data = (MGWrapperData*) malloc(sizeof(MGWrapperData));
  assert(data != NULL);

  /* Set default values - no stemming, no case-folding, boolean OR queries */
  data->defaultStemMethod = 0;
  data->defaultBoolCombine = 0;

  /* Allocate a QueryInfo object to store more query parameters */
  data->queryInfo = (QueryInfo*) malloc(sizeof(QueryInfo));
  assert(data->queryInfo != NULL);

  /* Set default values - 50 documents max, return term freqs, sort by rank */
  data->queryInfo->index = NULL;
  data->queryInfo->maxDocs = 50;
  data->queryInfo->needTermFreqs = 1;

  /* Save the object on the Java side */
  (*j_env)->SetIntField(j_env, j_obj, FID_mg_data, (long) data);

  /* Initialise MG environment variables */
  InitEnv();
  SetEnv("expert", "true", NULL);
  SetEnv("mode", "docnums", NULL);
  
  return 1;  /* true - no errors */
}

/*
 * Class:     org_greenstone_mg_MGRetrieveWrapper
 * Method:    unloadIndexData
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_greenstone_mg_MGRetrieveWrapper_unloadIndexData
(JNIEnv* j_env, jobject j_obj)
{
  /* Free all the loaded indexes */
  int i = 0;
  while (i < MAX_INDEXES_CACHED && cached_indexes[i] != NULL) {
    FinishQuerySystem(cached_indexes[i]);
    cached_indexes[i] = NULL;
    i++;
  }

  return 1;  /* true - no errors */
}
/* Choose MG index to search */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGRetrieveWrapper_setIndex(JNIEnv *j_env, jobject j_obj,
					  jstring j_index)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);

  /* Get the index name as a C string */
  const char* index = (*j_env)->GetStringUTFChars(j_env, j_index, NULL);
  assert(index != NULL);
  printf("Choosing index %s...\n", index);

  /* Free the previous index name */
  if (data->queryInfo->index)
    free(data->queryInfo->index);

  /* Allocate memory for the index name, and fill it */
  data->queryInfo->index = (char*) malloc(strlen(index) + 1);
  assert(data->queryInfo->index != NULL);
  strcpy(data->queryInfo->index, index);

  /* Release the index string */
  (*j_env)->ReleaseStringUTFChars(j_env, j_index, index);
}

/*
 * Class:     org_greenstone_mg_MGRetrieveWrapper
 * Method:    getDocument
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_greenstone_mg_MGRetrieveWrapper_getDocument
(JNIEnv *j_env, jobject j_obj,
					     jstring j_base_dir, jstring j_text_path,
					     jlong j_docnum)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);

  char* index_path;
  const char* base_dir;
  const char* text_path;
  query_data* qd;

  u_long pos, len;
  u_char* c_buffer = NULL;
  u_char* uc_buffer = NULL;
  int ULen;

  jstring result;

  /* Make sure an index has been specified */
  index_path = data->queryInfo->index;
  assert(index_path != NULL);

  /* Obtain C versions of the two string parameters */
  base_dir = (*j_env)->GetStringUTFChars(j_env, j_base_dir, NULL);
  if (base_dir == NULL) {
    return NULL;
  }
  text_path = (*j_env)->GetStringUTFChars(j_env, j_text_path, NULL);
  if (text_path == NULL) {
    (*j_env)->ReleaseStringUTFChars(j_env, j_base_dir, base_dir);
    return NULL;
  }

  /* Load the appropriate index for satisfying this request */
  printf("Document retrieval, index path: %s\n", index_path);
  qd = loadIndexData((char*) base_dir, (char*) index_path, (char*) text_path);

  /* The C text strings are no longer needed */
  (*j_env)->ReleaseStringUTFChars(j_env, j_base_dir, base_dir);
  (*j_env)->ReleaseStringUTFChars(j_env, j_text_path, text_path);

  /* Check that the index was loaded successfully */
  if (qd==NULL) {
    return NULL;
  }
  /*assert(qd != NULL);*/

  /* Get the document position and length in the text file */
  printf("Fetching document number %ld...\n", (unsigned long) j_docnum);
  FetchDocStart(qd, (unsigned long) j_docnum, &pos, &len);
  printf("Fetched document start. Pos: %ld, Len: %ld\n", pos, len);

  /* Allocate memory for the document text (from mg/src/text/mgquery.c:RawDocOutput()) */
  c_buffer = (u_char*) malloc(len);
  assert(c_buffer != NULL);
  uc_buffer = (u_char*) malloc((int) (qd->td->cth.ratio * 1.01 * len) + 100);
  assert(uc_buffer != NULL);

  /* Seek to the correct position in the file and read the document text */
  Fseek (qd->td->TextFile, pos, 0);
  Fread (c_buffer, 1, len, qd->td->TextFile);

  /* Decompress the document text into another buffer, and terminate it */
  DecodeText (qd->cd, c_buffer, len, uc_buffer, &ULen);
  uc_buffer[ULen] = '\0';

  /* Load the document text into a Java string */
  result = (*j_env)->NewStringUTF(j_env, uc_buffer);
  assert(result != NULL);

  /* Free C buffers */
  free(c_buffer);
  free(uc_buffer);

  /* Return the document text */
  return result;
}

