# Copyright (C) 2009-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

lib_LTLIBRARIES = libGamsUtils.la
libGamsUtils_la_SOURCES = GamsOptions.cpp GamsBBTrace.c \
  $(GAMS_VERSION)/optcc.c $(GAMS_VERSION)/gmomcc.c $(GAMS_VERSION)/gevmcc.c $(GAMS_VERSION)/gclgms.c $(GAMS_VERSION)/gdxcc.c
libGamsUtils_la_LDFLAGS =

includecoindir = $(includedir)/coin
includecoin_HEADERS = GamsCompatibility.h GamsOptions.hpp GamsNLinstr.h GamsBBTrace.h

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/..` -I`$(CYGPATH_W) $(srcdir)/$(GAMS_VERSION)` $(GAMSLIBCFLAGS)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I..

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =
