# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../cbc \
  -I$(srcdir)/../osi \
  -I$(srcdir)/../ipopt \
  -I$(srcdir)/../utils \
  $(GAMSBONMIN_CFLAGS)

# TODO better way to get to cplex header, or avoid it
AM_CPPFLAGS += $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsBonmin.la
libGamsBonmin_la_SOURCES = GamsBonmin.cpp GamsMINLP.cpp \
  ../osi/GamsMessageHandler.cpp \
  ../ipopt/GamsJournal.cpp ../ipopt/GamsNLP.cpp \
  ../utils/GamsSolveTrace.c ../utils/GamsHelper.c ../utils/GamsLicensing.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsBonmin_la_LIBADD  = $(GAMSBONMIN_LFLAGS)
libGamsBonmin_la_LDFLAGS = $(LT_LDFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optbonmin
optbonmin_SOURCES = optbonmin.cpp ../utils/GamsOptionsSpecWriter.cpp
optbonmin_LDADD = libGamsBonmin.la
CLEANFILES += optbonmin.gms optbonmin.txt optbonmin_t.md

install-exec-local : install-libLTLIBRARIES
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsBonmin.la MYBONMIN bon "MIP MIQCP MINLP" 0
#TODO uninstall-exec-local should remove solver in GAMS
