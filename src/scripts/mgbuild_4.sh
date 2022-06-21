#!/bin/csh -f
###########################################################################
#
# mgbuild_4.sh -- Script used to build mg text collection in 4 passes.
# Copyright (C) 1994  Neil Sharman; Modified by tes@kbs 1995
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
# Does an mgbuild in 4 passes instead of 2 passes.
# This is useful if one does not have as much RAM and a large collection.
#

set complex = ""

# ************ parse the command line arguments ************

while ($#argv >= 1)
  if ("$1" == "-s" || "$1" == "--source") then
    shift
    if ($#argv >= 1) then
      set source = $1
      shift
    endif
  else if ("$1" == "-g" || "$1" == "--get") then
    shift
    if ($#argv >= 1) then
      set get = $1
      shift
    endif
  else if ("$1" == "-c" || "$1" == "--complex") then
    set complex = "-text"
    shift
  else
    if (! $?text ) then
      set text = $1
    endif
    shift
  endif
    
end

if (! $?text ) then
  set prog = $0
  echo "USAGE:"
  echo "     "$prog:t" [-s config-script] [-g get-program] [-c] source"
  echo ""
  echo " The config-script is only needed if a non-standard build is required."
  echo " The get-program defaults to mg_get if not specified."
  exit 1	
endif

set bindir = `which $0` # find which one we are executing
set bindir = $bindir:h # use it as the bindir path

# ************ set up shell variables ************
# Override these by providing a script to source using the -s option

# if $pipe == 1 then pipe in the source text using $get and $text otherwise
# read the source text directly from the file names specified in $input_files
set pipe = 1

set get_options = ""


if (! $?get ) then
  set complex = "-text"
  set get = $bindir/mg_get $get_options
endif


if (-e $MGDATA/${text}.chunks) then
  set input_files = `cat $MGDATA/${text}.chunks`
endif

# Set the stemming method
#   Bit 0 = case folding
#   Bit 1 = S stemmer
set stem_method = 3

# whether to build weights or not
set do_weights = 1

# If do_pass1 == 1 then do pass1 of building a mg database. 
# Otherwise don't do pass1.
# Text 1st pass
set do_pass1 = 1

# If do_pass2 == 1 then do pass2 of building a mg database. 
# Otherwise don't do pass2.
# Text 2nd pass
set do_pass2 = 1

# If do_pass3 == 1 then do pass3 of building a mg database. 
# Otherwise don't do pass3.
# Invf 1st pass
set do_pass3 = 1

# If do_pass4 == 1 then do pass4 of building a mg database. 
# Otherwise don't do pass4.
# Invf 2nd pass
set do_pass4 = 1

# Buffer size used by mg_passes
# Should be big enough to hold largest document
# In Kilobytes
set buf_size = 3072

# $invf_mem specifies the amount of memory to use for the pass2 inversion
set invf_mem = 5

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
set mcd = -C


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

# ************ set up pass parameters ************

if ($do_pass1) then
  set pass1_text = (-f ${bname} -T1)
endif

if ($do_pass2) then
  set pass2_text = (-f ${bname} -T2)
endif

if ($do_pass3) then
  set pass3_invf = (-f ${bname} -${invf_level} -m ${invf_mem} -s ${stem_method})
  set pass3_invf = (${pass3_invf} -N1)
endif

if ($do_pass4) then
  set pass4_invf = (-f ${bname} -${invf_level} -N2)
endif

if ($strip_sgml) then
  set pass1_text = (${pass1_text} -G)
  set pass2_text = (${pass2_text} -G)
  set pass3_invf = (${pass3_invf} -G)
  set pass4_invf = (${pass4_invf} -G)
endif

if ($?trace) then
  set pass1_text = (${pass1_text} -t ${trace})
  set pass2_text = (${pass2_text} -t ${trace})
  set pass3_invf = (${pass3_invf} -t ${trace})
  set pass4_invf = (${pass4_invf} -t ${trace})
endif

if ($?trace_name) then
  set pass1_text = (${pass1_text} -n ${trace_name})
  set pass2_text = (${pass2_text} -n ${trace_name})
  set pass3_invf = (${pass3_invf} -n ${trace_name})
  set pass4_invf = (${pass4_invf} -n ${trace_name})
endif

if ($buf_size) then
  set pass1_text = (${pass1_text} -b ${buf_size})
  set pass2_text = (${pass2_text} -b ${buf_size})
  set pass3_invf = (${pass3_invf} -b ${buf_size})
  set pass4_invf = (${pass4_invf} -b ${buf_size})
endif
  
if ($?comp_stats) then
  set pass2_text = (${pass2_text} -C ${comp_stats})
endif

echo "-----------------------------------"
echo "`uname -n`, `date`"
echo "${text} --> ${bname}"
echo "-----------------------------------"

# ************ init get ************

if ($pipe) then
  if ("$complex" != "") then
    echo "$get $text -init"
    $get $text -init
    if ("$status" != "0") exit 1 
    echo "-----------------------------------"
  endif  
endif

# ************************************* T E X T **********************************

# ************ pass 1_text ************

if (${do_pass1}) then
  if ($pipe) then
    if ($?pass1filter) then
      echo "$get $text $complex | $pass1filter | mg_passes ${pass1_text}"
      $get $text $complex| $pass1filter | $bindir/mg_passes ${pass1_text}
      if ("$status" != "0") exit 1 
    else
      echo "$get $text $complex | mg_passes ${pass1_text}"
      $get $text $complex| $bindir/mg_passes ${pass1_text}
      if ("$status" != "0") exit 1 
    endif
  else
    echo mg_passes ${pass1_text} ${input_files}
    $bindir/mg_passes ${pass1_text} ${input_files}
    if ("$status" != "0") exit 1 
  endif
  echo "-----------------------------------"
endif

# ************ compression dict  ************
if ($do_pass2) then 
  echo "mg_compression_dict -f ${bname} ${mcd}"
  $bindir/mg_compression_dict -f ${bname} ${mcd}
  if ("$status" != "0") exit 1 
  echo "-----------------------------------"
endif

# ************ pass 2_text ************
if (${do_pass2}) then
  if ($pipe) then
    if ($?pass2filter) then
      echo "$get $text $complex | $pass2filter | mg_passes ${pass2_text}"
      $get $text $complex| $pass2filter | $bindir/mg_passes ${pass2_text}
      if ("$status" != "0") exit 1 
    else
      echo "$get $text $complex | mg_passes ${pass2_text}"
      $get $text $complex| $bindir/mg_passes ${pass2_text}
      if ("$status" != "0") exit 1 
    endif
  else
    echo mg_passes ${pass2_text} ${input_files}
    $bindir/mg_passes ${pass2_text} ${input_files}
    if ("$status" != "0") exit 1 
  endif
  echo "-----------------------------------"
endif

# ************************************* I N V F **********************************

# ************ pass 3_invf ************
if (${do_pass3}) then
  if ($pipe) then
    if ($?pass3filter) then
      echo "$get $text $complex | $pass3filter | mg_passes ${pass3_invf}"
      $get $text $complex| $pass3filter | $bindir/mg_passes ${pass3_invf}
      if ("$status" != "0") exit 1 
    else
      echo "$get $text $complex | mg_passes ${pass3_invf}"
      $get $text $complex| $bindir/mg_passes ${pass3_invf}
      if ("$status" != "0") exit 1 
    endif
  else
    echo mg_passes ${pass3_invf} ${input_files}
    $bindir/mg_passes ${pass3_invf} ${input_files}
    if ("$status" != "0") exit 1 
  endif
  echo "-----------------------------------"
endif

# ************ perfect hash  ************
if ($do_pass4) then
  echo "mg_perf_hash_build -f ${bname}"
  $bindir/mg_perf_hash_build -f ${bname}
  if ("$status" != "0") exit 1 

  echo "-----------------------------------"
endif

# ************ pass 4_invf ************
if (${do_pass4}) then
  if ($pipe) then
    if ($?pass4filter) then
      echo "$get $text $complex | $pass4filter | mg_passes ${pass4_invf}"
      $get $text $complex| $pass4filter | $bindir/mg_passes ${pass4_invf}
      if ("$status" != "0") exit 1 
    else
      echo "$get $text $complex | mg_passes ${pass4_invf}"
      $get $text $complex| $bindir/mg_passes ${pass4_invf}
      if ("$status" != "0") exit 1 
    endif
  else
    echo mg_passes ${pass4_invf} ${input_files}
    $bindir/mg_passes ${pass4_invf} ${input_files}
    if ("$status" != "0") exit 1 
  endif
  echo "-----------------------------------"
endif


# ************ build invf dictionary ************

if ($do_pass4) then
  echo "mg_invf_dict -f ${bname} -b 4096"
  $bindir/mg_invf_dict -f ${bname} -b 4096
  if ("$status" != "0") exit 1 
  echo "-----------------------------------"
endif

# ************ build weights ************

if ($do_pass4 && $do_weights) then
  echo "mg_weights_build -f ${bname} -b ${weight_bits} "
  $bindir/mg_weights_build -f ${bname} -b ${weight_bits}
  if ("$status" != "0") exit 1 
  echo "-----------------------------------"
endif

# *******************************************************************************


# ************ print out statistics ************
if ($do_pass1 && $do_pass2 && $do_pass3 && $do_pass4) then
  echo "mgstat -f ${bname} -E"
  $bindir/mgstat -f ${bname} -E
  if ("$status" != "0") exit 1 
endif


# ************ cleanup get ************

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

if ($do_pass2) then
  echo "-- The fast-loading compression dictionary has not been built."
  echo "-- If you wish to build it, execute the following command:"
  echo "-- "$bindir/mg_fast_comp_dict -f ${bname}

  echo "-----------------------------------"
endif

echo ""

