/*
 *    MGPassesWrapper.java
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


/** java wrapper class for access to gs3_mg_passes in C
 *
 * the native side implemented in MGPassesWrapperImpl.c
 */

public class MGPassesWrapper
{
    static {
	System.loadLibrary("mgpassjni");
	initIDs();
    }
    
    static final public char INVF_LEVEL_1 = '1';
    static final public char INVF_LEVEL_2 = '2';
    static final public char INVF_LEVEL_3 = '3';
    
    static final public int TEXT_PASS_1 = 0;
    static final public int TEXT_PASS_2 = 1;
    static final public int INDEX_PASS_1 = 2;
    static final public int INDEX_PASS_2 = 3;
    static final public int SPECIAL_PASS = 4;

    static final public int NO_STEM_OR_CASE = 0;
    static final public int CASE_ONLY = 1;
    static final public int STEM_ONLY = 2;
    static final public int STEM_AND_CASE = 3;

    static final public String STEMMER_ENGLISH = "english";
    static final public String STEMMER_FRENCH = "french";
    static final public String STEMMER_LOVIN = "lovin";
    static final public String STEMMER_SIMPLE_FRENCH = "simple-french";

    static final private char END_OF_DOCUMENT = (char) 2;

    public MGPassesWrapper() {
	initCSide();
    }

    /** initialise the pass through the documents */
    public native boolean init();
    
    /** add a pass declaration */
    public void addPass(int pass) {
	switch (pass) {
	case TEXT_PASS_1:
	    addPass('T','1');
	    break;
	case TEXT_PASS_2:
	    addPass('T','2');	    
	    break;
	case INDEX_PASS_1:
	    addPass('I','1');
	    break;
	case INDEX_PASS_2:
	    addPass('I','2');
	    break;
	case SPECIAL_PASS:
	    addPass('S','1');
	    break;
	}
    }
    /** set the base path */
    public native void setBasePath(String basepath);
   /** set the file name */
    public native void setFileName(String filename);

    public native void setStemOptions(String stemmer_type, int stem_method);

    public native void setInvfLevel(char level);

    /** Specify  the size of the document buffer in kilobytes.
	If any document is larger than  bufsize,  the  program
	will  abort with an error message. 
     */
    public native void setBufferSize(long bufsize);
    
    /** Maximum amount of memory to use for  the index  pass-2  file
	inversion  in  megabytes.
    */
    public native void setInversionMemLimit(int limit);

    /** If true, treat   SGML  tags  as  non-words  when  building  the
	inverted file.
    */
    public native void ignoreSGMLTags(boolean ignore);
    
    /** if mg_passes fails, the document that caused teh failure will be
	output to teh trace file or STDERR. 
    */
    public native void dumpFailedDocument(boolean dump);
    
    /** output statistics on the compression performance to  a  file  
	called  *.compression.stats.    frequency  specifies  the  interval
	(in kilobytes of source text) between outputting each line of 
	statistics. 
    */
    public native void outputCompStats(int frequency);

    /** activate tracing, a line will be output every tracepos input bytes */
    public native void enableTracing(int tracepos);
    /** process a Greenstone document, which may consist of many MG documents (seeparated by ^B */
    public boolean processDocument(String docs_text) {
	// need -1 in the following to keep empty strings at the end
	String [] docs = docs_text.split(String.valueOf(END_OF_DOCUMENT), -1);
	System.err.println("GS document split into "+docs.length+" mg documents");
	for (int i=0; i<docs.length; i++) {
	    try {
		processMGDocument(docs[i].getBytes("UTF-8"));
	    } catch (Exception e) {
		e.printStackTrace();
	    }
	}
	return true;
    }
   
    /** finalise the pass through the documents */
    public native boolean finish();

    /** get the exit value once finished */
    public native int exitValue();
 
    /** initialises field and method IDs for java side to enable access on C side */
    private static native void initIDs();

    /** initialises any C side stuff */
    private native boolean initCSide();

    private native void addPass(char pass_type, char pass_num);

    /** process a MG document */
    private native boolean processMGDocument(byte[] text);

}
