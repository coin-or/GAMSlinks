# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AM_CPPFLAGS = \
  -I$(srcdir)/../utils \
  $(GAMSOSI_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsOsi.la
libGamsOsi_la_SOURCES = GamsOsiHelper.cpp GamsOsi.cpp GamsMessageHandler.cpp \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsOsi_la_LIBADD  = $(GAMSOSI_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc
