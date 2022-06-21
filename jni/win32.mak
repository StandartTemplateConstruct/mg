###########################################################################
#
# win32 makefile -- mg\jni
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
CXXFLAGS = 

!IF $(DEBUG)
CFLAGS = -DEBUG -Z7 /errorReport:prompt
LDFLAGS = -DEBUG -Z7 /errorReport:prompt
!ELSE
CFLAGS =
LDFLAGS = 
!ENDIF

DEFS = -DQUIET -DHAVE_CONFIG_H -D__WIN32__ -D_LITTLE_ENDIAN -DPARADOCNUM \
       -DSHORT_SUFFIX
INCLUDES = -I"$(MGHOME)" -I"$(MGHOME)\lib" -I"$(MGHOME)\src\text" \
           -I"$(JAVA_HOME)\include" -I"$(JAVA_HOME)\include\win32"           

COMPILE = $(CC) $(CFLAGS) -c $(DEFS) $(INCLUDES)
 
.SUFFIXES:
.SUFFIXES: .c .obj
.c.obj:
	$(COMPILE) $<

ANSI2KNR = 
o = .obj
 
HEADERS = \
    MGSearchWrapperImpl.h   org_greenstone_mg_MGSearchWrapper.h  \
    MGRetrieveWrapperImpl.h   org_greenstone_mg_MGRetrieveWrapper.h  \
    org_greenstone_mg_MGPassesWrapper.h


SOURCES = \
    MGSearchWrapperImpl.c MGRetrieveWrapperImpl.c MGPassesWrapperImpl.c

SEARCHOBJECTS = \
    MGSearchWrapperImpl$o $(MGHOME)\src\text\libmgtext.lib \
    $(MGHOME)\lib\libmglib.lib

RETRIEVEOBJECTS = \
    MGRetrieveWrapperImpl$o  $(MGHOME)\src\text\libmgtext.lib \
    $(MGHOME)\lib\libmglib.lib

PASSES_OBJECTS = \
    MGPassesWrapperImpl$o   $(MGHOME)\src\text\libmgpass.lib \
    $(MGHOME)\lib\libmglib.lib

DISTFILES = Makefile.in $(HEADERS) $(SOURCES)
 
all : compile link

compile:
        $(COMPILE) MGSearchWrapperImpl.c MGRetrieveWrapperImpl.c MGPassesWrapperImpl.c

link:
        $(CC) -MD -LD $(SEARCHOBJECTS) -Femgsearchjni.dll
	  $(CC) -MD -LD $(RETRIEVEOBJECTS) -Femgretrievejni.dll
        $(CC) -MD -LD $(PASSES_OBJECTS) -Femgpassjni.dll


install: 

clean:
        if exist *$o del *$o 
	if exist mgsearchjni.dll del mgsearchjni.dll
	if exist mgretrievejni.dll del mgretrievejni.dll
	if exist mgpassjni.dll del mgpassjni.dll
