# Copyright (C) 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = 

lib_LTLIBRARIES = libLinearSolverLoader.la
libLinearSolverLoader_la_SOURCES = LibraryHandler.c

if COIN_ENABLE_HSLLOADER
  libLinearSolverLoader_la_SOURCES += HSLLoader.c
endif

if COIN_ENABLE_PARDISOLOADER
  libLinearSolverLoader_la_SOURCES += PardisoLoader.c
endif

AM_CPPFLAGS = 
#-I`$(CYGPATH_W) $(srcdir)/..`

libLinearSolverLoader_la_LIBADD = 
libLinearSolverLoader_la_DEPENDENCIES =  

#LIBS += 

#libLinearSolverLoader_la_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = 

include ../../BuildTools/Makemain.inc
