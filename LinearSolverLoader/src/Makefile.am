# Copyright (C) 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = 

lib_LIBRARIES = libLinearSolverLoader.a
libLinearSolverLoader_a_SOURCES = LibraryHandler.c

if COIN_ENABLE_HSLLOADER
  libLinearSolverLoader_a_SOURCES += HSLLoader.c
endif

AM_CPPFLAGS = 
#-I`$(CYGPATH_W) $(srcdir)/..`

libLinearSolverLoader_a_LIBADD = 
libLinearSolverLoader_a_DEPENDENCIES =  

#LIBS += 

#libLinearSolverLoader_a_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = 

include ../../BuildTools/Makemain.inc
