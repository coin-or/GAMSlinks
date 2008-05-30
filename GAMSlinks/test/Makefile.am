# Copyright (C) GAMS Development and others 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

bin_PROGRAMS = adbench
adbench_SOURCES = adbench.cpp ../src/GamsIO/SmagExtra.c

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) $(FORTRAN_NAMEMANGLING_CFLAG) \
  -I`$(CYGPATH_W) $(srcdir)/../src` \
  -I`$(CYGPATH_W) $(srcdir)/../src/GamsIO`


#adbench_LDADD = ../src/GamsIO/libSmagIO.la
#adbench_DEPENDENCIES = ../src/GamsIO/libSmagIO.la $(SMAG_LIBS)
adbench_LDADD = $(GAMSIO_OBJDIR)/libGamsGDX.la
adbench_DEPENDENCIES = $(GAMSIO_OBJDIR)/libGamsGDX.la $(SMAG_LIBS)

LIBS += `cat $(GAMSLINKSOBJDIR)/smagio_addlibs.txt`

adbench_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

test:
	chmod u+x ./run_tests
	sh -c ./run_tests

unitTest: test

.PHONY: test unitTest

DISTCLEANFILES = 

include ../../BuildTools/Makemain.inc
