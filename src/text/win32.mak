###########################################################################
#
# win32 makefile -- mg\src\text
# A component of the Greenstone digital library software
# from the New Zealand Digital Library Project at the
# University of Waikato, New Zealand.
#
# Copyright (C) 1999  The New Zealand Digital Library Project
#
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
###########################################################################
 
MGHOME = ..\..
# for now put executables in indexers/bin
INSTALLDIR = ..\..\..\bin

DEBUG = 0

AR = lib
CC = cl

!IF $(DEBUG)
CFLAGS = -DEBUG -Z7 /errorReport:prompt
LDFLAGS = -DEBUG -Z7 /errorReport:prompt
!ELSE
CFLAGS =
LDFLAGS = 
!ENDIF


DEFS = -DNZDL -DQUIET -DSHORT_SUFFIX -DPARADOCNUM -DHAVE_CONFIG_H \
       -D__WIN32__ -D_LITTLE_ENDIAN -D_CRT_SECURE_NO_DEPRECATE
INCLUDES = -I"." -I"$(MGHOME)" -I"$(MGHOME)\lib"
COMPILE = $(CC) $(CFLAGS) -c $(DEFS) $(INCLUDES)

LINK = $(CC) $(LDFLAGS)

LIBS = "$(MGHOME)\lib\libmglib.lib"

.SUFFIXES: .c .cpp .obj
.c.obj:
	$(COMPILE) $<
.cpp.obj:
	$(COMPILE) $<

o = .obj
e = .exe

HEADERS = \
  backend.h          conditions.h       locallib.h         term_lists.h \
  bool_optimiser.h   environment.h      mg.h               text.h       \
  bool_parser.h      globals.h          mg_errors.h        text_get.h   \
  bool_query.h       hash.h             mg_files.h         warranty.h   \
  bool_tree.h        help.mg.h          mg_merge.h         weights.h    \
  build.h            invf.h             read_line.h        words.h      \
  commands.h         invf_get.h         stem_search.h                   \
  comp_dict.h        lists.h            stemmer.h          query_term_list.h

SOURCES = \
  backend.c               locallib.c              mg_weights_build.c   \
  bool_optimiser.c        mg.special.c            mgdictlist.c         \
  bool_parser.c           mg_compression_dict.c   mgquery.c            \
  bool_query.c            mg_errors.c             mgstat.c             \
  bool_tester.c           mg_fast_comp_dict.c     mg_decompress_text.c \
  bool_tree.c             mg_files.c              query.docnums.c      \
  commands.c              mg_hilite_words.c       query.ranked.c       \
  comp_dict.c             mg_invf_dict.c          read_line.c          \
  environment.c           mg_invf_dump.c          stem_search.c        \
  mg_invf_merge.c         stemmer.c               mg_stem_idx.c        \
  mg_invf_rebuild.c       term_lists.c            mgstemidxlist.c      \
  invf_get.c              mg_passes.c             text.pass1.c         \
  ivf.pass1.c             mg_perf_hash_build.c    text.pass2.c         \
  ivf.pass2.c             mg_text_estimate.c      text_get.c           \
  lists.c                 mg_text_merge.c         weights.c            \
  mgpass.c                query_term_list.c       words.c              \
  mg_passes_4jni.c

EXEC = \
  mgquery$e mg_weights_build$e mgstat$e \
  mg_invf_dump$e     mg_invf_dict$e \
  mg_invf_rebuild$e     mgdictlist$e \
  mg_passes$e     mg_perf_hash_build$e \
  mg_compression_dict$e     mg_text_estimate$e \
  mg_fast_comp_dict$e \
  mg_hilite_words$e     mg_invf_merge$e \
  mg_text_merge$e     bool_tester$e  mgpass$e \
  mg_stem_idx$e  mgstemidxlist$e \
  mg_decompress_text$e 

all: $(EXEC) libmgtext.lib libmgpass.lib

HILITE_OBJS = mg_hilite_words$o stemmer$o locallib$o words$o environment$o
mg_hilite_words$e: $(HILITE_OBJS)
	$(LINK) $(HILITE_OBJS) $(LIBS)

BOOL_OBJS = bool_tester$o bool_tree$o bool_parser$o bool_optimiser$o  \
	    term_lists$o stemmer$o stem_search$o mg_errors$o \
	    query_term_list$o words$o environment$o

bool_tester$e: $(BOOL_OBJS)
	$(LINK) $(BOOL_OBJS) $(LIBS)

QUERY_OBJS = \
	mgquery$o locallib$o lists$o \
	query.ranked$o query.docnums$o stem_search$o \
	environment$o commands$o weights$o text_get$o stemmer$o \
	read_line$o mg_errors$o backend$o invf_get$o \
	term_lists$o bool_tree$o bool_parser$o bool_optimiser$o bool_query$o \
	query_term_list$o words$o
mgquery$e: $(QUERY_OBJS)
	$(LINK) $(READLINE) $(QUERY_OBJS) $(READLINE_LIBS) $(LIBS)

commands$o : help.mg.h warranty.h conditions.h

PASSES_OBJS = \
	mg_passes$o text.pass1$o comp_dict$o stemmer$o \
	text.pass2$o locallib$o \
	ivf.pass1$o ivf.pass2$o mg.special$o mg_files$o \
	words$o environment$o
mg_passes$e: $(PASSES_OBJS)
	$(LINK) $(PASSES_OBJS) $(LIBS)

PASS_OBJS = \
	 mgpass$o words$o text.pass1$o comp_dict$o stemmer$o \
	text.pass2$o locallib$o \
	ivf.pass1$o ivf.pass2$o mg.special$o mg_files$o environment$o
