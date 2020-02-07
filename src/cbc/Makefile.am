# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../osi \
  -I$(srcdir)/../utils \
  $(GAMSCBC_CFLAGS)

LDFLAGS += $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsCbc.la
libGamsCbc_la_SOURCES = GamsCbc.cpp \
  ../osi/GamsOsiHelper.cpp ../osi/GamsMessageHandler.cpp \
  ../utils/GamsSolveTrace.c ../utils/GamsHelper.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/gdxcc.c ../utils/optcc.c ../utils/palmcc.c
libGamsCbc_la_LIBADD  = $(GAMSCBC_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optcbc
optcbc_SOURCES = optcbc.cpp ../utils/GamsOptionsSpecWriter.cpp
optcbc_LDADD = libGamsCbc.la
CLEANFILES += optcbc.txt optcbc.gms

install-exec-local : install-libLTLIBRARIES
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsCbc.la MYCBC cbc "LP MIP RMIP" 5 @srcdir@/optmycbc.def

uninstall-local :
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYCBC optmycbc.def
