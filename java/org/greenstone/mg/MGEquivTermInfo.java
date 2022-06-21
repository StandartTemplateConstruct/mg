/*
 *    MGEquivTermInfo.java
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


/** EquivTermInfo 'struct' class for a search result from mg
 *  used by MGQueryResult
 *
 * @see MGQueryResult
 */
public class MGEquivTermInfo
{
    /** the term itself */
    public String term_ = null;

    /** the number of documents containing this term */
    public long match_docs_ = 0;

    /** overall term freq - word level */
    public long term_freq_ = 0;


    public MGEquivTermInfo(String term, long match, long freq)
    {
	term_ = term;
	match_docs_ = match;
	term_freq_ = freq;
    }


    public String toString()
    {
	String result = "\"" + term_ + "\"";
	result += " (" + match_docs_ + "/" + term_freq_ + ")";
	return result;
    }
}