mgpass$e: $(PASS_OBJS)
	$(LINK) $(PASS_OBJS) $(LIBS)

LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib

BUILDER_OBJS = \
	builder$o
builder$e: $(BUILDER_OBJS)
	$(LINK) $(BUILDER_OBJS) $(LIBS) $(LINK32_FLAGS)

WEIGHTS_BUILD_OBJS = mg_weights_build$o mg_files$o
mg_weights_build$e: $(WEIGHTS_BUILD_OBJS)
	$(LINK) $(WEIGHTS_BUILD_OBJS) $(LIBS)

COMP_DICT_OBJS = mg_compression_dict$o mg_files$o locallib$o
mg_compression_dict$e: $(COMP_DICT_OBJS)
	$(LINK) $(COMP_DICT_OBJS) $(LIBS)

FAST_COMP_OBJS = mg_fast_comp_dict$o mg_files$o locallib$o
mg_fast_comp_dict$e: $(FAST_COMP_OBJS)
	$(LINK) $(FAST_COMP_OBJS) $(LIBS)

TEXT_EST_OBJS = mg_text_estimate$o locallib$o comp_dict$o mg_files$o
mg_text_estimate$e: $(TEXT_EST_OBJS)
	$(LINK) $(TEXT_EST_OBJS) $(LIBS)

STAT_OBJS = mgstat$o mg_files$o locallib$o
mgstat$e: $(STAT_OBJS)
	$(LINK) $(STAT_OBJS) $(LIBS)

INVF_DUMP_OBJS = mg_invf_dump$o locallib$o mg_files$o
mg_invf_dump$e: $(INVF_DUMP_OBJS)
	$(LINK) $(INVF_DUMP_OBJS) $(LIBS)

INVF_DICT_OBJS = mg_invf_dict$o mg_files$o locallib$o
mg_invf_dict$e: $(INVF_DICT_OBJS)
	$(LINK) $(INVF_DICT_OBJS) $(LIBS)

INVF_REB_OBJS = mg_invf_rebuild$o locallib$o
mg_invf_rebuild$e: $(INVF_REB_OBJS)
	$(LINK) $(INVF_REB_OBJS) $(LIBS)

DICTLIST_OBJS = mgdictlist$o locallib$o
mgdictlist$e: $(DICTLIST_OBJS)
	$(LINK) $(DICTLIST_OBJS) $(LIBS)

PERF_HASH_OBJS = mg_perf_hash_build$o mg_files$o
mg_perf_hash_build$e: $(PERF_HASH_OBJS)
	$(LINK) $(PERF_HASH_OBJS) $(LIBS)

STEM_IDX_OBJS = mg_stem_idx$o mg_files$o stemmer$o locallib$o \
                mg_errors$o term_lists$o
mg_stem_idx$e: $(STEM_IDX_OBJS)
	$(LINK) $(STEM_IDX_OBJS) $(LIBS)

STEM_IDX_LIST_OBJS = mgstemidxlist$o mg_files$o
mgstemidxlist$e: $(STEM_IDX_LIST_OBJS)
	$(LINK) $(STEM_IDX_LIST_OBJS) $(LIBS)

DECOMPRESS_TEXT_OBJS = mg_decompress_text$o query_term_list$o mg_files$o \
		       mg_errors$o \
                       text_get$o locallib$o backend$o stem_search$o \
                       term_lists$o stemmer$o weights$o invf_get$o lists$o
mg_decompress_text$e: $(DECOMPRESS_TEXT_OBJS)
	$(LINK) $(DECOMPRESS_TEXT_OBJS) $(LIBS)

TEXT_MERGE_OBJS = mg_text_merge$o mg_files$o locallib$o
mg_text_merge$e: $(TEXT_MERGE_OBJS)
	$(LINK) $(TEXT_MERGE_OBJS) $(LIBS)

INVF_MERGE_OBJS = mg_invf_merge$o mg_files$o locallib$o
mg_invf_merge$e: $(INVF_MERGE_OBJS)
	$(LINK) $(INVF_MERGE_OBJS) $(LIBS)

LIB_OBJS = \
	query_term_list$o \
	locallib$o lists$o \
	query.ranked$o query.docnums$o stem_search$o \
	environment$o commands$o weights$o text_get$o stemmer$o \
	read_line$o mg_errors$o backend$o invf_get$o \
	term_lists$o bool_tree$o bool_parser$o bool_optimiser$o \
        bool_query$o words$o

JNI_LIB_OBJS = \
	mg_passes_4jni$o text.pass1$o comp_dict$o stemmer$o \
	text.pass2$o locallib$o  words$o environment$o \
	ivf.pass1$o ivf.pass2$o mg.special$o mg_files$o 


DISTFILES = Makefile.in $(HEADERS) $(SOURCES)

libmgtext.lib : $(LIB_OBJS)
        if exist libmgtext.lib del libmgtext.lib
	$(AR) /out:libmgtext.lib $(LIB_OBJS)

libmgpass.lib : $(JNI_LIB_OBJS)
 	  if exist libmgpass.lib del libmgpass.lib
	$(AR) /out:libmgpass.lib $(JNI_LIB_OBJS)

install: 
	if not exist "$(INSTALLDIR)" mkdir "$(INSTALLDIR)"
	for %%i in ($(EXEC)) do \
	  copy %%i "$(INSTALLDIR)"

clean:
        if exist *$o del *$o 
	if exist *$e del *$e
        if exist libmgtext.lib del libmgtext.lib
        if exist libmgpass.lib del libmgpass.lib
	if exist *.pdb del *.pdb