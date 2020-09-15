# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AM_CPPFLAGS = \
  -I$(srcdir)/../osi \
  -I$(srcdir)/../utils \
  $(GAMSCBC_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsCbc.la
libGamsCbc_la_SOURCES = GamsCbc.cpp \
  ../osi/GamsOsiHelper.cpp ../osi/GamsMessageHandler.cpp \
  ../utils/GamsSolveTrace.c ../utils/GamsHelper.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/gdxcc.c ../utils/optcc.c ../utils/palmcc.c ../utils/cfgmcc.c
libGamsCbc_la_LIBADD  = $(GAMSCBC_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optcbc
optcbc_SOURCES = optcbc.cpp ../utils/GamsOptionsSpecWriter.cpp
optcbc_LDADD = libGamsCbc.la

pkgdata_DATA = optcbc.def
doc_DATA = optcbc.md
optcbc.def optcbc.md : optcbc$(EXEEXT)
	./optcbc$(EXEEXT)
