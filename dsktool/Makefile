###########################################################################
#
#    Dsktool v1.0.5 - LibDsk front end
#
#    Copyright (C) 2005, 2007, 2010  John Elliott <jce@seasip.demon.co.uk>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
###########################################################################
#
# User-amendable settings...
#
# Library paths: wxWidgets
#
WX_INCLUDE=`wx-config --cflags`
WX_LIBS=`wx-config --libs`

#
# Library paths: LibDsk
#
LIBDSK_INCLUDE=
LIBDSK_LIBS=-ldsk
#
# Installation directory
#
INSTALLDIR = /usr/local/bin
#
# The C++ compiler
#
CXX = g++
#
# C++ flags and libraries
#
CXXFLAGS = -O2 -fPIC -fpic $(WX_INCLUDE) $(LIBDSK_INCLUDE)
LIBS = $(WX_LIBS) $(LIBDSK_LIBS)
#
##########################################################################
#
# Below this point it shouldn't be necessary to change anything.
#
REVISION=1.0.5

OBJECTS = AboutBox.o	     dsktool.o     IdentityForm.o  OptionsPage.o \
	  DriveSelectPage.o  MainWindow.o  SerialForm.o    wxprec.o

SRC=AboutBox.cpp DriveSelectPage.cpp dsktool.cpp IdentityForm.cpp \
    MainWindow.cpp OptionsPage.cpp dsktool.rc SerialForm.cpp wxprec.cpp \
    ChangeLog

HEADERS=AboutBox.h DriveSelectPage.h dsktool.h IdentityForm.h \
	MainWindow.h OptionsPage.h resource.h SerialForm.h

MAKEFILES=Makefile dsktool.dsp dsktool.dsw configure

TARBALL= COPYING README.txt $(MAKEFILES) $(SRC) $(HEADERS) 

all:    dsktool

install:        all
	install -m 755 dsktool $(INSTALLDIR)

dsktool:      $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LIBS)

%.o:    %.cxx
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o dsktool

dist:
	mkdir dsktool-$(REVISION)
	mkdir dsktool-$(REVISION)/res
	ln res/* dsktool-$(REVISION)/res
	ln $(TARBALL) dsktool-$(REVISION)
	tar chvfz dsktool-$(REVISION).tar.gz dsktool-$(REVISION)/*
	cd dsktool-$(REVISION) && rm $(TARBALL) && rm -rf res
	rmdir dsktool-$(REVISION)
