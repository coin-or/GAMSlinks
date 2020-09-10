# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

include ../../BuildTools/Makemain.inc

AM_CPPFLAGS = \
  -I$(srcdir)/../cbc \
  -I$(srcdir)/../osi \
  -I$(srcdir)/../ipopt \
  -I$(srcdir)/../utils \
  $(GAMSBONMIN_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsBonmin.la
libGamsBonmin_la_SOURCES = GamsBonmin.cpp GamsMINLP.cpp \
  ../osi/GamsMessageHandler.cpp \
  ../ipopt/GamsJournal.cpp ../ipopt/GamsNLP.cpp \
  ../utils/GamsSolveTrace.c ../utils/GamsHelper.c ../utils/GamsLicensing.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsBonmin_la_LIBADD  = $(GAMSBONMIN_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optbonmin
optbonmin_SOURCES = optbonmin.cpp ../utils/GamsOptionsSpecWriter.cpp
optbonmin_LDADD = libGamsBonmin.la

pkgdata_DATA = optbonmin.def
pkgdoc_DATA = optbonmin.md
optbonmin.def optbonmin.md : optbonmin$(EXEEXT)
	./optbonmin$(EXEEXT)

install-exec-local : install-libLTLIBRARIES
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsBonmin.la MYBONMIN bon "MIQCP MINLP" 0

uninstall-local :
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYBONMIN
