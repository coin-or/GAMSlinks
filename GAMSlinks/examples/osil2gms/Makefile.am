# Copyright (C) GAMS Development 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

bin_PROGRAMS = osil2gms osrl2trace

osil2gms_SOURCES      = osil2gms.cpp
osil2gms_LDADD        =
osil2gms_DEPENDENCIES =

osrl2trace_SOURCES      = osrl2trace.cpp 
osrl2trace_LDADD        =
osrl2trace_DEPENDENCIES =

AM_CPPFLAGS = $(OS_CFLAGS)

osil2gms_DEPENDENCIES += $(OS_DEPENDENCIES)
osrl2trace_DEPENDENCIES += $(OS_DEPENDENCIES)

LIBS += $(OS_LIBS)

#osil2gms_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
#osrl2trace_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

include ../../BuildTools/Makemain.inc
