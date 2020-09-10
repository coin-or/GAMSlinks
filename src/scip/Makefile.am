# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

include ../../BuildTools/Makemain.inc

AM_CPPFLAGS = \
  -I$(srcdir)/../utils \
  $(GAMSSCIP_CFLAGS)

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsScip.la
libGamsScip_la_SOURCES = GamsScip.cpp reader_gmo.c event_solvetrace.c \
  ../utils/GamsLicensing.c ../utils/GamsHelper.c ../utils/GamsSolveTrace.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c ../utils/gdxcc.c ../utils/optcc.c
libGamsScip_la_LIBADD  = $(GAMSSCIP_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optscip
optscip_SOURCES = optscip.cpp ../utils/GamsOptionsSpecWriter.cpp
optscip_LDADD = libGamsScip.la

pkgdoc_DATA = optscip.md
optscip.md : optscip$(EXEEXT)
	./optscip$(EXEEXT)

install-exec-local : install-libLTLIBRARIES
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsScip.la MYSCIP scp "MIP QCP RMIQCP NLP DNLP RMINLP CNS MIQCP MINLP" 5
if COIN_HAS_GCG
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsScip.la MYGCG gcg "MIP" 5
endif

uninstall-local :
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYSCIP
if COIN_HAS_GCG
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYGCG
endif
