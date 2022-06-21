#! /bin/csh -f
###########################################################################
#
# mg_get.sh -- script to get text for mgbuild
# Copyright (C) 1994  Tim Bell
# Changed to allow it to be driven from a file ~/.mg_getrc
# by Bruce McKenzie, Oct 1994
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
#       @(#)mg_get.sh	1.11 25 Mar 1994
#
###########################################################################
#
# "mg_get" formats the text for mgbuild by collecting the appropriate
#   documents, and outputing them with control-B after each document.
#   The name of the text must be supplied as an argument.
#   mg_get is called with -i (init) first, then with -t (text) each time the 
#   text is needed, then with -c (cleanup).
#   e.g. mg_get alice -t
#
# The file ~/.mg_get contains lines of the form:
#     name	TYPE	files and directories
# where name is the name supplied to mg_get, TYPE is one of
# PARA, MAIL, DIR, DIR2, BIB or TXTIMG and says how mg_get should process the
# named files and directories
#
###########################################################################

if ($?MG_GETRC) then
  set mg_getrc = $MG_GETRC
else
  set mg_getrc = ~/.mg_getrc
endif

# Added by [TS:May/95]
# if we don't have an mg_getrc file in existence then create a default one
if !(-e $mg_getrc) then 
  echo "Can not find a .mg_getrc, so creating a default one: $mg_getrc"

  if ($?MGSAMPLE) then
    set SampleData = $MGSAMPLE
  else
    set SampleData = ./SampleData
  endif

  cat > $mg_getrc << END
alice	PARA	$SampleData/alice13a.txt.Z
davinci	TXTIMG	$SampleData/davinci
mailfiles	MAIL	~/mbox ~/.sentmail
allfiles	DIR	~/Mail
mods	LINESEP	$SampleData/MODIFICATIONS
bible	PLAIN	$SampleData/bible/genesis.txt.Z $SampleData/bible/revelation.txt.Z
END
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
     echo 'Usage: mg_get <document> [-i | -t | -c]'
     exit(1)
     endsw
  breaksw

  default:
  echo 'Usage: get <document> [-i | -t | -c]'
  exit(1)
  endsw

set COLLECTION = $1
set TYPE  = `grep \^$1 $mg_getrc | cut -f2`
set FILES = `grep \^$1 $mg_getrc | cut -f3`

set bindir = $0
set bindir = $bindir:h

switch ($TYPE)
  case PLAIN:
# Just outputs the given text, assumes ^Bs already inserted 
  switch ($flag)
     case '-init':
     breaksw

     case '-text':
      foreach f ($FILES)
	if ($f:e == 'Z') then
	  set decoder=zcat
	else if ($f:e == 'gz') then
	  set decoder=gunzip
	else
	  set decoder=cat
	endif
        $decoder < $f
      end
     breaksw #-text

     case '-cleanup':
     breaksw
  endsw #flag
  breaksw

  case LINESEP:
# Assumes that the documents are separated by a line of =====
  switch ($flag)
     case '-init':
     breaksw

     case '-text':
      foreach f ($FILES)
	if ($f:e == 'Z') then
	  set decoder=zcat
	else if ($f:e == 'gz') then
	  set decoder=gunzip
	else
	  set decoder=cat
	endif
        $decoder < $f |  sed -e '/\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*/s/.*//' < $f
      end
     breaksw #-text

     case '-cleanup':
     breaksw
  endsw #flag
  breaksw

  case PARA:
# Splits the file at blank lines, which corresponds to paragraphs.
  switch ($flag)
     case '-init':
     breaksw

     case '-text':
      foreach f ($FILES)
	if ($f:e == 'Z') then
	  set decoder=zcat
	else if ($f:e == 'gz') then
	  set decoder=gunzip
	else
	  set decoder=cat
	endif
      $decoder <$f |awk '   /^ *$/ {if (b!=1) printf "";b=1} \
		\!/^ *$/ {print;b=0} \
		 END {printf ""}'
       end #foreach
     breaksw #-text

     case '-cleanup':
     breaksw
  endsw #flag
  breaksw

  case BIB:
# Takes a list of files that contain bibliographies, and splits them up 
#   by putting ^B after each reference. Assumes that each reference
#   begins with a line '^@'.

  switch ($flag)
    case '-init':
    breaksw
 
    case '-text':
      foreach f ($FILES)
	if ($f:e == 'Z') then
	  set decoder=zcat
	else if ($f:e == 'gz') then
	  set decoder=gunzip
	else
	  set decoder=cat
	endif
      $decoder <$f |awk '/^@/&&NR!=1{printf ""} {print $0} END{print ""}'
      end # foreach
    breaksw #-text
 
    case '-cleanup':
    breaksw #-cleanup
  endsw #flag
  breaksw #BIB

  case MAIL:
# Takes a list of files that contain mail, and splits them up 
#   by putting ^B after each message. Assumes that each message
#   begins with a line '^From '.

  switch ($flag)
    case '-init':
    breaksw
 
    case '-text':
      foreach f ($FILES)
	if ($f:e == 'Z') then
	  set decoder=zcat
	else if ($f:e == 'gz') then
	  set decoder=gunzip
	else
	  set decoder=cat
	endif
      $decoder <$f| awk '/xbtoa Begin/,/xbtoa End/ {next} /^From /&&NR!=1{printf ""} {print $0} END{print ""}'
      end # foreach
    breaksw #-text
 
    case '-cleanup':
    breaksw #-cleanup
  endsw #flag
  breaksw #MAIL

  case DIR:
# Recursively concatenates every file in every subdirectory of the given
#   directory.

  switch ($flag)
    case '-init':
    breaksw

    case '-text'
      find $FILES -type f -name '*.gz' -exec echo '<' {} '>' \;		\
			 -exec gzcat {} \; -exec echo -n '' \;
      find $FILES -type f -name '*.Z' -exec echo '<' {} '>' \;		\
			 -exec zcat {} \; -exec echo -n '' \;
#     find $FILES -type f -not -regex '.*\.\(gz\|Z\)' -exec echo '<' {} \
#			'>' \; -exec cat {} \; -exec echo -n '' \;
      find $FILES -type f \! \( -name '*.gz' -o -name '*.Z' \) \
			-exec echo '<' {} '>' \; \
			-exec cat {} \; -exec echo -n '' \;

    breaksw

    case '-cleanup':
    breaksw #-cleanup
  endsw #flag
  breaksw #DIR



  case DIR2:
# Recursively concatenates every file in every subdirectory of the given
#   directory. Does not include filename

  switch ($flag)
    case '-init':
    breaksw

    case '-text'
      find $FILES -type f -name '*.gz'  		\
			 -exec gzcat {} \; -exec echo -n '' \;
      find $FILES -type f -name '*.Z' 	 		\
			 -exec zcat {} \; -exec echo -n '' \;
#     find $FILES -type f -not -regex '.*\.\(gz\|Z\)'	\
#			-exec cat {} \; -exec echo -n '' \;
      find $FILES -type f \! \( -name '*.gz' -o -name '*.Z' \) \
			-exec cat {} \; -exec echo -n '' \;
    breaksw

    case '-cleanup':
    breaksw #-cleanup
  endsw #flag
  breaksw #DIR2




  case TXTIMG:
# compress and index the collection of text and images
# (this code is suitable for all sorts of integrated collections
#  of text and images)
# Files that are related have the same prefix. For example,
#  monaLisa.pgm might be a gray-level image, and
#  monaLisa.txt would be a textual file describing the image.
# The suffixes recognised are:
#    .txt	ascii text
#    .ptm	scanned text stored as a bilevel image
#    .pbm	a black and white image (typically a line drawing)
#    .pgm	a gray-scale image
# In addition, if no corresponding ascii text file is found for
#  a .pbm or .pgm file, then one is created with suffix .tmp.txt,
#  and it stores the name of the image file (in principle it could
#  store the OCR of a .txt.pbm file). At present the .tmp.txt files
#  are deleted by the '-cleanup' option.
  
#### the next two 'set' statements define which directory the files
#    come from, and where they will be stored. This is the only
#    part of this code that is specific to the 'davinci' collection
  set sourceDir = $FILES
  set targetDir = $MGDATA/$COLLECTION

  switch ($flag)
    case '-init':
      mkdir $targetDir >&/dev/null	   # create the directory to store
					   # compressed images in
      $rm -rf $targetDir/* >&/dev/null     # in case it already existed
    
      # take care if no match to foreach statements
      set nonomatch

      # process all pbm (black and white) images of text
      foreach f ($sourceDir/*.ptm)
        if ($f == "$sourceDir/*.ptm") then
                break
        endif
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
        if ($f == "$sourceDir/*.pgm") then
                break
        endif
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
        if ($f == "$sourceDir/*.pbm") then
                break
        endif
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
      unset nonomatch
    breaksw #-init

    case '-text'
      # take care if no match to foreach statements
      set nonomatch

      #output each text file and tmp.txt, with names of other associated files

      foreach f ($sourceDir/*.txt $sourceDir/*.tmp)
        if ($f == "$sourceDir/*.txt") then
           continue
        endif
        if ($f == "$sourceDir/*.tmp") then
           continue
        endif
	set r = $f:r	#root name of file
	set base = $r:t
        set first_image = 1 # set so that I print out image header once
                            # also signifies if header printed out at all

	foreach d ($targetDir/$base.*)
          if ($d == "$targetDir/$base.*") then
                break
          endif

          if ($first_image) then
            set first_image = 0
            echo '::::::::::'  #separate the image information from the text
            echo 'Image(s) available:'
          endif

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

        if (! $first_image) then
	  echo '::::::::::'  #separate the image information from the text
        endif

	cat $f  # output the text associated with the images
	echo -n ''
      end #foreach f (text file)
      unset nonomatch
    breaksw #-text

    case '-cleanup':
      #remove temporary text files
      $rm $sourceDir/*.tmp
    breaksw #-cleanup
  endsw #flag
  breaksw #IMGTXT


  default:
    echo 'Sorry, I do not know how to get' $1
    exit 1	
endsw
exit 0
