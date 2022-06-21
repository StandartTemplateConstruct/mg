/*
 *    MGSearchWrapperImpl.c
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
#include "org_greenstone_mg_MGSearchWrapper.h"

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


/* to access objects and methods on java side, need their field/method ids - 
 this initialises them at the start to avoid recalculating them each time they
 are needed
Note: the descriptors need to be exactly right, otherwise you get an error 
saying "no such field" but no reference to the fact that it has the right 
name but the wrong type.
Note: apparently the jclass is a local ref and should only work 
in the method that created it. It seems to work ok, but I'll make it
global cos the book said I should, and it may avoid future hassles.
*/
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_initIDs(JNIEnv *j_env, jclass j_cls)
{
  jclass JC_MGQueryResult;

  /* a long-"J" */
  FID_mg_data = (*j_env)->GetFieldID(j_env, j_cls, "mg_data_ptr_", "J");
  assert(FID_mg_data != NULL);

  /* an object -"L<class name>;" */
  FID_query_result = (*j_env)->GetFieldID(j_env, j_cls, "mg_query_result_",
					  "Lorg/greenstone/mg/MGQueryResult;");
  assert(FID_query_result != NULL);

  /* the methods we want to use */
  JC_MGQueryResult = (*j_env)->FindClass(j_env, "org/greenstone/mg/MGQueryResult");

  /* addDoc(long doc, float rank) */
  MID_addDoc = (*j_env)->GetMethodID(j_env, JC_MGQueryResult, "addDoc", "(JF)V");
  assert(MID_addDoc != NULL);

  /* addTerm(String term, int stem) */
  MID_addTerm = (*j_env)->GetMethodID(j_env, JC_MGQueryResult, "addTerm", "(Ljava/lang/String;I)V");
  assert(MID_addTerm != NULL);

  /* addEquivTerm(String term, String equivTerm, long match, long freq) */
  MID_addEquivTerm = (*j_env)->GetMethodID(j_env, JC_MGQueryResult, "addEquivTerm", "(Ljava/lang/String;Ljava/lang/String;JJ)V");
  assert(MID_addEquivTerm != NULL);

  /* setTotalDocs(long) */
  MID_setTotalDocs = (*j_env)->GetMethodID(j_env, JC_MGQueryResult, "setTotalDocs", "(J)V");
  assert(MID_setTotalDocs != NULL);

  /* clear(void) */
  MID_clearResult = (*j_env)->GetMethodID(j_env, JC_MGQueryResult, "clear", "()V");
  assert(MID_clearResult != NULL);
}


/* the java side MGSearchWrapper has a pointer to a C object - MGWrapperData
   initialise this and set the pointer 
*/
JNIEXPORT jboolean JNICALL
Java_org_greenstone_mg_MGSearchWrapper_initCSide(JNIEnv *j_env, jobject j_obj)
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


/* Clean up by unloading all cached indexes */
JNIEXPORT jboolean JNICALL
Java_org_greenstone_mg_MGSearchWrapper_unloadIndexData(JNIEnv* j_env, jobject j_obj)
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


/*******************************************
   do a query
 *******************************************/

/* do the actual query - the results are written to query_result held on the Java side */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_runQuery(JNIEnv *j_env, jobject j_obj,
					  jstring j_base_dir, jstring j_text_path,
					  jstring j_query)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);

  char* index_path;
  const char* base_dir;
  const char* text_path;
  query_data* qd;

  jobject result_ptr;
  char* query;
  int i, j;

  jthrowable exc;
  /* First of all, clear the previous result */
  /* The result to write to */
  result_ptr = (*j_env)->GetObjectField(j_env, j_obj, FID_query_result);
  assert(result_ptr != NULL);

  /* Clear any previous query results */
  (*j_env)->CallVoidMethod(j_env, result_ptr, MID_clearResult);
  exc = (*j_env)->ExceptionOccurred(j_env);
  if (exc) {
    (*j_env)->ExceptionDescribe(j_env);
    return;
  }

  /* Make sure an index has been specified */
  index_path = data->queryInfo->index;
  if (index_path == NULL) {
    return;
  }

  /* Obtain C versions of the two string parameters */
  base_dir = (*j_env)->GetStringUTFChars(j_env, j_base_dir, NULL);
  if (base_dir == NULL) {
    return;
  }
  text_path = (*j_env)->GetStringUTFChars(j_env, j_text_path, NULL);
  if (text_path == NULL) {
    (*j_env)->ReleaseStringUTFChars(j_env, j_base_dir, base_dir);
    return;
  }

  /* Load the appropriate index for satisfying this request */
  qd = loadIndexData((char*) base_dir, (char*) index_path, (char*) text_path);

  /* The C text strings are no longer needed */
  (*j_env)->ReleaseStringUTFChars(j_env, j_base_dir, base_dir);
  (*j_env)->ReleaseStringUTFChars(j_env, j_text_path, text_path);

  /* Check that the index was loaded successfully */
  if (qd == NULL) {
    return;
  }
  
  /* Remove anything hanging around from last time */
  FreeQueryDocs(qd);

  /* Obtain a C version of the query string */
  query = (char*) (*j_env)->GetStringUTFChars(j_env, j_query, NULL);
  if (query == NULL) {
    return;
  }
  printf("Searching for query \"%s\"...\n", query);

  /* Make sure the query isn't empty */
  if (strlen(query) == 0) {
    printf("Warning: Empty query.\n");
    return;
  }

  /* "Some" queries are done as ranked queries */
  if (data->defaultBoolCombine == 0) {
    RankedQueryInfo rqi;
    rqi.QueryFreqs = 1;  /* Use the frequency of each query term in the query - OK? */
    rqi.Exact = 1;  /* Perform exact ranking */
    rqi.MaxDocsToRetrieve = data->queryInfo->maxDocs;  /* Get only the desired number */
    rqi.MaxParasToRetrieve = rqi.MaxDocsToRetrieve;  /* OK? */
    /* we may need to get more paragraphs to get enough documents. I copied the following from mgquery. it seems to work, not sure why - kjdon */
    if (qd->id->ifh.InvfLevel == 3 && GetEnv ("maxparas")) {
      rqi.MaxParasToRetrieve = atoi (GetEnv ("maxparas"));
    }
    
    rqi.Sort = 1;  /* Sort the query terms by frequency before ranking */
    rqi.AccumMethod = 'L';  /* Use a list when accumulating (has bugs though...) */
    /* rqi.MaxAccums = -1; */ /* Use as many accumulators as necessary - CRASHES with list */
    rqi.MaxAccums = 100000;
    rqi.MaxTerms = -1;  /* Use all the query terms */
    /* rqi.StopAtMaxAccum = 0;*/  /* Don't care (using as many accumulators as necessary) */
    rqi.StopAtMaxAccum = 1;
    rqi.HashTblSize = 1000;  /* Don't care (not using a hash table) */
    rqi.skip_dump = NULL;  /* Don't dump skip information */

    /* RankedQuery() reads 'casefold' and 'stem' parameters from the environment */
    SetEnv("casefold", ((data->defaultStemMethod & 1) ? "on" : "off"), NULL);
    SetEnv("stem", ((data->defaultStemMethod & 2) ? "on" : "off"), NULL);

    RankedQuery(qd, query, &rqi);
  }
  /* "All" queries are done as boolean queries */
  else {
    BooleanQueryInfo bqi;
    bqi.MaxDocsToRetrieve = data->queryInfo->maxDocs;

    /* Had to add "words$o" to LIB_OBJS in mg/src/text/Makefile and recompile mg for this */
    BooleanQuery(qd, query, &bqi, data->defaultStemMethod);
  }

  /* Finished with the C query string */
  (*j_env)->ReleaseStringUTFChars(j_env, j_query, query);

  /* Check the query was processed successfully */
  if (qd->DL == NULL || qd->QTL == NULL || qd->TL == NULL) {
    return;
  }

  /* Record the total number of matching documents */
  (*j_env)->CallVoidMethod(j_env, result_ptr, MID_setTotalDocs, (jlong) qd->DL->num);
  exc = (*j_env)->ExceptionOccurred(j_env);
  if (exc) {
    (*j_env)->ExceptionDescribe(j_env);
    return;
  }

  /* Record the matching documents, but only the number requested */
  printf("Number of matching documents: %d\n", qd->DL->num);
  
  for (i = 0; (i < qd->DL->num && i < data->queryInfo->maxDocs); i++) {
    int doc_num = qd->DL->DE[i].DocNum;
    float doc_weight = qd->DL->DE[i].Weight;

#if defined(PARADOCNUM) || defined(NZDL)
    if (qd->id->ifh.InvfLevel == 3) {
      /* pararaph level, need to convert to doc level*/
      doc_num = GetDocNumFromParaNum(qd, doc_num);
    }
#endif
    
    
    /* Call the addDoc function (Java side) to record a matching document */
    (*j_env)->CallVoidMethod(j_env, result_ptr, MID_addDoc,
			     (jlong) doc_num, (jfloat) doc_weight);
    exc = (*j_env)->ExceptionOccurred(j_env);
    if (exc) {
      (*j_env)->ExceptionDescribe(j_env);
      return;
    }
  }

  /* Record the term information, if desired */
  if (data->queryInfo->needTermFreqs) {
    /* The following code is a lot more complicated than it could be, but it is necessary
       to compensate for an oddity in MG. */
    unsigned char** stemmed_terms = malloc(sizeof(unsigned char*) * qd->TL->num);

    printf("Number of terms: %d\n", qd->TL->num);
    printf("Number of query terms: %d\n", qd->QTL->num);

    /* Generate the stemmed form of each of the relevant terms */
    for (i = 0; i < qd->TL->num; i++) {
      u_char* raw_term = qd->TL->TE[i].Word;
      unsigned int term_length = raw_term[0];

      u_char* raw_stemmed_term = malloc(term_length + 1);
      unsigned int stemmed_term_length;

      /* Copy the term, and stem it */
      for (j = 0; j <= term_length; j++)
	raw_stemmed_term[j] = raw_term[j];
      stemmer(data->defaultStemMethod, qd->sd->sdh.stemmer_num, raw_stemmed_term);

      /* Allocate memory to store the stemmed term, and fill it */
      stemmed_term_length = raw_stemmed_term[0];
      stemmed_terms[i] = malloc(stemmed_term_length + 1);
      assert(stemmed_terms[i] != NULL);
      strncpy(stemmed_terms[i], &(raw_stemmed_term[1]), stemmed_term_length);
      stemmed_terms[i][stemmed_term_length] = '\0';
    }

    /* Record every query term, along with their equivalent terms */
    for (i = 0; i < qd->QTL->num; i++) {
      u_char* raw_query_term = qd->QTL->QTE[i].Term;
      unsigned int query_term_length = raw_query_term[0];
      unsigned char* query_term;
      jstring j_query_term;

      u_char* raw_stemmed_query_term = malloc(query_term_length + 1);
      unsigned int stemmed_query_term_length;
      unsigned char* stemmed_query_term;

      /* Allocate memory to store the query term, and fill it */
      query_term = malloc(query_term_length + 1);
      assert(query_term != NULL);
      strncpy(query_term, &(raw_query_term[1]), query_term_length);
      query_term[query_term_length] = '\0';

      /* Allocate a new jstring for the query term */
      j_query_term = (*j_env)->NewStringUTF(j_env, query_term);
      assert(j_query_term != NULL);

      /* Call the addTerm function (Java side) to record the query term */
      (*j_env)->CallVoidMethod(j_env, result_ptr, MID_addTerm,
			       j_query_term, (jint) data->defaultStemMethod);
      exc = (*j_env)->ExceptionOccurred(j_env);
      if (exc) {
	(*j_env)->ExceptionDescribe(j_env);
	return;
      }

      /* Copy the query term, and stem it */
      for (j = 0; j <= query_term_length; j++)
	raw_stemmed_query_term[j] = raw_query_term[j];
      stemmer(data->defaultStemMethod, qd->sd->sdh.stemmer_num, raw_stemmed_query_term);

      /* Allocate memory to store the stemmed query term, and fill it */
      stemmed_query_term_length = raw_stemmed_query_term[0];
      stemmed_query_term = malloc(stemmed_query_term_length + 1);
      assert(stemmed_query_term != NULL);
      strncpy(stemmed_query_term, &(raw_stemmed_query_term[1]), stemmed_query_term_length);
      stemmed_query_term[stemmed_query_term_length] = '\0';

      /* Find all the terms equivalent to the query term */
      for (j = 0; j < qd->TL->num; j++) {
	/* Check if the stemmed query term matches the stemmed term */
	if (strcmp(stemmed_query_term, stemmed_terms[j]) == 0) {
	  u_char* raw_term = qd->TL->TE[j].Word;
	  unsigned int term_length = raw_term[0];
	  unsigned char* term;
	  jstring j_term;

	  /* Allocate memory to store the query term, and fill it */
	  term = malloc(term_length + 1);
	  assert(term != NULL);
	  strncpy(term, &(raw_term[1]), term_length);
	  term[term_length] = '\0';

	  /* Allocate a new jstring for the query term */
	  j_term = (*j_env)->NewStringUTF(j_env, term);
	  assert(j_term != NULL);

	  /* Call the addEquivTerm function (Java side) to record the equivalent term */
	  (*j_env)->CallVoidMethod(j_env, result_ptr, MID_addEquivTerm,
				   j_query_term, j_term,
				   (jlong) qd->TL->TE[j].WE.doc_count,
				   (jlong) qd->TL->TE[j].WE.count);
	  exc = (*j_env)->ExceptionOccurred(j_env);
	  if (exc) {
	    (*j_env)->ExceptionDescribe(j_env);
	    return;
	  }
	}
      }
    }
  }
}


/*******************************************
   set query options
 *******************************************/

/* Turn casefolding on or off */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_setCase(JNIEnv *j_env, jobject j_obj, 
					 jboolean j_on)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);

  if (j_on) {
    data->defaultStemMethod |= 1;
  } else {
    data->defaultStemMethod &= 0xe;
  }
}


/* Turn stemming on or off */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_setStem(JNIEnv *j_env, jobject j_obj, 
					 jboolean j_on)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);

  if (j_on) {
    data->defaultStemMethod |= 2;
  } else {
    data->defaultStemMethod &= 0xd;
  }
}


/* Set the maximum number of documents to return from a query */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_setMaxDocs(JNIEnv *j_env, jobject j_obj, 
					    jint j_max)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);
  data->queryInfo->maxDocs = j_max;
}

/* set the maximum number of numeric to split*/
JNIEXPORT void JNICALL 
Java_org_greenstone_mg_MGSearchWrapper_setMaxNumeric (JNIEnv *j_env, 
						 jobject j_obj, 
						 jint j_max) {

  char text[20];
  char* maxnumeric;
  sprintf(text,"%d",j_max); 
  maxnumeric = text;  
  SetEnv("maxnumeric",maxnumeric, NULL);
}


/* Turn term frequency recording on or off */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_setReturnTerms(JNIEnv *j_env, jobject j_obj, 
						jboolean j_on)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);
  data->queryInfo->needTermFreqs = j_on;
}


