# Generated automatically from Makefile.in by configure.
###########################################################################
#
# Makefile.in -- Makefile for the MG system
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
#       @(#)Makefile.in	1.10 22 Mar 1994
#
###########################################################################

MGHOME = ..\..
# for now put executables in indexers/bin
INSTALLDIR = ..\..\..\bin

DEBUG = 0

CC = cl

!IF $(DEBUG)
CFLAGS = -DEBUG -Z7 /errorReport:prompt
LDFLAGS = -DEBUG -Z7 /errorReport:prompt
!ELSE
CFLAGS =
LDFLAGS =
!ENDIF

DEFS = -DHAVE_CONFIG_H -D__WIN32__ -D_LITTLE_ENDIAN -D_CRT_SECURE_NO_DEPRECATE
INCLUDES = -I"." -I"$(MGHOME)" -I"$(MGHOME)\lib"
LIBS = $(MGHOME)\lib\libmglib.lib 
 
COMPILE = $(CC) $(CFLAGS) -c $(DEFS) $(INCLUDES)
LINK = $(CC) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES: .c .obj
.c.obj:
	$(COMPILE) $<
 
o = .obj
e = .exe

 
HEADERS = \
  arithcode.h     codeoffsets.h   marklist.h      sortmarks.h \
  basictypes.h    codesyms.h      match.h         utils.h     \
  bilevel.h       extractor.h     pbmtools.h      model.h


SOURCES = \
  arithcode.c     extractor.c     mgbilevel.c     mgticdump.c     utils.c \
  bilevel.c       felics.c        mgfelics.c      mgticprune.c    mgticstat.c\
  codeoffsets.c   marklist.c      mgtic.c         pbmtools.c      lstevent.c \
  codesyms.c      match.c         mgticbuild.c    sortmarks.c     ppm.c  



MISC          = Makefile.in 

ALLFILES      =	$(SOURCES) $(HEADERS) $(MISC) $(MAN)

DISTFILES     = $(ALLFILES) 

EXEC          = mgfelics$e mgbilevel$e mgtic$e mgticbuild$e mgticprune$e mgticstat$e \
                mgticdump$e

MAN           = mgfelics.1 mgbilevel.1 mgtic.1 mgticbuild.1  mgticprune.1 \
                mgticstat.1 mgticdump.1

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #


all: $(EXEC)


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

FELICS_OBJS = felics$o mgfelics$o utils$o

mgfelics$e: $(FELICS_OBJS)
	$(LINK) $(FELICS_OBJS) $(LIBS)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

BILEVEL_OBJS = \
	arithcode$o bilevel$o extractor$o marklist$o mgbilevel$o \
	pbmtools$o utils$o
            
mgbilevel$e: $(BILEVEL_OBJS)
	$(LINK) $(BILEVEL_OBJS) $(LIBS)

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

TIC_OBJS = \
	mgtic$o extractor$o marklist$o pbmtools$o utils$o match$o \
	sortmarks$o codesyms$o ppm$o lstevent$o arithcode$o codeoffsets$o bilevel$o

mgtic$e: $(TIC_OBJS) 
	$(LINK) $(TIC_OBJS) $(LIBS) 

TICBUILD_OBJS = mgticbuild$o extractor$o marklist$o pbmtools$o utils$o

mgticbuild$e: $(TICBUILD_OBJS)
	$(LINK) $(TICBUILD_OBJS) $(LIBS)
 
TICPRUNE_OBJS = \
	mgticprune$o extractor$o marklist$o pbmtools$o \
	utils$o match$o

mgticprune$e: $(TICPRUNE_OBJS)
	$(LINK) $(TICPRUNE_OBJS) $(LIBS)
 
TICSTAT_OBJS = mgticstat$o extractor$o marklist$o pbmtools$o utils$o

mgticstat$e: $(TICSTAT_OBJS)
	$(LINK) $(TICSTAT_OBJS) $(LIBS) 
 
TICDUMP = mgticdump$o extractor$o marklist$o pbmtools$o utils$o

mgticdump$e: $(TICDUMP)
	$(LINK) $(TICDUMP) $(LIBS) 
 
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

#ansi2knr: ansi2knr.o
#	$(LINK) ansi2knr.o $(LIBS)
#
install: 
uninstall: 

mostlyclean:
	if exist *$o del *$o
	if exist  _*.c del  _*.c
	if exist _*.o  del  _*.o 
	if exist *._c  del *._c
	if exist *._o  del *._o
	if exist core  del  core
	if exist core.* del  core.*
 
clean: mostlyclean
	if exist *$e del *$e
 	if exist *.pdb del *.pdb

distclean: clean
	del ansi2knr
	 
maintainer-clean: distclean
	@echo "This command is intended only for maintainers to use;"
	@echo "rebuilding the deleted files may require special tools."
 
#dist: $(DISTFILES)
#	for file in $(DISTFILES); do \
#	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
#	  || cp -p $(srcdir)/$$file $(distdir); \
#	done
 
#Makefile: Makefile.in ../../config.status
#	cd ../.. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status
 
# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
#.NOEXPORT:
