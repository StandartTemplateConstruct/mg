###########################################################################
#
# win32 makefile -- mg\lib
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
 
MGHOME = ..

AR = lib
CC = cl

DEBUG = 0

!IF $(DEBUG)
CFLAGS = -DEBUG -Z7 /errorReport:prompt
ARFLAGS = /DEBUG /Z7 /errorReport:prompt
!ELSE
CFLAGS =
ARFLAGS =
!ENDIF


DEFS = -DQUIET -DHAVE_CONFIG_H -D__WIN32__ -D_LITTLE_ENDIAN -DPARADOCNUM \
       -DSHORT_SUFFIX -D_CRT_SECURE_NO_DEPRECATE

INCLUDES = -I"." -I"$(MGHOME)"

COMPILE = $(CC) $(CFLAGS) -c $(DEFS) $(INCLUDES)
 
.SUFFIXES: .c .obj
.c.obj:
	$(COMPILE) $<

o = .obj
 
HEADERS = \
    bitio_m_random.h   filestats.h        local_strings.h    lovinstem.h \
    bitio_m_stdio.h    getopt.h           memlib.h \
    bitio_gen.h        bitio_mem.h        mgheap.h             messages.h \
    bitio_m.h          bitio_mems.h       huffman.h          timing.h \
    bitio_m_mem.h      bitio_random.h     huffman_mem.h      perf_hash.h \
    bitio_m_mems.h     bitio_stdio.h      huffman_stdio.h    sptree.h \
                       rx.h               pathmax.h          getpagesize.h \
    random.h           win32in.h          simplefrenchstem.h unitool.h \
    longlong.h	

SOURCES = \
    bitio_random.c    mgheap.c          memlib.c \
    bitio_stdio.c     huffman.c         messages.c \
    bitio_gen.c       filestats.c       huffman_mem.c     perf_hash.c \
    bitio_mem.c       getopt.c          huffman_stdio.c   sptree.c \
    bitio_mems.c      getopt1.c         local_strings.c   lovinstem.c \
    timing.c                            rx.c              \
    alloca.c          error.c           xmalloc.c         strstr.c \
    ftruncate.c       strcasecmp.c      random.c          win32in.c \
    simplefrenchstem.c                  unitool.c

OBJECTS =   rx$o \
    bitio_random$o    mgheap$o            memlib$o \
    bitio_stdio$o     huffman$o         messages$o \
    bitio_gen$o       filestats$o       huffman_mem$o     perf_hash$o \
    bitio_mem$o       getopt$o          huffman_stdio$o   sptree$o \
    bitio_mems$o      getopt1$o         local_strings$o   lovinstem$o \
    timing$o           \
    error$o           xmalloc$o \
    random$o          win32in$o         simplefrenchstem$o \
    unitool$o
 
DISTFILES = Makefile.in $(HEADERS) $(SOURCES)
 
all : libmglib.lib

libmglib.lib : $(OBJECTS)
        if exist libmglib.lib del libmglib.lib
	$(AR) /out:libmglib.lib $(ARFLAGS) $(OBJECTS)

install: 

clean:
        if exist *$o del *$o 
        if exist libmglib.lib del libmglib.lib
