# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  $(GAMSSOPLEX_CFLAGS)

lib_LTLIBRARIES = libGamsSoplex.la
libGamsSoplex_la_SOURCES = GamsSoPlex.cpp \
  ../utils/GamsLicensing.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsSoplex_la_LIBADD  = $(GAMSSOPLEX_LFLAGS)
libGamsSoplex_la_LDFLAGS = $(LT_LDFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc
