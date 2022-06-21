#!/bin/csh -f
###########################################################################
#
# mgmerge.sh -- Script used to build mg text collection.
# Copyright (C) 1994  Neil Sharman, Shane Hudson (mods. for merging)
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

set complex = ""
set slow_merge = ""
set guess_weights = "-w "
# [RPAP - Feb 97: Level 3 Merge]
set text_pass = 1

# Parse the command line arguments
while ($#argv >= 1)
  if ("$1" == "-s") then
    shift
    if ($#argv >= 1) then
      set source = $1
      shift
    endif
  else if ("$1" == "-g") then
    shift
    if ($#argv >= 1) then
      set get = $1
      shift
    endif
  else if ("$1" == "-c") then
    set complex = "-text"
    shift
  else if ("$1" == "-S") then
# set slow merge option in mg_invf_merge
    set slow_merge = "-s "
    shift
  else if ("$1" == "-w") then
# turn weights guessing off (rebuild weights file from scratch)
    set guess_weights = ""
    shift

# [RPAP - Feb 97: Level 3 Merge]
  else if ("$1" == "-T") then
# turn text pass off
    set text_pass = 0
    shift

  else
    if ($?text == "0") then
      set text = $1
    endif
    shift
  endif
    
end

if ($?text == "0") then
  set prog = $0
  echo "USAGE:"
  echo "     "$prog:t" [-s config-script] [-g get-program] [-c] source"
  echo ""
  echo " The config-script is only needed if a non-standard build is required."
  echo " The get-program defaults to mg_get if not specified."
  exit 1	
endif

set bindir = $0
set bindir = $bindir:h

# if $pipe == 1 then pipe in the source text using $get and $text otherwise
# read the source text directly from the file names specified in $input_files
set pipe = 1

if ($?get == "0") then
  set complex = "-text"
  set get = $bindir/mg_get_merge
endif

if (-e $MGDATA/${text}.chunks) then
  set input_files = `cat $MGDATA/${text}.chunks`
endif

# Set the inversion method this may be either "I" or "N"
# N is memory-efficient, I is faster for small collections
set invf_method = N


# Set the stemming method
#   Bit 0 = case folding
#   Bit 1 = S stemmer
set stem_method = 3


# [RPAP - Jan 97: Stem Index Change]
# If do_indexes == 1 then build collection with full indexes to blocked file
# overriding stem_method (stem_method will be set to 0)
# Otherwise don't build with indexes
set do_indexes = 0


# $invf_mem specifies the amount of memory to use for the pass2 inversion
# This only has an effect if $invf_method is "N".
set invf_mem = 32


# $num_chunks specifies the number of interium chunks of inverted file that
# may be written to disc before a merge into the invf file is done
# This only has an effect if $invf_method is "N".
set num_chunks = 3

# $invf_level specifies the level of the inverted file that will be generated.
# Note: The value *MUST* be the same as was used to build the collection
# that is being merged.
# In the current mg_invf_merge implementation the maximum level is 2. 
# Paragraph-level inversion (invf_level = 3) is NOT supported.
set invf_level = 2


# If $strip_sgml == 1 then sgml tags are stripped from the inversion phase.
# Otherwise sgml tags are kepted.
set strip_sgml = 1


# $trace specifies the interval between trace entries in Mb.
# If this is not set no trace entries will be generated.
set trace = 10


# $weight_bits specifies the number of bits of precision bo be given to the 
# approximate weights.
set weight_bits = 6


# $mcd specifies the commandline arguments for the mg_compression_dict program
set mcd = -S

# $merge_name is the name of the subdirectory under ${MGDATA} where
# the mergeing steps will be performed. There should NOT be an mg
# collection with this name! The default is "MERGE"
set merge = "MERGE"

# Source the parameter file to modify parameters of the build.
if ($?source) then
  source ${source}
endif


# [RPAP - Jan 97: Stem Index Change]
# If do_indexes == 1 then set stem_method = 0
if ($do_indexes) then
  set stem_method = 0
endif


# Generate the collection name.
set coll_name = ${text}

# Generate the directory where merging will take place
if (-e $MGDATA/${merge}) then
else
  mkdir $MGDATA/${merge}
endif

# Generate the base name for the collection.
# [RPAP - Feb 97: Level 3 Merge]
set bname = ${coll_name}

# generate the old, new and merge names in the $merge directory
set oldname = ${merge}/${coll_name}.old
set newname = ${merge}/${coll_name}.new
set mergename = ${merge}/${coll_name}

# build up the command lines for pass 1 and 2 to build a
# dummy collection for the new documents
set pass1 = (-f ${newname} -${invf_level} -m ${invf_mem} -s ${stem_method})
set pass2 = (-f ${newname} -${invf_level} -c ${num_chunks})

if ($strip_sgml) then
  set pass1 = (${pass1} -G)
  set pass2 = (${pass2} -G)
endif

if ($?trace) then
  set pass1 = (${pass1} -t ${trace})
  set pass2 = (${pass2} -t ${trace})
endif

# Note that a -T1 pass isnt done since the old .text.stats file is used
# as the compression model for compressing the new documents.

# [RPAP - Feb 97: Level 3 Merge]
# Do not perform -T2 pass if text_pass == 0
if ($text_pass) then
  set pass2 = (${pass2} -T2)
endif

set pass1 = (${pass1} -${invf_method}1)
set pass2 = (${pass2} -${invf_method}2)


if ($?trace_name) then
  set pass1 = (${pass1} -n ${trace_name})
  set pass2 = (${pass2} -n ${trace_name})
endif
  
if ($?comp_stats) then
  set pass2 = (${pass2} -C ${comp_stats})
endif

########################################################################
# Here is where mgmerge goes to work.

echo "--------------------------------------------------------------"
echo "`uname -n`, `date`"
echo "MGMERGE collection: ${bname}"
echo "--------------------------------------------------------------"
echo "FIRST PHASE: build new collection from new documents"
echo "--------------------------------------------------------------"

# move and rename the text dictionary file
# [RPAP - Feb 97: Level 3 Merge] - only if performing text pass
if ($text_pass) then
  mv $MGDATA/${bname}.text.dict $MGDATA/${newname}.text.dict
endif

if ($pipe) then
  if ("$complex" != "") then
    echo ">> $get $text -init"
    $get $text -init
    if ("$status" != "0") exit 1 
    echo "-----------------------------------"
  endif  
endif

if ($pipe) then
  if ($?pass1filter) then
    echo ">> $get $text $complex | $pass1filter | mg_passes ${pass1}"
    $get $text $complex| $pass1filter | $bindir/mg_passes ${pass1}
    if ("$status" != "0") exit 1 
  else
    echo ">> $get $text $complex | mg_passes ${pass1}"
    $get $text $complex| $bindir/mg_passes ${pass1}
    if ("$status" != "0") exit 1 
  endif
else
  echo ">> mg_passes ${pass1} ${input_files}"
  $bindir/mg_passes ${pass1} ${input_files}
  if ("$status" != "0") exit 1 
endif
echo "-----------------------------------"

echo "mg_perf_hash_build -f ${newname}"
$bindir/mg_perf_hash_build -f ${newname}
if ("$status" != "0") exit 1 
echo "-----------------------------------"

if ($pipe) then
  if ($?pass2filter) then
    echo ">> $get $text $complex | $pass2filter | mg_passes ${pass2}"
    $get $text $complex | $pass2filter | $bindir/mg_passes ${pass2}
    if ("$status" != "0") exit 1 
  else
    echo ">> $get $text $complex | mg_passes ${pass2}"
    $get $text $complex | $bindir/mg_passes ${pass2}
    if ("$status" != "0") exit 1 
  endif
else
  echo ">> mg_passes ${pass2} ${input_files}"
  $bindir/mg_passes ${pass2} ${input_files}
  if ("$status" != "0") exit 1 
endif
echo "-----------------------------------"

if ($pipe) then
  if ("$complex" != "") then
    echo "-----------------------------------"
    echo ">> $get $text -cleanup"
    $get $text -cleanup
    if ("$status" != "0") exit 1 
  endif
endif

echo "---------------------------------------"
date
echo "--------------------------------------------------------------"
echo "SECOND PHASE: merge the two collections"
echo "--------------------------------------------------------------"

# move the appropriate files from the base directory to the merge dir
echo "(moving files....)"
# [RPAP - Feb 97: Level 3 Merge]
# Move files if doing text pass
if ($text_pass) then
  mv $MGDATA/${bname}.text       $MGDATA/${oldname}.text
  mv $MGDATA/${bname}.text.idx   $MGDATA/${oldname}.text.idx
else
# Need this file if building weights
  cp -d $MGDATA/${bname}.text.idx   $MGDATA/${mergename}.text.idx
endif
mv $MGDATA/${bname}.invf       $MGDATA/${oldname}.invf
mv $MGDATA/${bname}.invf.idx   $MGDATA/${oldname}.invf.idx
mv $MGDATA/${bname}.invf.dict  $MGDATA/${oldname}.invf.dict
# the old weight file may be used for the new file
mv $MGDATA/${bname}.weight     $MGDATA/${mergename}.weight
# EOC: the old invf.paragraph file may be used for the new file for Level 3 Invf
if (-e $MGDATA/${bname}.invf.paragraph) then
  mv $MGDATA/${bname}.invf.paragraph  $MGDATA/${mergename}.invf.paragraph
endif
echo "---------------------------------------"

# [RPAP - Feb 97: Level 3 Merge]
# Only merge text if doing text pass
if ($text_pass) then
  echo ">> mg_text_merge -f ${mergename}"
  $bindir/mg_text_merge -f $mergename
  echo "---------------------------------------"
endif

echo ">> mg_invf_merge ${slow_merge}${guess_weights} -f ${mergename}"
$bindir/mg_invf_merge ${slow_merge}${guess_weights} -f ${mergename}
echo "---------------------------------------"

#remove .weight file, so a new one will be created by mg_weights_build
if ("$guess_weights" == "") then
  echo "rm ${mergename}.weight"
  rm $MGDATA/${mergename}.weight
endif

# mv $MGDATA/${mergename}.weight $MGDATA/${mergename}.w2

echo ">> mg_weights_build -f ${mergename} -b ${weight_bits}"
$bindir/mg_weights_build -f ${mergename} -b ${weight_bits}
if ("$status" != "0") exit 1 

echo "---------------------------------------"

echo ">> mg_invf_dict -f ${mergename} -b 4096"
$bindir/mg_invf_dict -f ${mergename} -b 4096

# [RPAP - Jan 97: Stem Index Change]
if ($do_indexes) then
  echo "-----------------------------------"

  echo "mg_stem_idx -f ${mergename} -b 4096 -s1"
  $bindir/mg_stem_idx -f ${mergename} -b 4096 -s1
  if ("$status" != "0") exit 1 

  echo ""

  echo "mg_stem_idx -f ${mergename} -b 4096 -s2"
  $bindir/mg_stem_idx -f ${mergename} -b 4096 -s2
  if ("$status" != "0") exit 1 

  echo ""

  echo "mg_stem_idx -f ${mergename} -b 4096 -s3"
  $bindir/mg_stem_idx -f ${mergename} -b 4096 -s3
  if ("$status" != "0") exit 1 
endif

echo "--------------------------------------------------------------"

# move files back to base dir
# [RPAP - Feb 97: Level 3 Merge]
# mv the text files if doing text pass
if ($text_pass) then
  mv $MGDATA/${oldname}.text         $MGDATA/${bname}.text
  mv $MGDATA/${mergename}.text.idx   $MGDATA/${bname}.text.idx
  mv $MGDATA/${newname}.text.dict    $MGDATA/${bname}.text.dict
endif
mv $MGDATA/${mergename}.invf       $MGDATA/${bname}.invf
mv $MGDATA/${mergename}.invf.idx   $MGDATA/${bname}.invf.idx
mv $MGDATA/${mergename}.invf.dict  $MGDATA/${bname}.invf.dict
mv $MGDATA/${mergename}.invf.dict.blocked  $MGDATA/${bname}.invf.dict.blocked

# [RPAP - Jan 97: Stem Index Change]
if ($do_indexes) then
  mv $MGDATA/${mergename}.invf.dict.blocked.1 $MGDATA/${bname}.invf.dict.blocked.1
  mv $MGDATA/${mergename}.invf.dict.blocked.2 $MGDATA/${bname}.invf.dict.blocked.2
  mv $MGDATA/${mergename}.invf.dict.blocked.3 $MGDATA/${bname}.invf.dict.blocked.3
endif

mv $MGDATA/${mergename}.weight     $MGDATA/${bname}.weight
mv $MGDATA/${mergename}.weight.approx $MGDATA/${bname}.weight.approx
mv $MGDATA/${mergename}.text.idx.wgt $MGDATA/${bname}.text.idx.wgt

# EOC: mv .invf.paragraph file if exists
if (-e $MGDATA/${mergename}.invf.paragraph) then
  mv $MGDATA/${mergename}.invf.paragraph   $MGDATA/${bname}.invf.paragraph
endif

# remove old files in the MERGE directory
rm -f $MGDATA/${merge}/*

echo "mgstat -f ${bname} -E"
$bindir/mgstat -f ${bname} -E
if ("$status" != "0") exit 1 

echo "--------------------------------------------------------------"
echo "`uname -n`, `date`"
echo "--------------------------------------------------------------"

echo ""
