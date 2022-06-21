###########################################################################
#
# win32 makefile -- mg
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



AR = lib
CC = cl

MGHOME = $(MAKEDIR)
DEBUG = 0
DLL = 0
DLLDEBUG = 0

MDEFINES = /f win32.mak

OTHERDIRS = lib 

INSTALLDIRS = src\text jni 

MAKECMD = $(MAKE) $(MDEFINES)
!IF $(DEBUG)
MAKECMD = $(MAKECMD) DEBUG=1
!ENDIF
!IF $(DLL)
MAKECMD = $(MAKECMD) DLL=1
!ENDIF
!IF $(DLLDEBUG)
MAKECMD = $(MAKECMD) DLLDEBUG=1
!ENDIF

all: win32.mak $(OTHERDIRS) $(INSTALLDIRS)

java:	FORCE
	cd "java\org\greenstone\mg"
	call winMake.bat
	call winMake.bat install 
	cd "$(MGHOME)"


install: win32.mak $(INSTALLDIRS)

$(OTHERDIRS): FORCE
	cd "$@"
	$(MAKECMD)
	cd "$(MGHOME)"

$(INSTALLDIRS): FORCE
	cd "$@"
	$(MAKECMD)
	$(MAKECMD) install
	cd "$(MGHOME)"

FORCE:
