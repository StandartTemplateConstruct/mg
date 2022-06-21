/*
 *    MGRetrieveWrapper.java
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
package org.greenstone.mg;


/** java wrapper class for access to mg in C
 *
 * the native side implemented in MGWrapperImpl.c
 * uses MGQueryResult to hold the result of a query.
 * the result of a getDocument is a String
 * uses the jni
 *
 *@see MGQueryResult
 */
public class MGRetrieveWrapper {
    /** the query result, filled in by runQuery */
//    protected MGQueryResult mg_query_result_ = null;
    
    /** pointer to c MGWrapperData class - cached indexData and queryInfo */
    protected long mg_data_ptr_ = 0;
    
    static {
        System.loadLibrary ("mgretrievejni"); 
        initIDs ();
    }
    
    public MGRetrieveWrapper () { 
//        mg_query_result_ = new MGQueryResult ();
        initCSide ();
    }
    
    /** initialises field and method IDs for java side to enable access on C side */
        private static native void initIDs ();
    
    /** initialises the mg_data_ptr_ */
    private native boolean initCSide ();

    /** unloads the data */
    public native boolean unloadIndexData ();
    
    /** sets the index to search - default is 'dtx' */
    public native void setIndex (String index);

    /** returns a document: number docnum at level level
     * the base_dir and text_path paths should join together to provide
     * the absolute location of the mg text files eg ..../index/text/demo
     * returns the doc in utf-8
     */
    public native String getDocument (String base_dir, String text_dir, long docnum);
    
    
//    the following is for search service // query param methods
//    
//    /** if on=true, sets default casefolding on - it's off by default */
//    public native void setCase (boolean on);
//    /** if on=true, sets default stemming on - it's off by default */
//    public native void setStem (boolean on);
//    /** default is 50 */
//    public native void setMaxDocs (int num);
//    /** if on=true, a query returns term freq info - default is on */
//    public native void setReturnTerms (boolean on);
//    /** sets the default boolean operator - AND(=1)/OR(=0) */
//    public native void setMatchMode (int mode);
//    
//    /** returns a string with all the current query param settings */
//    public native String getQueryParams ();
//    
//    /** sets maxnumeric */
//    public native void setMaxNumeric (int maxnumeric);
//    
//    /** actually carry out the query.
//     * Use the set methods to set query results.
//     * Writes the result to query_result.
//     * - maintains state between requests as can be slow
//     * base_dir and index_path should join together to provide
//     * the absolute location of the mg index files eg ..../index/dtx/demo
//     * base_dir must end with a file separator (OS dependant)
//     */
//    public native void runQuery (String base_dir, String text_dir, String query_string);
//    
//    
//    /** get the result out of the wrapper */
//    public MGQueryResult getQueryResult () {
//        return mg_query_result_;
//    }
}
