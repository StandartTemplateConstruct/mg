/*
 *    MGQueryResult.java
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


import java.util.Vector;


/** a java version of QueryResult for mg
 *
 * contains get methods for the java side, and simple
 * set methods to be called from c side
 *
 */
public class MGQueryResult
{
    /** the list of MGDocInfo */
    protected Vector docs_ = null;
    /** the list of MGTermInfo */
    protected Vector terms_ = null;
    /** the total number of docs found - not likely to be the size of docs_ */
    protected long total_num_docs_ = 0;


    MGQueryResult() {
	docs_ = new Vector();
	terms_ = new Vector();
    }


    /** clear the info from the last query
	should be called before setting any new docs/terms */
    public void clear() {
	total_num_docs_ = 0;
	docs_.clear();
	terms_.clear();
    }

    public boolean isClear() {
	return (total_num_docs_ == 0 && docs_.isEmpty() && terms_.isEmpty());
    }

    /** returns the result as a String - useful for printing out results */
    public String toString()
    {
	String result = "";
	result += "docs (ranks): ";
	for (int i = 0; i < docs_.size(); i++) {
	    if (i > 0) result += ", ";
	    result += ((MGDocInfo) docs_.elementAt(i)).toString();
	}
	result += "\nterms: ";
	for (int i = 0; i < terms_.size(); i++) {
	    if (i > 0) result += ", ";
	    result += ((MGTermInfo) terms_.elementAt(i)).toString();
	}
	result += "\nactual number of docs found = " + total_num_docs_;
	return result;
    }


    /** a shorter representation - just terms and total docs - not the 
	individual docnums and ranks */
    public String toShortString() {
	String result = "";
	result += "\nterms: ";
	for (int i = 0; i < terms_.size(); i++) {
	    if (i > 0) result += ", ";
	    result += ((MGTermInfo) terms_.elementAt(i)).toString();
	}
	result += "\nactual number of docs found = "+total_num_docs_;
	return result;
    }


    // set methods used by c++ code

    public void setTotalDocs(long num) {
	total_num_docs_=num;
    }


    public void addDoc(long doc, float rank) {
	MGDocInfo doc_info = new MGDocInfo(doc, rank);
	///ystem.out.println("(Java) Added doc " + doc_info);

	docs_.add(doc_info);
    }


    public void addTerm(String term, int stem)
    {
	MGTermInfo ti = new MGTermInfo();
	ti.term_ = term;
	ti.stem_method_ = stem;
	terms_.add(ti);
	///ystem.out.println("(Java) Added term " + ti);
    }


    public void addEquivTerm(String term, String equivTerm,
			     long match, long freq)
    {
	// Find the term to add the equivalent to
	MGTermInfo ti = null;
	for (int i = (terms_.size() - 1); i >= 0; i--) {
	    ti = (MGTermInfo) terms_.elementAt(i);
	    // Found
	    if (ti.term_ == term) {
		break;
	    }
	}

	if (ti == null) {
	    System.err.println("Internal error: No term exists to add to.\n");
	}
	else {
	    ti.addEquivTerm(equivTerm, match, freq);
	    ///ystem.out.println("(Java) Added equivalent term " + equivTerm + ", match: " + match + ", freq: " + freq);
	}
    }


    // Get methods for the java side - GS2MGSearch.java, GS2MGRetrieve.java
    public Vector getDocs() {
	return docs_;
    }


    public Vector getTerms() {
	return terms_;
    }


    public long getTotalDocs() {
	return total_num_docs_;
    }
}

