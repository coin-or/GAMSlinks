# Copyright (C) GAMS Development 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 598 2008-12-01 19:14:17Z stefan $
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

AM_CPPFLAGS = \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSParsers` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)`

osil2gms_LDADD += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la
osrl2trace_LDADD += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la
 
osil2gms_DEPENDENCIES += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la
osrl2trace_DEPENDENCIES += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la

LIBS += `cat $(OSOBJDIR)/os_addlibs.txt`

osil2gms_LDADD += $(COINUTILSOBJDIR)/src/libCoinUtils.la
osrl2trace_LDADD += $(COINUTILSOBJDIR)/src/libCoinUtils.la
LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

osil2gms_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
osrl2trace_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

include ../../../BuildTools/Makemain.inc
