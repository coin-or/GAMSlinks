# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  $(GAMSSCIP_CFLAGS)

lib_LTLIBRARIES = libGamsScip.la
libGamsScip_la_SOURCES = GamsScip.cpp reader_gmo.c event_solvetrace.c \
  ../utils/GamsLicensing.c ../utils/GamsHelper.c ../utils/GamsSolveTrace.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c ../utils/gdxcc.c ../utils/optcc.c
libGamsScip_la_LIBADD  = $(GAMSSCIP_LFLAGS)
libGamsScip_la_LDFLAGS = $(LT_LDFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optscip
optscip_SOURCES = optscip.cpp ../utils/GamsOptionsSpecWriter.cpp
optscip_LDADD = libGamsScip.la
CLEANFILES += optscip.gms branchrules.md conflicthdlrs.md conshdlrs.md disps.md heurs.md nodesels.md presols.md props.md sepas.md

install-exec-local : install-libLTLIBRARIES
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsScip.la MYSCIP scp "MIP QCP RMIQCP NLP DNLP RMINLP CNS MIQCP MINLP" 5

uninstall-local :
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYSCIP
