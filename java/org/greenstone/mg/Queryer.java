/*
 *    Queryer.java
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
import java.io.BufferedReader;
import java.io.InputStreamReader;

/** Queryer - java port of c++ Queryer 
 * - uses MGWrapper to access mg
 * only has get document and search - full text browse not implemented
 *
 * run as:
 * java org.greenstone.mg.Queryer <basedir> <indexdir> <textdir>
 *@see MGWrapper
 */
public class Queryer {
  
    /** outputs to std out a help message describing the commands used by 
     * Queryer 
     */
    public void printHelp()
    {
	System.out.println( "commands available are:\n"+
			    "\t.q\t\tquit\n"+
			    "\t.h\t\tprint this help message\n"+
			    "\t.d\t\tprint the current query parameter settings\n" +
			    "\t.i<index>\tsearch index <index>\n"+
			    "\t.t0/.t1\t\tquery type some/all\n"+
			    "\t.c0/.c1\t\tcasefolding off/on\n"+
			    "\t.s0/.s1\t\tstemming off/on\n"+
			    "\t.o0/.o1\t\tshort output off/on\n"+
			    "\t.m<num>\t\tset max docs to return to num\n\n"+
			    "\t.p<docnum>\tprint document docnum\n"+
			    "\t<query string>\tdo a query\n");
    }


    public static void main(String[] args)
    {
	if (args.length != 3) {
	    System.out.println("Usage: java org.greenstone.mg.Queryer <basedir> <indexdir> <textdir>");
	    return;
	}

	Queryer self = new Queryer();

	String base_dir = args[0];
	String index_path = args[1];
	String text_path = args[2];

	// the jni classes to access mg stuff
	MGSearchWrapper searcher = new MGSearchWrapper();
	MGRetrieveWrapper retriever = new MGRetrieveWrapper();
	searcher.setIndex(index_path);
	retriever.setIndex(index_path);

	System.out.println("Welcome to Java Queryer :-)");
	self.printHelp();

	BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
	boolean shortOutput = false;

	String command; // user input
	char x; // the command letter
	String data = null; // any auxiliary input to a command
	while (true) {
	    System.out.print(">"); // the prompt
            try {
                command = br.readLine();
                command = command.trim();
		if (command.startsWith(".")) {
		    // a system command
		    x = command.charAt(1);
		    if (command.length() > 2) {
			data = command.substring(2);
			data = data.trim();
		    }

		    switch (x) {
		    case 'q': // clean up and exit
			searcher.unloadIndexData();
			retriever.unloadIndexData();
			return;
		    case 'h': // print help message
			self.printHelp();
			break;
		    case 'd': // print query param settings
			String info = searcher.getQueryParams();
			System.out.println(info);
			break;
		    case 'p': // print doc
			int docnum = Integer.parseInt(data);
			String doc = retriever.getDocument(base_dir, text_path, docnum);
			System.out.println(doc);
			break;
		    case 'm': //match docs
			int match = Integer.parseInt(data);
			searcher.setMaxDocs(match);
			break;
		    case 's': // set stem on/off
			int stem = Integer.parseInt(data);
			if (stem==0 ){
			    searcher.setStem(false);
			} else if(stem==1) {
			    searcher.setStem(true);
			} else {
			    System.err.println("Error: stem should be 0 or 1");
			}
			break;
		    case 'c': // set case on/off
			int casef = Integer.parseInt(data);
			if (casef==0) {
			    searcher.setCase(false);
			} else if (casef==1) {
			    searcher.setCase(true);
			} else {
			    System.err.println("Error: case should be 0 or 1");
			}
			break;
		    case 'i': // set index
			searcher.setIndex(data);
			break;
		    case 't': // set query type some/all
			int type = Integer.parseInt(data);
			if (type==0 || type==1) {
			    searcher.setMatchMode(type);
			} else {
			    System.err.println("Error: type should be 0 (some) or 1 (all)");
			}
			break;
		    case 'o': // set output short/long
			int output = Integer.parseInt(data);
			if (output==0) {
			    shortOutput = false;
			} else if (output==1) {
			    shortOutput = true;
			} else {
			    System.err.println("Error: output should be 0 or 1");
			}
			break;
		    }
		}
		else {
		    // a query
		    searcher.runQuery(base_dir, text_path, command);
		    MGQueryResult res = searcher.getQueryResult();
		    System.out.println("(Java) Matching documents: " + res.getTotalDocs());
		    if (shortOutput) {
			System.out.println(res.toShortString());
		    } else {
			System.out.println(res.toString());
		    }
		}

	    } catch (Exception e) {
		System.out.println("Queryer error: "+e.getClass() + " "+e.getMessage());
		e.printStackTrace();
	    }
	}
    }
}
