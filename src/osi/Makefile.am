# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsOsi.la
libGamsOsi_la_SOURCES = GamsOsiHelper.cpp GamsOsi.cpp GamsMessageHandler.cpp \
  ../utils/gmomcc.c ../utils/gevmcc.c
libGamsOsi_la_LIBADD  = $(GAMSOSI_LFLAGS)
libGamsOsi_la_LDFLAGS = $(LT_LDFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc
