# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AM_CPPFLAGS = \
  -I$(srcdir)/../bonmin \
  -I$(srcdir)/../cbc \
  -I$(srcdir)/../osi \
  -I$(srcdir)/../ipopt \
  -I$(srcdir)/../utils \
  $(GAMSCOUENNE_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsCouenne.la
libGamsCouenne_la_SOURCES = GamsCouenne.cpp \
  ../bonmin/GamsMINLP.cpp \
  ../osi/GamsMessageHandler.cpp \
  ../ipopt/GamsJournal.cpp ../ipopt/GamsNLP.cpp \
  ../utils/GamsSolveTrace.c ../utils/GamsHelper.c ../utils/GamsLicensing.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsCouenne_la_LIBADD  = $(GAMSCOUENNE_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optcouenne
optcouenne_SOURCES = optcouenne.cpp ../utils/GamsOptionsSpecWriter.cpp
optcouenne_LDADD = libGamsCouenne.la

pkgdata_DATA = optcouenne.def
doc_DATA = optcouenne.md
optcouenne.def optcouenne.md : optcouenne$(EXEEXT)
	./optcouenne$(EXEEXT)
