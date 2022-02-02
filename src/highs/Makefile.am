# Copyright (C) GAMS Development and others 2022
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AM_CPPFLAGS = \
  -I$(srcdir)/../utils \
  $(GAMSHIGHS_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsHighs.la
libGamsHighs_la_SOURCES = GamsHighs.cpp \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsHighs_la_LIBADD  = $(GAMSHIGHS_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc
