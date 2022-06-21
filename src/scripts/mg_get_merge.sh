#! /bin/csh -f
###########################################################################
#
# mg_get_merge.sh -- script to get text for mgbuild
# Copyright (C) 1994  Tim Bell, Shane Hudson (mods. for merging)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
###########################################################################
#
# "mg_get_merge" formats the text for mgbuild by collecting the appropriate
#   documents, and outputing them with control-B after each document.
#   The name of the text must be supplied as an argument.
#   mg_get is called with -i (init) first, then with -t (text) each time the 
#   text is needed, then with -c (cleanup).
#   e.g. mg_get alice -t
#
###########################################################################



# The directory containing the data files is stored in the environment
#   variable MGSAMPLE

if ($?MGSAMPLE) then
  set SampleData = $MGSAMPLE
else
  set SampleData = ./SampleData
endif

if (! $?MGIMAGEVIEWER) then
  setenv MGIMAGEVIEWER "xv -"
endif

set rm = /bin/rm

switch ($#argv)
  case 1:
  set flag = '-text'
  breaksw

  case 2:
  set flag = $2
  switch ($flag)
     case '-i':
	set flag = '-init'
	breaksw
     case '-t':
	set flag = '-text'
	breaksw
     case '-c':
	set flag = '-cleanup'
	breaksw
     case '-init':
     case '-text':
     case '-cleanup':
     breaksw
     default:
     echo 'Usage: get <document> [-i | -t | -c]'
     exit(1)
     endsw
  breaksw

  default:
  echo 'Usage: get <document> [-i | -t | -c]'
  exit(1)
  endsw

set bindir = $0
set bindir = $bindir:h

switch ($1)
  case alice:
  switch ($flag)
     case '-init':
     breaksw

     case '-text':
	echo "This is a test document 1 2 3 4 5 6 7 8 9 "
	echo ""
	echo "A B C"
     breaksw

     case '-cleanup':
     breaksw
  endsw #flag
  breaksw

  case bible:
  switch ($flag)
    case '-init':
    breaksw

    case '-text':
       # use uncompressed file if it exists
       # revelation.txt already contains '^B' characters between verses
       if (-e $SampleData/bible/revelation.txt) then
          cat $SampleData/bible/revelation.txt
       else
          uncompress <$SampleData/bible/revelation.txt.Z
    breaksw #-text

    case '-cleanup':
    breaksw #-cleanup

  endsw #flag
  breaksw #bible



  case mailfiles:
# Takes a list of files that contain mail, and splits them up 
#   by putting ^B after each message. Assumes that each message
#   begins with a line '^From '.
# If messages are already in individual files, use "get allfiles"
#   rather than this one.
# The EXTRA documents added are in files in the directory mailextra
#### The variable 'mailfiles' contains a list of files to be split up
  set mailfiles = '~/mbox ~/.sentmail'
  set mailextra = '~/extra/*'

  switch ($flag)
    case '-init':
    breaksw

    case '-text':
      foreach f ($mailextra)
	awk '/^From /&&NR!=1{print ""} {print $0} END{print ""}' $f
      end # foreach
    breaksw #-text

    case '-cleanup':
    breaksw #-cleanup

  endsw #flag
  breaksw #mailfiles



  case allfiles:
# Recursively concatenates every file in every subdirectory of the given
#   directory.

# The variable 'directory' on the next line contains the directory to be used
  set directory = ~/Mail

# The date of the file ".mgbuildtime" in 'directory' is the last
# build or merge. At the start of the merge ("-init" argument) the
# file ".mgmergetime" is touched. Only files newer than ".mgbuildtime"
# but older than ".mgmergetime" are output by mg_get_merge

  switch ($flag)
    case '-init':
        touch $directory/.mgmergetime
    breaksw

    case '-text'
      find $directory -type f -newer $directory/.mgbuildtime \! -newer $directory/.mgmergetime \! -name '.*' -exec echo '<' {} '>' \; -exec cat {} \; -exec echo -n '' \;
    breaksw

    case '-cleanup':
        touch $directory/.mgmergetime
        touch $directory/.mgbuildtime
    breaksw #-cleanup
  endsw #flag
  breaksw #allfiles




  case davinci:
# compress and index the DaVinci collection of text and images
# (this code is suitable for all sorts of integrated collections
#  of text and images)
# It assumes that all files are in one directory ($sourceDir).
# Files that are related have the same prefix. For example,
#  monaLisa.img.pgm might be a gray-level image, and
#  monaLisa.txt.txt would be a textual file describing the image.
# The suffixes recognised are:
#    .txt.txt	ascii text
#    .txt.pbm	scanned text stored as a bilevel image
#    .img.pbm	a black and white image (typically a line drawing)
#    .img.pgm	a gray-scale image
# In addition, if no corresponding ascii text file is found for
#  a .pbm or .pgm file, then one is created with suffix .tmp.txt,
#  and it stores the name of the image file (in principle it could
#  store the OCR of a .txt.pbm file). At present the .tmp.txt files
#  are deleted by the '-cleanup' option.
  
#### the next two 'set' statements define which directory the files
#    come from, and where they will be stored. This is the only
#    part of this code that is specific to the 'davinci' collection
  set sourceDir = $SampleData/davinci
  set targetDir = $MGDATA/davinci

  switch ($flag)
    case '-init':
      mkdir $targetDir >&/dev/null	   #create the directory to store
					   # compressed images in
      $rm -rf $targetDir/* >&/dev/null #in case it already existed
    
      # process all pbm (black and white) images of text
      foreach f ($sourceDir/*.ptm)
	set base = $f:t
	
        $bindir/mgticbuild $targetDir/$base:r.ticlib.$$ $f
        $bindir/mgticprune $targetDir/$base:r.ticlib.$$

	$bindir/mgtic -L -e $targetDir/$base:r.ticlib.$$ $f >$targetDir/$base:r.tic
	$rm $targetDir/$base:r.ticlib.$$
	set r = $f:r	#root name of file
	if (! (-e $r.txt || -e $r.tmp)) then
	# creates a file; could do OCR here to get its contents
	    echo "No corresponding txt file for" $f "- creating" $r.tmp
	    echo '#######' $f > $r.tmp
	endif
      end
      # process all pgm (gray scale) images
      foreach f ($sourceDir/*.pgm)
	set base = $f:t
	$bindir/mgfelics -e $f >$targetDir/$base:r.flx
	set r = $f:r	#root name of file
	if (! (-e $r.txt || -e $r.tmp)) then
	# creates a file; could do image recognition here to create one(!)
	    echo "No corresponding txt file for" $f "- creating" $r.tmp
	    echo '#######' $f > $r.tmp
	endif
      end
      # process all pbm (black and white) images
      foreach f ($sourceDir/*.pbm)
	set base = $f:t
	$bindir/mgbilevel -e $f >$targetDir/$base:r.blv
	set r = $f:r	#root name of file
	if (! (-e $r.txt || -e $r.tmp)) then
	# creates a file; could do drawing recognition here to create one(!)
	    echo "No corresponding txt file for" $f "- creating" $r.tmp
	    echo '####### No corresponding text file available for this image' >$r.tmp
	    echo 'Original image file name was' $f >> $r.tmp
	endif
      end
    breaksw #-init

    case '-text'
      #output each text file and tmp.txt, with names of other associated files
      foreach f ($sourceDir/*.txt $sourceDir/*.tmp)
	set r = $f:r	#root name of file
	set base = $r:t
	set n = `/bin/ls $sourceDir/$base.* | wc -l`
	if ($n != 1) then # more files than just the text one
	 echo '::::::::::'  #separate the image information from the text
	 echo 'Image(s) available:'
	 foreach d ($targetDir/$base.*)
	  switch ($d:e)	# work out decoding method from suffix
            case 'tic':
              echo 'MGDATA (mgtic -d ' $d '  | '$MGIMAGEVIEWER') '
            breaksw
            case 'flx':
              echo 'MGDATA (mgfelics -d ' $d '  | '$MGIMAGEVIEWER')'
            breaksw
            case 'blv':
              echo 'MGDATA (mgbilevel -d ' $d '  | '$MGIMAGEVIEWER')'
	    breaksw
	  endsw # $d:e (suffix)
	 end #foreach d (associated file)
	echo '::::::::::'  #separate the image information from the text
	endif # if more than one file
	cat $f  # output the text associated with the images
	echo -n ''
      end #foreach f (text file)
    breaksw #-text

    case '-cleanup':
      #remove temporary text files
      $rm $sourceDir/*.tmp
    breaksw #-cleanup
  endsw #flag
  breaksw #davinci


#---------------------------
  case mods:
# Splits the modifications file at the mod separator "--------"
  switch ($flag)
     case '-init':
     breaksw
 
     case '-text':
       sed -e 's/^---.*//g' < ${SampleData}/MODIFICATIONS
     breaksw
 
     case '-cleanup':
     breaksw
  endsw #flag
  breaksw #mods

  default:
    echo 'Sorry, I do not know how to get' $1
    exit 1	
endsw
exit 0
