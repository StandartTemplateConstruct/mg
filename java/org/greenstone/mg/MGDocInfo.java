/*
 *    MGDocInfo.java
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


/** DocInfo 'struct' class for a search result from mg
 *  used by MGQueryResult
 *
 * @See MGQueryResult
 */
public class MGDocInfo
{
    public long num_ = 0;
    public float rank_ = 0;

    public MGDocInfo(long doc, float rank)
    {
	num_ = doc;
	rank_ = rank;
    }

    public String toString()
    {
	return "" + num_ + " (" + rank_ + ")";
    }
}
