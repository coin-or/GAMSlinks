# Copyright (C) GAMS Development and others
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
libGamsHighs_la_LIBADD = $(GAMSHIGHS_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = opthighs
opthighs_SOURCES = opthighs.cpp ../utils/GamsOptionsSpecWriter.cpp
opthighs_LDADD = $(GAMSHIGHS_LFLAGS)

pkgdata_DATA = opthighs.def
doc_DATA = opthighs.md
opthighs.def opthighs.md : opthighs$(EXEEXT)
	./opthighs$(EXEEXT)
