# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AM_CPPFLAGS = \
  -I$(srcdir)/../utils \
  $(GAMSSOPLEX_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsSoplex.la
libGamsSoplex_la_SOURCES = GamsSoPlex.cpp \
  ../utils/GamsLicensing.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsSoplex_la_LIBADD  = $(GAMSSOPLEX_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optsoplex
optsoplex_SOURCES = optsoplex.cpp ../utils/GamsOptionsSpecWriter.cpp
optsoplex_LDADD = $(GAMSSOPLEX_LFLAGS)

doc_DATA = optsoplex.md
optsoplex.md : optsoplex$(EXEEXT)
	./optsoplex$(EXEEXT)