/* Choose MG index to search */
JNIEXPORT void JNICALL
Java_org_greenstone_mg_MGSearchWrapper_setIndex(JNIEnv *j_env, jobject j_obj,
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


/* Choose boolean AND or boolean OR queries */
JNIEXPORT void JNICALL 
Java_org_greenstone_mg_MGSearchWrapper_setMatchMode(JNIEnv *j_env, jobject j_obj,
					      jint j_mode)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);
  data->defaultBoolCombine = j_mode;
}


/* Get a text representation of the current parameter values */
JNIEXPORT jstring JNICALL
Java_org_greenstone_mg_MGSearchWrapper_getQueryParams(JNIEnv *j_env, jobject j_obj)
{
  MGWrapperData* data = (MGWrapperData*) (*j_env)->GetIntField(j_env, j_obj, FID_mg_data);
  char result[512];  /* Assume this is big enough */

  /* Print the data to a character array */
  sprintf(result, "Query params:\nindex\t\t%s\ncasefold\t%d\nstem\t\t%d\nquery type\t%s\nmax docs\t%ld\n",
	  (data->queryInfo->index == NULL ? "<none loaded>" : data->queryInfo->index),
	  (data->defaultStemMethod & 1),
	  (data->defaultStemMethod & 2),
	  (data->defaultBoolCombine == 1 ? "all" : "some"),
	  (data->queryInfo->maxDocs));

  /* Convert to a jstring, and return */
  return (*j_env)->NewStringUTF(j_env, result);
}

