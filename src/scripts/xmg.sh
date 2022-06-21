#!/usr/local/bin/wish -f
# Edit the line above to point to your version of wish
#
###############################################################################
#
# xmg -- X interface to the mg information retrieval system
# Copyright (C) 1994  Bruce McKenzie, Ian Witten and Craig Nevill-Manning
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#       @(#)xmg.sh	1.4 25 Mar 1994
#
###############################################################################
#
# "xmg" is an X interface to the mg information retrieval system.
#
# Type a query in the "Query" box.
# The first line or so of each matching document is listed.
# Click on any line to show the complete document.
# Shift-click to show the document in a new window.
# Press Next and Previous to cycle through the list of documents (Shift works
#   here too). 
# If you change the query, existing windows continue to refer to the original
#   query. 
# 
# You can select either Boolean or Ranked queries.
# Boolean queries: use "&" for AND, "|" for OR, "!" for NOT, and parentheses
# for grouping 
# Ranked queries:  - documents are listed in order of relevance; a weight is
# 		     shown alongside each 
#                  - weights are normalized to 100 for the most relevant
#		     document 
#                  - choose Approximate Ranking for greater retrieval speed. 
# 
# In document windows (other than the main window): 
#		Typing 'n' = next document
#		Typing 'N' = next document in new window
#		Typing 'p' = previous document
#		Typing 'P' = previous document in new window
#		Typing 'q' = close window
#
###############################################################################

# Date: 13 March 1994
# Authors: Bruce McKenzie, Ian Witten and Craig Nevill-Manning

###############################################################################
# Customizing xmg
###############################################################################

# To override the default font options, use RESOURCE_MANAGER properties like
# the following in your .Xdefaults file
#
#	xmg.textfont: fixed
#	xmg.buttonfont: fixed
#
# To override the default query type of ranked, use
#
#	xmg.querytype: boolean
#
# To override the default collection name (the first one listed by /bin/ls from
# your MGDATA environment variable), use
#
#	xmg.collection: mail93

###############################################################################
# Global variables
###############################################################################

#   genuine constants:
#
#	env		the environment variables
#	Terminator	terminator for mgquery
#	MaxWindowHeight	the maximum height permitted for a document window
#	ListBoxLength	the number of items in the document list
#	HeadLength	the length document head displayed in the list box
#	ImageString	the string at the start of a document associated
#			with an image
#
#   things that rarely get changed:
#
#	querytype	the current query type (boolean, ranked, approx-ranked)
#	booleanquery	says whether the query is boolean or ranked
#	approxquery	says whether ranking is to be approximate or not
#	collection	the name of the document collection being used
#	fileId		file identifer for a pipe to the mgquery process
#       errorOccurred	flag to say that there has been an I/O error on the
#			current collection
#
#   things that reflect items in the main panel
#
#	query		the current query
#	hits		number of hits for the current query
#	display		shell command to display the image associated with
#			the last document retrieved
#	scrolledto	document prefixes have been obtained for all documents
#			up to this position in the document list
#
#   static variable needed in just one procedure
#
#	top_rank	weight associated with the first document retrieved
#	windowname	unique id for each document window

###############################################################################
# High-level procedures
###############################################################################

proc SaveDocument {w} {
  set file [FSBox "Save document to file:"]

  if {$file != ""} {
    if [file exists $file] {
      toplevel .r
      wm title .r "\n"
      .r configure -borderwidth 2 -relief raised 
      focus .r
      label .r.message -text "The file already exists"
      button .r.replace -text "Replace" -width 10 -height 2 \
          -command "destroy .r; WriteDocument $file \"w\" $w" 
      button .r.append -text "Append" -width 10 -height 2 \
          -command "destroy .r; WriteDocument $file \"a\" $w"
      button .r.cancel -text "Cancel" -command {destroy .r} -width 10 -height 2
      pack .r.message -side top -pady 10 -padx 20
      pack .r.replace .r.append .r.cancel -side left -pady 10 -padx 20

    } else {
       WriteDocument $file "w" $w
    }
  }
}

proc Dialog {text command} {
      if {![winfo exists .r] } { toplevel .r }
      wm title .r "\n"
      .r configure -borderwidth 2 -relief raised 
      focus .r
      label .r.message -text $text
      button .r.ok -text "OK" -width 10 -height 2 \
          -command "destroy .r $command"
      pack .r.message -side top -pady 10 -padx 20
      pack .r.ok -side bottom -pady 10 -padx 20
}

proc WriteDocument {file mode w} {
    if {[catch {set f [open $file $mode]}] == 0} {
        if [winfo exists $w] {
            puts $f [$w get 0.0 end]
        } else {
            Dialog "Unable to save text" ""
        }
        flush $f
        close $f
    } else {
        Dialog "Unable to open file for writing" "; SaveDocument $w"
    }
}

# Send a query to mg and enter the response in the document list
#
proc sendQuery {window} {
    global query hits querytype errorOccurred scrolledto

    .m.t.q.retrieved.label configure -foreground Gray
    .m.t.q.retrieved.hits configure -foreground Gray

    sendto ".set query \"$querytype\""
    sendto ".set mode \"docnums\""
    foreach f "sdocnums sdocs srank" { $window.$f delete 0 end }
    disableButton .m.d.buttons.previous
    disableButton .m.d.buttons.next
    if {[string index $query 0] == "." || [string length $query] > 255} {
        set doclist ""
    } else { set doclist [sendandget $query] }

    if {$errorOccurred} {
        enterText .m.d.text [mgqueryFailure]
        .m.t.q.retrieved.label configure -foreground Black
        .m.t.q.retrieved.hits configure -foreground Black 
        update
        return
    }
    sendto ".set query \"DocNums\""
    sendto ".set mode  \"text\""
    .m.t.q.retrieved.label configure -foreground Black
    .m.t.q.retrieved.hits configure -foreground Black 
    set hits [llength $doclist]
    set i 0
    global top_rank

    scan [lindex $doclist 0] "%d %f %d" d rank size
    set top_rank $rank
    if {$top_rank == 0} {set top_rank 1}

    while {$i < $hits} {
	scan [lindex $doclist $i] "%d %f %d" d rank size
        if {$rank == "Inf"} {set rank 1}
	if {$querytype == "boolean"} {
	    $window.srank insert end ""
	} else {
	    $window.srank insert end \
		[format "%3.0f" [expr $rank / $top_rank * 99 + 1]]
	}
	$window.sdocnums insert end [format " %d" $d]
	incr i
    }

    set scrolledto 0
    scroll $window 0
    bind $window.sdocs <ButtonRelease-1> \
	"showDocument $window.sdocs %y .m.d \"$doclist\" \"$query\""
    bind $window.sdocs <Shift-ButtonRelease-1> \
	"showDocument $window.sdocs %y New \"$doclist\" \"$query\""
    enterText .m.d.text ""
}

# Scrolling routine for the document list
#
proc scroll {window n} {
    global hits scrolledto ListBoxLength Terminator fileId ImageString

    $window.sdocnums yview $n
    $window.sdocs yview $n
    $window.srank yview $n
    set last $n
    if {$last <= 0} {set last 0}
    incr last $ListBoxLength
    if {$last > $hits} {set last $hits}

#    .m config -cursor {watch}
#    update
    sendto ".set mode \"heads\""
    sendto ".set terminator \"\""
    while {$scrolledto < $last} {
	if {[scan [$window.sdocnums get $scrolledto] "%d" d] == 1} {
            sendto $d
            gets $fileId line
            set i [string first " " $line]
            if {$i != -1} {set line [string range $line [expr $i + 1] end]}
            if {[string compare $ImageString [string range $line 0 9]] == 0} {
                set line [getImageDocumentHead $d]
            }
	    $window.sdocs insert $scrolledto $line
	}
	incr scrolledto
    }
    sendto ".set terminator \"$Terminator\\n\""
    sendto ".set mode text"
#   .m config -cursor {}
}

proc getImageDocumentHead {d} {
    global HeadLength ImageString fileId

    sendto ".set heads_length 1000"
    sendto $d
    gets $fileId line

    set line [string range $line 11 end]

    set s [string first $ImageString $line]
    if {$s != -1} {
        incr s 11
        set line [string range $line $s [expr $s + $HeadLength]]
    }
  
    sendto ".set heads_length $HeadLength"

    return "* $line"
}

# Show document from the document list
#
proc showDocument {window y newWindow doclist query} {
    showThisDocument [$window nearest $y] $doclist $newWindow $query
}

proc adjacentDocument {index doclist window q button} {
    global ListBoxLength

    showThisDocument $index $doclist $window $q

    if {$window == ".m.d"} {
      set first [.m.t.q.r.sdocs nearest 0]
      if {$index < $first || $index >= $first + $ListBoxLength} {
          scroll .m.t.q.r [expr $index - $ListBoxLength / 2]
      }
      .m.t.q.r.sdocs select from $index
    } 
}


# Show document with this index in the document list
#
proc showThisDocument {index doclist window q} {
    global collection MaxWindowHeight display windowname

    set document ""
    scan [lindex $doclist $index] "%d %f" document rank
    if {$document == ""} { return }
    set textlist [sendandget $document]
	
    if {[string length $display] != 0} { exec csh -c $display >& /dev/null & }
	
# throw away the first line
    set text [join [lrange $textlist 1 end] "\n"]

# if necessary, create a new window for this document
    if {$window == "New"} {
        .m.t.q.r.sdocs select clear
	set window .m.document$windowname
        incr windowname
        set existing ""
        set raised 0
        foreach w [lrange [winfo children .m] 2 end] {
           if {[lindex [$w.docnum configure -text] 4] == $index && \
               [lindex [$w.query configure -text] 4] == $q} {
              if {$raised} {
                 closeDocumentWindow $w
              } else {
                  raise $w
		  getFocus $w
                  set window $w
                  set raised 1
              }
           }
        }
  
        if {!$raised} {
   	    toplevel $window
            $window configure -borderwidth 2 -relief raised
            wm protocol $window WM_TAKE_FOCUS "getFocus $window"
	    getFocus $window
	    makeDocumentPanel $window
            bind $window q "$window.buttons.close invoke"
            label $window.docnum -text $index
            label $window.query -text $q
       }
    }

# configure the window for this document
    set rows [expr [llength $textlist] - 1]
    if {$window != ".m.d"} {
	wm title $window "$collection \"$q\""
	wm iconname $window "$document"
	$window.text configure -height $rows
	if {$rows > $MaxWindowHeight} {
	    $window.text configure -height $MaxWindowHeight
	}
    }

# show the document text in the window
    enterText $window.text $text

# configure the window's Previous and Next buttons
    if {$index == 0} {
        disableButton $window.buttons.previous
    } else {
	set newindex [expr $index-1]
	bindBoth $window previous $newindex $doclist $q
        bind $window p [bind $window.buttons.previous <1>]
        bind $window P [bind $window.buttons.previous <Shift-1>]
    }
    if {$index == [expr [llength $doclist]-1]} {
	disableButton $window.buttons.next 
    } else {
	set newindex [expr $index+1]
	bindBoth $window next $newindex $doclist $q
        bind $window n [bind $window.buttons.next <1>]
        bind $window N [bind $window.buttons.next <Shift-1>]
    }

    if {$window != ".m.d"} { $window.docnum configure -text $index }

    if {[string length $display] != 0} { 
        set on "{
            if \[winfo exists $window\] \{
                 $window.buttons.image configure \
                   -foreground Black
                 update\}
        }"
        set off "{
            if \[winfo exists $window\] \{
                 $window.buttons.image configure \
                   -foreground [lindex [.m.d configure -background] 4]
                 update\}
        }"
        set t 0
        while {$t < 5} {
            after [expr $t * 1000] eval $on
            after [expr $t * 1000 + 500] eval $off
            incr t
        }   
    }
}

# Bind both Button-1 and Shift-Button-1 commands
#
proc bindBoth {window button index doclist query} {
    $window.buttons.$button configure -state normal
    bind $window.buttons.$button <Button-1> \
	"adjacentDocument $index \"$doclist\" $window \"$query\" $button"
    bind $window.buttons.$button <Shift-Button-1> \
	"adjacentDocument $index \"$doclist\" New \"$query\" $button"
}

# Change the type of query
#
proc changeQuerytype {name element op} {
    global querytype booleanquery approxquery

    if {$booleanquery} {
	.m.t.b.type.approx configure -state disabled
	.m.t.q.retrieved.weight configure \
             -foreground [lindex [.m configure -background] 4]
	set querytype boolean
    } else {
	.m.t.b.type.approx configure -state normal
	.m.t.q.retrieved.weight configure -foreground Black
	if {$approxquery} {
	    set querytype "approx-ranked"
	} else {
	    set querytype "ranked"
	}
    }
}

proc enterText {window text} {
    $window configure -state normal
    $window delete 0.0 end
    $window insert 0.0 $text
    $window configure -state disabled
}

###############################################################################
# IO and initialization
###############################################################################

# Send a message to mg and return the reply
#
proc sendandget {message} {
    global fileId Terminator display errorOccurred ImageString
    set display ""

    .m config -cursor {watch}
    update idletasks
    set reply ""
    sendto "$message\n"
    if {$message != "" && !$errorOccurred} {
        gets $fileId line
    	while {[string compare $Terminator $line] != 0} {
            lappend reply $line;
            gets $fileId line
        }

        if {[string compare $ImageString [lindex $reply 1]] == 0 } {
          if {[string compare "MGDATA" [string range [lindex $reply 3] 0 5]] == 0} {
             set display [string range [lindex $reply 3] 7 end];
          }
          set reply [lrange $reply 4 end]
        }
    }

    .m config -cursor {}
    return $reply
}
    
# Send a message to mg
#
proc sendto {message} {
    global fileId errorOccurred

    puts $fileId $message
    set errorOccurred [catch {flush $fileId}]
}

# Reinitialise mgquery to work with a new document collection
#
proc reInitMG {c} {
    global fileId
    catch {close $fileId}
    initMG $c
}

# Initialize mgquery to work with a specified text collection
#
proc initMG {c} {
    global query hits fileId collection Terminator env errorOccurred
    global HeadLength

    set collection $c
    set query ""
    set hits 0
    set errorOccurred 0

    foreach f "sdocs srank" { .m.t.q.r.$f delete 0 end }
    enterText .m.d.text ""
    .m.t.b.collect.m configure -text $collection
    foreach w [winfo children .m] {
	if {[string first ".m.document" $w] == 0} { closeDocumentWindow $w }
    }

    disableButton .m.d.buttons.previous 
    disableButton .m.d.buttons.next

    if {$c != ""} {
        set f $env(MGDATA)/$c

        if {[file exists $f] && [file readable $f]} {
    	    set fileId [open "| mgquery $collection" RDWR]
	    sendto ".set expert \"true\""
	    sendto ".set terminator \"$Terminator\\n\""
	    sendto ".set pager \"cat\""
            sendto ".set heads_length $HeadLength"
	    return
        }
    }
    bind .m.t.q.e.entry <Return> {null}
    enterText .m.d.text [initializationFailure]
    update
}

# Routine to obtain values from RESOURCE_MANAGER
#
proc query_resource {option class default} {
	set val [option get . $option $class]
	if {[string length $val] == 0} { return $default }
	return $val
}


proc disableButton {b} {
    bind $b <Button-1> {null}
    bind $b <Shift-Button-1> {null}
    $b configure -state disabled
}

proc null {} {}

###############################################################################
# Setting up windows
###############################################################################

proc removeFromWindowList {window} {
    global stacking

    set i 0
    while {$i < [llength $stacking]} {
      if {$window == [lindex $stacking $i]} {
         set stacking [lreplace $stacking $i $i]
      }
      incr i
    }
}

proc getFocus {window} {
    global stacking

    focus $window
    removeFromWindowList $window
    set stacking [linsert $stacking 0 $window]
}

proc closeDocumentWindow {w} {
   global stacking
   removeFromWindowList $w
   set t [lindex $stacking 0]

   if {$t != ""} {
     focus $t
     raise $t
  }
  destroy $w
}

# Make a "document" panel, including Previous, Next, and Close buttons
#
proc makeDocumentPanel {window} {
    frame $window.buttons
    button $window.buttons.previous -text "Previous" -width 10 -state disabled
    button $window.buttons.next -text "Next" -width 10 -state disabled
    button $window.buttons.close -text "Close" -width 10 \
	-command "closeDocumentWindow $window"
    button $window.buttons.save -text "Save" -width 10 \
        -command "SaveDocument $window.text"

    label $window.buttons.image -text "Fetching image..." \
            -foreground [lindex [$window configure -background] 4]
    pack $window.buttons.previous $window.buttons.next -side left
    pack $window.buttons.image -side left -padx 10
    pack $window.buttons.close $window.buttons.save -side right 
    pack $window.buttons -side top -fill x 

    text $window.text -borderwidth 2 -relief raised \
             -setgrid true -width 80 -state disabled

    $window.text configure -yscrollcommand "$window.sy set" 
    scrollbar $window.sy -relief flat -command "$window.text yview"
    pack $window.sy -side right -fill y 
    pack $window.text -side top -expand yes -fill both
}

# Make the "buttons" panel, with the query type and collection name
#
proc mkButtons {window} {
    global collection booleanquery approxquery env
    frame $window.type
    label $window.type.label -text "Query type"

    radiobutton $window.type.boolean -text "Boolean" -variable booleanquery \
	  -value 1 -relief flat

    radiobutton $window.type.ranked -text "Ranked" -variable booleanquery \
	  -value 0 -relief flat

    checkbutton $window.type.approx -text "Approximate" -variable approxquery \
	  -relief flat

    trace variable booleanquery w changeQuerytype
    trace variable approxquery w changeQuerytype

    pack $window.type.label   -side top -anchor w
    pack $window.type.boolean -side top -anchor w -padx 12
    pack $window.type.ranked  -side top -anchor w -padx 12
    pack $window.type.approx  -side top -anchor w -padx 36

    pack $window.type -side top -anchor w -pady 12

    frame $window.collect
    label $window.collect.label -text "Collection: "
    pack $window.collect.label -side left -anchor w

    getViewer

    set dirs [getMGDATA]
    set collection ""
    if {$dirs != ""} { set collection [lindex $dirs 0] }
   
    menubutton $window.collect.m -menu $window.collect.m.menu \
	-relief sunken
    menu $window.collect.m.menu
    foreach c $dirs {
        if {[file readable $env(MGDATA)/$c] && 
            [file isdirectory $env(MGDATA)/$c]} {
    	    $window.collect.m.menu add command -label $c \
	        -command "reInitMG $c"
        }
    }
    pack $window.collect.m -side left
    pack $window.collect -side bottom -anchor w


}

# Check that the MGDATA environment variable is set, and is non-null
#
proc getMGDATA {} {
    global env

    set MGDATA 0
    foreach i [array names env] {
	if {$i == "MGDATA"} {
	    set dirs [exec /bin/ls $env(MGDATA)]
	    if {[info exists dirs]} { return $dirs }
	}
    }

    return ""
}

proc getViewer {} {
    global env

    set viewer 0
    foreach i [array names env] {
	if {$i == "MGIMAGEVIEWER"} {set viewer $env(MGIMAGEVIEWER)}
    }

    if {$viewer == 0} {set env(MGIMAGEVIEWER) "xloadimage -quiet -fit stdin"}
}

# Make the "query" panel, with the query and list of retrieved documents
#
proc mkQuery {window} {
    global hits ListBoxLength

# at the top of the "query" panel is the "entry" frame
    frame $window.e
    label $window.e.label -text "Query"
    entry $window.e.entry -relief sunk -width 47 -textvariable query
    pack $window.e.label -side left
    pack $window.e.entry -side right -expand yes -fill x
    focus default $window.e.entry
    focus $window.e.entry

    bind $window.e.entry <Control-Key> {null}
    bind $window.e.entry <Return> "sendQuery $window.r"
    bind $window.e.entry <Escape> { set query "" }
    bind $window.e.entry <2> {
	%W insert insert [selection get STRING]
	tk_entrySeeCaret %W
    }

# next is the "retrieved" frame
    frame $window.retrieved
    label $window.retrieved.weight -text "Weight"
    label $window.retrieved.label -text "   Documents retrieved: "
    label $window.retrieved.hits -width 4 -relief sunk -textvariable hits
    set documentsRetrieved 0
    pack $window.retrieved.weight $window.retrieved.label \
        $window.retrieved.hits -side left

# next is the "results" frame (note that the docnums listbox doesn't appear on
# the screen)
    frame $window.r
    scrollbar $window.r.sscroll -relief sunken -command "scroll $window.r"
#    bind $window.r.sscroll <ButtonRelease-1> "updateSdocs $window.r.sdocs $window.r.sdocnums"
    listbox $window.r.sdocnums
    listbox $window.r.sdocs -relief sunken \
            -setgrid 1 -geometry 50x$ListBoxLength
    listbox $window.r.srank -yscroll "$window.r.sscroll set" \
            -setgrid 1 -geometry 4x$ListBoxLength
  
    bind $window.r.srank <Any-1> {null}
    bind $window.r.srank <Any-2> {null}
    bind $window.r.srank <Any-3> {null}

    bind $window.r.srank <Any-B1-Motion> {null}
    bind $window.r.srank <Any-B2-Motion> {null}
    bind $window.r.srank <Any-B3-Motion> {null}

    bind $window.r.sdocs <Any-2> {null}
    bind $window.r.sdocs <Any-B2-Motion> {null}

    pack $window.r.sscroll -side right -fill y
    pack $window.r.srank -side left 
    pack $window.r.sdocs -side left -fill x -expand yes

# pack them up
    pack $window.e -side top -anchor e -pady 10 -fill x -expand yes
    pack $window.retrieved -anchor w
    pack $window.r -side bottom -anchor e -pady 10 -fill x -expand yes
}

# Make a "help" window
#
proc mkHelp {} {
    if [winfo exists .help] { destroy .help }
    toplevel .help
    wm title .help "X interface to the mg information retrieval system"
    wm iconname .help "About xmg"
    frame .help.buttons
    button .help.buttons.close -text Close -command "destroy .help" -width 10
    button .help.buttons.help -text Help -width 10
    button .help.buttons.conditions -text Conditions -width 10
    button .help.buttons.warranty -text Warranty -width 10
    button .help.buttons.bugs -text Bugs -width 10

    frame .help.t
    text .help.t.text -relief raised -bd 2 -setgrid true -height 24 -width 95
    .help.t.text configure -yscrollcommand ".help.t.sy set" 
    scrollbar .help.t.sy -relief flat -command ".help.t.text yview"
    
    pack .help.buttons.close .help.buttons.help -side right -fill x
    pack .help.buttons.conditions .help.buttons.warranty \
         .help.buttons.bugs -side left
    pack .help.buttons -side bottom -fill x

    pack .help.t.text -side left -expand yes -fill both
    pack .help.t.sy -side right -fill y 
    pack .help.t -side top -fill both -expand yes
    enterText .help.t.text [HelpText]
    grayButton help
}

proc grayButton {button} {
    bind .help.buttons.help <1> "enterText .help.t.text \[HelpText\]
         grayButton help" 
    .help.buttons.help configure -state normal

    bind .help.buttons.bugs <1> "enterText .help.t.text \[Bugs\]
         grayButton bugs" 
    .help.buttons.bugs configure -state normal

    bind .help.buttons.warranty <1> "enterText .help.t.text \[Warranty\]
         grayButton warranty"
    .help.buttons.warranty configure -state normal

    bind .help.buttons.conditions <1> "enterText .help.t.text \[Conditions\]
         grayButton conditions" 
    .help.buttons.conditions configure -state normal
    disableButton .help.buttons.$button
}

# The text for errors in the MG environment variable
#


proc bookDetails {} {
  return { \

 This X interface was written by Bruce McKenzie, Ian Witten and Craig Nevill-Manning.  
 For information on mg, see
             Witten, I.H.,  Moffat, A. and Bell, T.C. (1994) "Managing gigabytes: compressing
             and indexing documents and images." Van Nostrand Reinhold, New York.
             ISBN 0-442-01863-0, US $54.95; call 1 (800) 544-0550 to order.
}
}

proc initializationFailure {} {
    return [format "%s\n%s" {\
In order to run XMG you need to have already created and indexed some document collections,
 and placed them in a directory whose name is given by the environment variable MGDATA.
 Your MGDATA variable is unset or does not specify a directory with files in; therefore XMG
 cannot proceed.
} [bookDetails]]
}

proc mgqueryFailure {} {
    return [format "%s\n%s" {\
In order to run XMG you need to have already created and indexed some document collections,
 and placed them in a directory whose name is given by the environment variable MGDATA.
 There is a problem with the collection that you have chosen. Please re-index the collection before
 you use it again.
} [bookDetails]]

}

# The text for the help message
#
proc HelpText {} {
    return [format "%s\n%s" {\
This is an X interface to the mg information retrieval system.

 Type a query in the "Query" box.
 The first line or so of each matching document is listed. 
 Click on any line to show the complete document.
 Shift-click to show the document in its own window
 Press Next and Previous to cycle through the list of documents (Shift works here too).
 In document windows (other than the main window), typing n or p shows the next and previous 
 documents; N or P creates a new window for them. q closes the most recently created window.
 If you change the query, existing windows continue to refer to the original query.
 
 You can select either Boolean or Ranked queries.
 Boolean queries: use & for AND, | for OR, ! for NOT, and parentheses for grouping
 Ranked queries:  documents are listed in order of relevance; a weight is shown alongside each
                                  weights are normalized to 100 for the most relevant document
                                  choose Approximate Ranking for greater retrieval speed. 
 } [bookDetails]]
}

proc reformat {text} {
    regsub -all "\n *\n *" $text "BLEQUEWFI" text
    regsub -all "\[ \t\n\r\]+" $text " " text
    regsub -all "BLEQUEWFI" $text "\n\n" text
    return $text
}

proc Conditions {} {
    return [reformat [join [sendandget ".conditions\nTHISWILLNEVERMATCHANYTHING\n"] "\n"]]
}

proc Warranty {} {
    return [reformat [join [sendandget ".warranty\nTHISWILLNEVERMATCHANYTHING\n"] "\n"]]
}

proc Bugs {} {
    return [reformat {
Known shortcomings of xmg include:

    Slow picture viewing.  Pictures are decompressed in their entirety before a window is created 
        for the picture.  This often results in a long delay with no feedback. Moreover, when 
        an image's document is displayed repeatedly, multiple identical images are shown.

    Sequential viewing.  There is no way of going to the next document in the collection to see the
        context of documents that have been retrieved.  This would be trivial to implement but
        conflicts with the interpretation of the "previous" and "next" buttons.
    }]
}

###############################################################################
# The main program
###############################################################################

scan [info tclversion] "%d.%d" major minor

if {$major < 7} {
    puts "xmg requires version 7.0 of the tcl interpreter. You are using version $major.$minor"
    exit
}

# Default font settings
set defaulttextfont   -adobe-times-medium-r-normal--14-140-75-75-p-74-iso8859-1
set defaultbuttonfont -adobe-times-bold-r-normal--14-140-75-75-p-77-iso8859-1
set textfont   [query_resource textfont Textfont $defaulttextfont]
set buttonfont [query_resource buttonfont Buttonfont $defaultbuttonfont]

set MaxWindowHeight 10
set HeadLength 100
set scrolledto 0
set ListBoxLength 12
set windowname 0
set Terminator "RECORD_END"
set fileId ""
set query ""
set ImageString "::::::::::"
set stacking ".m.t"

tk_listboxSingleSelect Listbox
option add *Text.wrap word

option add *Text.font $textfont
option add *Listbox.font $textfont
option add *Entry.font $textfont

option add *Button.font $buttonfont
option add *Menubutton.font $buttonfont
option add *Radiobutton.font $buttonfont
option add *Checkbutton.font $buttonfont
option add *Label.font $buttonfont
option add *Menu.font $buttonfont

wm title . "Managing Gigabytes"
wm iconname . "xmg"
frame .m -borderwidth 5 -relief raised

# top part of frame
frame .m.t

# on the left side of the top part is the "buttons" panel
frame .m.t.b
mkButtons .m.t.b

# on the right side is the "query" panel
frame .m.t.q
mkQuery .m.t.q

set q [query_resource querytype Querytype approx-ranked]
set booleanquery [expr \"$q\" == \"boolean\"]
set approxquery [expr \"$q\" == \"approx-ranked\"]
# the above lines will trigger changeQuerytype

pack .m.t.b -side left -anchor nw
pack .m.t.q -side right -anchor ne -fill both -expand yes

# at the bottom is the "document" panel
frame .m.d
makeDocumentPanel .m.d

.m.d.buttons.close configure -text "Exit" \
    -command "destroy ."
button .m.d.buttons.help -text "Help" -width 10 \
	-command "mkHelp"

pack .m.d.buttons.help -side right
pack .m.t -side top -fill x -expand yes
pack .m.d -side bottom -fill both -expand yes

pack .m -fill both -expand yes
wm geometry . +300+85

set collection [query_resource collection Collection $collection]
if {$argc>0} { set collection $argv }

enterText .m.d.text [HelpText]

initMG $collection

###############################################################################
###############################################################################
# The following file selection box procedure was written by Andrew Donkin
# of the University of Waikato. 
###############################################################################

proc FSBox { {fsBoxMessage "Select file:"}} {
# xf ignore me 5
  global fsBox

  set fsBox(name) ""
  set fsBox(internalPath) [pwd]
  set fsBox(pattern) "*"
  set fsBox(all) 0

  # build widget structure

  # start build of toplevel
  if {"[info commands XFDestroy]" != ""} {
    catch {XFDestroy .fsBox}
  } {
    catch {destroy .fsBox}
  }
  toplevel .fsBox -borderwidth 0
  wm geometry .fsBox 350x300 
  wm title .fsBox {File select box}
  wm maxsize .fsBox 1000 1000
  wm minsize .fsBox 100 100
  # end build of toplevel

  label .fsBox.message1 -anchor c -text "$fsBoxMessage"
  frame .fsBox.frame1  -borderwidth 0  -relief raised

  button .fsBox.frame1.ok  -text "OK"  -command "
      global fsBox
      set fsBox(name) \[.fsBox.file.file get\]
      set fsBox(path) \[.fsBox.path.path get\]

      set fsBox(internalPath) \[.fsBox.path.path get\]

      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy .fsBox}
      } {
        catch {destroy .fsBox}
      }"

  button .fsBox.frame1.rescan -text "Rescan" -command {
      global fsBox
      FSBoxFSShow [.fsBox.path.path get] [.fsBox.pattern.pattern get] $fsBox(all)
  }

  button .fsBox.frame1.cancel -text "Cancel" -command "
      global fsBox
      set fsBox(name) {}
      set fsBox(path) {}

      if {\"\[info commands XFDestroy\]\" != \"\"} {
        catch {XFDestroy .fsBox}
      } {
        catch {destroy .fsBox}
      }"

  frame .fsBox.path -borderwidth 0 -relief raised
  frame .fsBox.path.paths 
  label .fsBox.path.paths.paths -borderwidth 0 -text "Pathname:"

  entry .fsBox.path.path -relief sunken
  .fsBox.path.path insert 0 $fsBox(internalPath)
  frame .fsBox.pattern -borderwidth 0 -relief raised
  frame .fsBox.pattern.patterns 

  label .fsBox.pattern.patterns.patterns -borderwidth 0 -text "Selection pattern:"
  entry .fsBox.pattern.pattern -relief sunken
  .fsBox.pattern.pattern insert 0 $fsBox(pattern)
  
  frame .fsBox.files -borderwidth 0 -relief raised
  scrollbar .fsBox.files.vscroll -relief raised -command ".fsBox.files.files yview"
  scrollbar .fsBox.files.hscroll -orient horiz -relief raised -command ".fsBox.files.files xview" 
  listbox .fsBox.files.files -exportselection false -relief raised -xscrollcommand \
        ".fsBox.files.hscroll set" -yscrollcommand ".fsBox.files.vscroll set"
  frame .fsBox.file -borderwidth 0 -relief raised
  label .fsBox.file.labelfile -text "Filename:"
  entry .fsBox.file.file -relief sunken
  focus .fsBox.file.file

  .fsBox.file.file delete 0 end
  .fsBox.file.file insert 0 $fsBox(name)
  
  checkbutton .fsBox.pattern.all -offvalue 0 -onvalue 1 -text "Show all files" -variable fsBox(all) -command {
      global fsBox
      FSBoxFSShow [.fsBox.path.path get] [.fsBox.pattern.pattern get] $fsBox(all)
    }

  FSBoxFSShow $fsBox(internalPath) $fsBox(pattern) $fsBox(all)

  # bindings
  bind .fsBox.files.files <Double-Button-1> "FSBoxFSFileSelectDouble %W %y"
  bind .fsBox.files.files <ButtonPress-1> "FSBoxFSFileSelect %W %y"
  bind .fsBox.files.files <Button1-Motion> "FSBoxFSFileSelect %W %y"
  bind .fsBox.files.files <Shift-Button1-Motion> "FSBoxFSFileSelect %W %y"
  bind .fsBox.files.files <Shift-ButtonPress-1> "FSBoxFSFileSelect %W %y"

  bind .fsBox.path.path <Return> {
    global fsBox
    FSBoxFSShow [.fsBox.path.path get] [.fsBox.pattern.pattern get] $fsBox(all)
    .fsBox.file.file icursor end
    focus .fsBox.file.file}
  catch "bind .fsBox.path.path <Up> {}"
  bind .fsBox.path.path <Down> {
    .fsBox.file.file icursor end
    focus .fsBox.file.file}

  bind .fsBox.file.file <Return> "
    global fsBox
    set fsBox(name) \[.fsBox.file.file get\]
    set fsBox(path) \[.fsBox.path.path get\]
    set fsBox(internalPath) \[.fsBox.path.path get\]

    if {\"\[info commands XFDestroy\]\" != \"\"} {
      catch {XFDestroy .fsBox}
    } {
      catch {destroy .fsBox}
    }"
  bind .fsBox.file.file <Up> {.fsBox.path.path icursor end
    focus .fsBox.path.path}
  bind .fsBox.file.file <Down> {.fsBox.pattern.pattern icursor end
    focus .fsBox.pattern.pattern}

  bind .fsBox.pattern.pattern <Return> {
    global fsBox
    FSBoxFSShow [.fsBox.path.path get] [.fsBox.pattern.pattern get] $fsBox(all)}
  bind .fsBox.pattern.pattern <Up> {
    .fsBox.file.file icursor end
    focus .fsBox.file.file}
  catch "bind .fsBox.pattern.pattern <Down> {}"

  # packing
  pack .fsBox.files.vscroll -side left -fill y 
  pack .fsBox.files.hscroll -side bottom -fill x
  pack .fsBox.files.files -side left -fill both -expand yes

  pack .fsBox.file.labelfile -side left
  pack .fsBox.file.file -side left -fill  both -expand yes

  pack .fsBox.frame1.ok -side left -fill both -expand yes
  pack .fsBox.frame1.rescan -side left -fill both -expand yes
  pack .fsBox.frame1.cancel -side left -fill both -expand yes

  pack .fsBox.path.paths.paths -side left

  pack .fsBox.pattern.patterns.patterns -side left

  pack .fsBox.path.paths -side left
  pack .fsBox.path.path -side left -fill both -expand yes

  pack .fsBox.pattern.patterns -side left
  pack .fsBox.pattern.all -side right -fill both
  pack .fsBox.pattern.pattern -side left -fill both -expand yes

  pack .fsBox.message1 -side top -fill both
  pack .fsBox.frame1 -side bottom -fill both
  pack .fsBox.pattern -side bottom -fill both
  pack .fsBox.file -side bottom -fill both
  pack .fsBox.path -side bottom -fill both
  pack .fsBox.files -side left -fill both -expand yes

  update idletask
  grab .fsBox
  tkwait window .fsBox

  if {"[string trim $fsBox(path)]" != "" ||
      "[string trim $fsBox(name)]" != ""} {
    if {"[string trimleft [string trim $fsBox(name)] /]" == ""} {
      return [string trimright [string trim $fsBox(path)] /]
    } {
      return [string trimright [string trim $fsBox(path)] /]/[string trimleft [string trim $fsBox(name)] /]
    }
  }
}

proc FSBoxBindSelectOne { fsBoxW fsBoxY} {
# xf ignore me 6

  set fsBoxNearest [$fsBoxW nearest $fsBoxY]
  if {$fsBoxNearest >= 0} {
    $fsBoxW select from $fsBoxNearest
    $fsBoxW select to $fsBoxNearest
  }
}

proc FSBoxFSFileSelect { fsBoxW fsBoxY} {
# xf ignore me 6
  global fsBox

  FSBoxBindSelectOne $fsBoxW $fsBoxY
  set fsBoxNearest [$fsBoxW nearest $fsBoxY]
  if {$fsBoxNearest >= 0} {
    set fsBoxTmpEntry [$fsBoxW get $fsBoxNearest]
    if {"[string index $fsBoxTmpEntry           [expr [string length $fsBoxTmpEntry]-1]]" == "/" ||
        "[string index $fsBoxTmpEntry           [expr [string length $fsBoxTmpEntry]-1]]" == "@"} {
      set fsBoxFileName [string range $fsBoxTmpEntry 0             [expr [string length $fsBoxTmpEntry]-2]]
      if {![IsADir [string trimright $fsBox(internalPath)/$fsBoxFileName @]] &&
          ![IsASymlink [string trimright $fsBox(internalPath)/$fsBoxFileName @]]} {
        set fsBoxFileName $fsBoxTmpEntry
      }
    } {
      if {"[string index $fsBoxTmpEntry             [expr [string length $fsBoxTmpEntry]-1]]" == "*"} {
        set fsBoxFileName [string range $fsBoxTmpEntry 0           [expr [string length $fsBoxTmpEntry]-2]]
        if {![file executable $fsBox(internalPath)/$fsBoxFileName]} {
          set fsBoxFileName $fsBoxTmpEntry
        }
      } {
        set fsBoxFileName $fsBoxTmpEntry
      }
    }
    if {![IsADir [string trimright $fsBox(internalPath)/$fsBoxFileName @]]} {
      set fsBox(name) $fsBoxFileName
      .fsBox.file.file delete 0 end
      .fsBox.file.file insert 0 $fsBox(name)
    }
  }
}

