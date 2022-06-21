#!/bin/csh -f
###########################################################################
#
# mgbuild.sh -- Script used to build mg text collection.
# Copyright (C) 1994  Neil Sharman
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
#       @(#)mgbuild.sh	1.9 16 Mar 1994
#
###########################################################################
#  mg.build
# Creates a Full Text Retrieval database
#
#  @(#)mgbuild.sh	1.9 16 Mar 1994
#

set complex = ""

# Parse the command line arguments
while ($#argv >= 1)
  if ("$1" == "-s") then
    shift
    if ($#argv >= 1) then
      set source = $1
      shift
    endif
  else if ("$1" == "-d") then
    shift
    if ($#argv >= 1) then
      set mgdata = $1
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
  echo "     "$prog:t" [-s config-script] [-g get-program] [-d mgdata dir] [-c] source"
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
  set get = $bindir/mg_get
endif

if ($?mgdata) then
  setenv MGDATA $mgdata
endif

if (-e $MGDATA/${text}.chunks) then
  set input_files = `cat $MGDATA/${text}.chunks`
endif

# Set the stemming method
#   Bit 0 = case folding
#   Bit 1 = S stemmer
set stem_method = 3


# If do_text == 1 then do the text component of building a mg database. 
# Otherwise don't do the text component.
set do_text = 1


# If do_invf == 1 then do the inversion component of building a mg database. 
# Otherwise don't do the inversion component.
set do_invf = 1


# If do_pass1 == 1 then do pass1 of building a mg database. 
# Otherwise don't do pass1.
set do_pass1 = 1

# If do_pass2 == 1 then do pass2 of building a mg database. 
# Otherwise don't do pass2.
set do_pass2 = 1


# $invf_mem specifies the amount of memory to use for the pass2 inversion
set invf_mem = 32


# $num_chunks specifies the number of interium chunks of inverted file that
# may be written to disc before a merge into the invf file is done
set num_chunks = 3

# $invf_level specifies the level of the inverted file that will be generated.
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
#### Note: Set -S so novel words can be encoded, as this will happen if
#### mgmerge is used.
set mcd = -S


# Source the parameter file to modify parameters of the build.
if ($?source) then
  source ${source}
endif

# Generate the collection name.
set coll_name = ${text}
if ("$invf_level" == "3") then
  set coll_name = ${text}-p
endif

# Generate the directory for the collection.
if (-e $MGDATA/${coll_name}) then
else
  mkdir $MGDATA/${coll_name}
endif

# Generate the base name for the collection.
set bname = ${coll_name}/${coll_name}


#build up the command lines for pass 1 and 2
set pass1 = (-f ${bname} -${invf_level} -m ${invf_mem} -s ${stem_method})
set pass2 = (-f ${bname} -${invf_level} -c ${num_chunks})

if ($strip_sgml) then
  set pass1 = (${pass1} -G)
  set pass2 = (${pass2} -G)
endif

if ($?trace) then
  set pass1 = (${pass1} -t ${trace})
  set pass2 = (${pass2} -t ${trace})
endif

if ($do_text) then
  set pass1 = (${pass1} -T1)
  set pass2 = (${pass2} -T2)
endif

if ($do_invf) then
  set pass1 = (${pass1} -I1)
  set pass2 = (${pass2} -I2)
endif
  
if ($?trace_name) then
  set pass1 = (${pass1} -n ${trace_name})
  set pass2 = (${pass2} -n ${trace_name})
endif
  
if ($?comp_stats) then
  set pass2 = (${pass2} -C ${comp_stats})
endif

echo "-----------------------------------"
echo "`uname -n`, `date`"
echo "${text} --> ${bname}"
echo "-----------------------------------"
if ($pipe) then
  if ("$complex" != "") then
    echo "$get $text -init"
    $get $text -init
    if ("$status" != "0") exit 1 
    echo "-----------------------------------"
  endif  
endif

if (${do_pass1}) then
  if ($pipe) then
    if ($?pass1filter) then
      echo "$get $text $complex | $pass1filter | mg_passes ${pass1}"
      $get $text $complex| $pass1filter | $bindir/mg_passes ${pass1}
      if ("$status" != "0") exit 1 
    else
      echo "$get $text $complex | mg_passes ${pass1}"
      $get $text $complex| $bindir/mg_passes ${pass1}
      if ("$status" != "0") exit 1 
    endif
  else
    echo mg_passes ${pass1} ${input_files}
    $bindir/mg_passes ${pass1} ${input_files}
    if ("$status" != "0") exit 1 
  endif
  echo "-----------------------------------"

  echo "mg_perf_hash_build -f ${bname}"
  $bindir/mg_perf_hash_build -f ${bname}
  if ("$status" != "0") exit 1 

  echo "-----------------------------------"
  echo "mg_compression_dict -f ${bname} ${mcd}"
  $bindir/mg_compression_dict -f ${bname} ${mcd}
  if ("$status" != "0") exit 1 
  echo "-----------------------------------"
endif

if (${do_pass2}) then
  if ($pipe) then
    if ($?pass2filter) then
      echo "$get $text $complex | $pass2filter | mg_passes ${pass2}"
      $get $text $complex | $pass2filter | $bindir/mg_passes ${pass2}
      if ("$status" != "0") exit 1 
    else
      echo "$get $text $complex | mg_passes ${pass2}"
      $get $text $complex | $bindir/mg_passes ${pass2}
      if ("$status" != "0") exit 1 
    endif
  else
    echo mg_passes ${pass2} ${input_files}
    $bindir/mg_passes ${pass2} ${input_files}
    if ("$status" != "0") exit 1 
  endif
  echo "-----------------------------------"
endif

echo "mg_weights_build -f ${bname} -b ${weight_bits}"
$bindir/mg_weights_build -f ${bname} -b ${weight_bits}
if ("$status" != "0") exit 1 

echo "-----------------------------------"

echo "mg_invf_dict -f ${bname} -b 4096"
$bindir/mg_invf_dict -f ${bname} -b 4096
if ("$status" != "0") exit 1 

echo "-----------------------------------"

echo "mgstat -f ${bname} -E"
$bindir/mgstat -f ${bname} -E
if ("$status" != "0") exit 1 

if ($pipe) then
  if ("$complex" != "") then
    echo "-----------------------------------"
    echo "$get $text -cleanup"
    $get $text -cleanup
    if ("$status" != "0") exit 1 
  endif
endif

echo "-----------------------------------"
echo "`uname -n`, `date`"
echo "-----------------------------------"

echo "-- The fast-loading compression dictionary has not been built."
echo "-- If you wish to build it, execute the following command:"
echo "-- "$bindir/mg_fast_comp_dict -f ${bname}

echo "-----------------------------------"

echo ""

