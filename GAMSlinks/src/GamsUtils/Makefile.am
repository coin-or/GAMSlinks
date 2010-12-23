# Copyright (C) 2009 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

lib_LTLIBRARIES = libGamsUtils.la
libGamsUtils_la_SOURCES = GamsOptions.cpp GamsNLinstr.c \
  $(GAMS_VERSION)/optcc.c $(GAMS_VERSION)/gdxcc.c $(GAMS_VERSION)/gmomcc.c $(GAMS_VERSION)/gevmcc.c $(GAMS_VERSION)/gclgms.c
libGamsUtils_la_LDFLAGS =

includecoindir = $(includedir)/coin
includecoin_HEADERS = GamsGDX.hpp GamsOptions.hpp GamsNLinstr.h

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/..` -I`$(CYGPATH_W) $(srcdir)/$(GAMS_VERSION)` $(CIA_FLAG)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../../../BuildTools/Makemain.inc