proc FSBoxFSFileSelectDouble { fsBoxW fsBoxY} {
# xf ignore me 6
  global fsBox

  FSBoxBindSelectOne $fsBoxW $fsBoxY
  set fsBoxNearest [$fsBoxW nearest $fsBoxY]
  if {$fsBoxNearest >= 0} {
    set fsBoxTmpEntry [$fsBoxW get $fsBoxNearest]
    if {"$fsBoxTmpEntry" == "../"} {
      set fsBoxTmpEntry [string trimright [string trim $fsBox(internalPath)] "@/"]
      if {"$fsBoxTmpEntry" == ""} {
        return
      }
      FSBoxFSShow [file dirname $fsBoxTmpEntry]         [.fsBox.pattern.pattern get] $fsBox(all)
      .fsBox.path.path delete 0 end
      .fsBox.path.path insert 0 $fsBox(internalPath)
    } {
      if {"[string index $fsBoxTmpEntry [expr [string length $fsBoxTmpEntry]-1]]" == "/" ||
          "[string index $fsBoxTmpEntry [expr [string length $fsBoxTmpEntry]-1]]" == "@"} {
        set fsBoxFileName [string range $fsBoxTmpEntry 0               [expr [string length $fsBoxTmpEntry]-2]]
        if {![IsADir [string trimright $fsBox(internalPath)/$fsBoxFileName @]] &&
            ![IsASymlink [string trimright $fsBox(internalPath)/$fsBoxFileName @]]} {
          set fsBoxFileName $fsBoxTmpEntry
        }
      } {
        if {"[string index $fsBoxTmpEntry [expr [string length $fsBoxTmpEntry]-1]]" == "*"} {
          set fsBoxFileName [string range $fsBoxTmpEntry 0                 [expr [string length $fsBoxTmpEntry]-2]]
          if {![file executable $fsBox(internalPath)/$fsBoxFileName]} {
            set fsBoxFileName $fsBoxTmpEntry
          }
        } {
          set fsBoxFileName $fsBoxTmpEntry
        }
      }
      if {[IsADir [string trimright $fsBox(internalPath)/$fsBoxFileName @]]} {
        set fsBox(internalPath) "[string trimright $fsBox(internalPath) {/@}]/$fsBoxFileName"
        FSBoxFSShow $fsBox(internalPath) [.fsBox.pattern.pattern get] $fsBox(all)
        .fsBox.path.path delete 0 end
        .fsBox.path.path insert 0 $fsBox(internalPath)
      } {
        set fsBox(name) $fsBoxFileName
        set fsBox(path) $fsBox(internalPath)
        if {"[info commands XFDestroy]" != ""} {
          catch {XFDestroy .fsBox}
        } {
          catch {destroy .fsBox}
        }
      }
    }
  }
}

proc FSBoxFSShow { fsBoxPath fsBoxPattern fsBoxAll} {
# xf ignore me 6
  global fsBox

  set fsBox(pattern) $fsBoxPattern

  if {[file exists $fsBoxPath] && [file readable $fsBoxPath] &&
      [IsADir $fsBoxPath]} {
    set fsBox(internalPath) $fsBoxPath
  } {
    if {[file exists $fsBoxPath] && [file readable $fsBoxPath] &&
        [IsAFile $fsBoxPath]} {
      set fsBox(internalPath) [file dirname $fsBoxPath]
      .fsBox.file.file delete 0 end
      .fsBox.file.file insert 0 [file tail $fsBoxPath]
      set fsBoxPath $fsBox(internalPath)
    } {
      while {"$fsBoxPath" != "" && "$fsBoxPath" != "/" &&
             ![file isdirectory $fsBoxPath]} {
        set fsBox(internalPath) [file dirname $fsBoxPath]
         set fsBoxPath $fsBox(internalPath)
      }
    }
  }
  if {"$fsBoxPath" == ""} {
    set fsBoxPath "/"
    set fsBox(internalPath) "/"
  }
  .fsBox.path.path delete 0 end
  .fsBox.path.path insert 0 $fsBox(internalPath)

  if {[.fsBox.files.files size] > 0} {
    .fsBox.files.files delete 0 end
  }
  if {$fsBoxAll} {
    if {[catch "exec ls -F -a $fsBoxPath" fsBoxResult]} {
      puts stderr "$fsBoxResult"
    }
  } {
    if {[catch "exec ls -F $fsBoxPath" fsBoxResult]} {
      puts stderr "$fsBoxResult"
    }
  }
  set fsBoxElementList [lsort $fsBoxResult]

  foreach fsBoxCounter [winfo children .fsBox.pattern.patterns.patterns] {
    if {[string length [info commands XFDestroy]] > 0} {
      catch {XFDestroy $fsBoxCounter}
    } {
      catch {destroy $fsBoxCounter}
    }
  }
  menu .fsBox.pattern.patterns.patterns.menu

  if {"$fsBoxPath" != "/"} {
    .fsBox.files.files insert end "../"
  }
  foreach fsBoxCounter $fsBoxElementList {
    if {[string match $fsBoxPattern $fsBoxCounter] ||
        [IsADir [string trimright $fsBoxPath/$fsBoxCounter "/@"]]} {
      if {"$fsBoxCounter" != "../" &&
          "$fsBoxCounter" != "./"} {
        .fsBox.files.files insert end $fsBoxCounter
      }
    }
  }
}

proc IsADir { pathName} {
# xf ignore me 5
  if {[file isdirectory $pathName]} {
    return 1
  } {
    catch "file type $pathName" fileType
    if {"$fileType" == "link"} {
      if {[catch "file readlink $pathName" linkName]} {
        return 0
      }
      catch "file type $linkName" fileType
      while {"$fileType" == "link"} {
        if {[catch "file readlink $linkName" linkName]} {
          return 0
        }
        catch "file type $linkName" fileType
      }
      return [file isdirectory $linkName]
    }
  }
  return 0
}

proc IsAFile { fileName} {
# xf ignore me 5
  if {[file isfile $fileName]} {
    return 1
  } {
    catch "file type $fileName" fileType
    if {"$fileType" == "link"} {
      if {[catch "file readlink $fileName" linkName]} {
        return 0
      }
      catch "file type $linkName" fileType
      while {"$fileType" == "link"} {
        if {[catch "file readlink $linkName" linkName]} {
          return 0
        }
        catch "file type $linkName" fileType
      }
      return [file isfile $linkName]
    }
  }
  return 0
}

proc IsASymlink { fileName} {
# xf ignore me 5
  catch "file type $fileName" fileType
  if {"$fileType" == "link"} {
    return 1
  }
  return 0
}
