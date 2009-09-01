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
libGamsUtils_la_SOURCES = GamsOptions.cpp GamsNLinstr.c optcc.c gdxcc.c gmomcc.c gevmcc.c
libGamsUtils_la_LDFLAGS =
#libGamsUtils_la_LIBADD = $(GAMSIO_OBJDIR)/libGamsJoat.la $(GAMSIO_OBJDIR)/libGamsGDX.la $(GAMSIO_OBJDIR)/libGamsOptions.la

includecoindir = $(includedir)/coin
includecoin_HEADERS = GamsGDX.hpp GamsOptions.hpp GamsNLinstr.h

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/..` $(CIA_FLAG)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../../../BuildTools/Makemain.inc
