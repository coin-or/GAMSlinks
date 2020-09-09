# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/../utils \
  $(GAMSIPOPT_CFLAGS)

LDFLAGS += $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsIpopt.la
libGamsIpopt_la_SOURCES = GamsIpopt.cpp GamsNLP.cpp GamsJournal.cpp \
  ../utils/GamsHelper.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsIpopt_la_LIBADD  = $(GAMSIPOPT_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optipopt
optipopt_SOURCES = optipopt.cpp ../utils/GamsOptionsSpecWriter.cpp
optipopt_LDADD = libGamsIpopt.la
CLEANFILES += optipopt.gms optipopt.txt

install-exec-local : install-libLTLIBRARIES
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsIpopt.la MYIPOPT ipo "LP RMIP NLP DNLP RMINLP CNS QCP RMIQCP" 0

uninstall-local :
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYIPOPT
