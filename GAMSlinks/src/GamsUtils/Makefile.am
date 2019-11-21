# Copyright (C) 2009-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

lib_LTLIBRARIES = libGamsUtils.la
libGamsUtils_la_SOURCES = \
  GamsLicensing.c \
  GamsSolveTrace.c \
  $(GAMS_PATH)/apifiles/C/api/optcc.c \
  $(GAMS_PATH)/apifiles/C/api/gmomcc.c \
  $(GAMS_PATH)/apifiles/C/api/gevmcc.c \
  $(GAMS_PATH)/apifiles/C/api/gclgms.c \
  $(GAMS_PATH)/apifiles/C/api/gdxcc.c
libGamsUtils_la_LDFLAGS =

includecoindir = $(includedir)/coin
includecoin_HEADERS = GamsCompatibility.h  GamsLicensing.h GamsNLinstr.h GamsSolveTrace.h

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/..` -I`$(CYGPATH_W) $(GAMS_PATH)/apifiles/C/api` $(GAMSLIBCFLAGS)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I..

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =
