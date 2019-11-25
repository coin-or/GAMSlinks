# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  -I"$(GAMS_PATH)/apifiles/C/api" \
  $(GAMSSOPLEX_CFLAGS)

lib_LTLIBRARIES = libGamsSoplex.la
libGamsSoplex_la_SOURCES = GamsSoPlex.cpp
libGamsSoplex_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSSOPLEX_LFLAGS)
libGamsSoplex_la_LDFLAGS = $(LT_LDFLAGS)
