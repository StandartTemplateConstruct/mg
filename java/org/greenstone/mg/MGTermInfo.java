/*
 *    MGTermInfo.java
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


/** TermInfo 'struct' class for a search result from mg
 *  used by MGQueryResult
 *
 * @see MGQueryResult
 */
public class MGTermInfo
{
    /** the term itself */
    public String term_ = null;

    /** the stem and casefold method used
	0 = none
	1 = casefold only
	2 = stem only
	3 = casefold and stem */
    public int stem_method_ = 0;

    /** list of stemmed and casefolded equivalent terms - if stem_method_ is non-zero 
      - Vector of Strings */
    public Vector equiv_terms_ = null;


    public MGTermInfo()
    {
	equiv_terms_ = new Vector();
    }


    public void addEquivTerm(String equivTerm, long match, long freq)
    {
	equiv_terms_.add(new MGEquivTermInfo(equivTerm, match, freq));
    }


    /** output the class as a string */
    public String toString()
    {
	String result = "\"" + term_ + "\"";
	result += " stem(" + stem_method_ + ")";
	result += " equiv terms(";
	for (int i = 0; i < equiv_terms_.size(); i++) {
	    if (i > 0) result += ", ";
	    result += equiv_terms_.elementAt(i);
	}
	result += ")";
	return result;
    }
}
