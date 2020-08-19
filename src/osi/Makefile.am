# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/../utils \
  $(GAMSOSI_CFLAGS)

LDFLAGS += $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsOsi.la
libGamsOsi_la_SOURCES = GamsOsiHelper.cpp GamsOsi.cpp GamsMessageHandler.cpp \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
libGamsOsi_la_LIBADD  = $(GAMSOSI_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

install-exec-local : install-libLTLIBRARIES
if COIN_HAS_OSICPX
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsOsi.la MYOSICPLEX ocp "LP RMIP MIP" 5
endif
if COIN_HAS_OSIGRB
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsOsi.la MYOSIGUROBI ogu "LP RMIP MIP" 5
endif
if COIN_HAS_OSIMSK
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsOsi.la MYOSIMOSEK omk "LP RMIP MIP" 5
endif
if COIN_HAS_OSIXPR
	$(top_srcdir)/gamsinst.py "@GAMS_PATH@" libGamsOsi.la MYOSIXPRESS oxp "LP RMIP MIP" 5
endif

uninstall-local :
if COIN_HAS_OSICPX
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYOSICPLEX
endif
if COIN_HAS_OSIGRB
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYOSIGUROBI
endif
if COIN_HAS_OSIMSK
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYOSIMOSEK
endif
if COIN_HAS_OSIXPR
	$(top_srcdir)/gamsuninst.py "@GAMS_PATH@" MYOSIXPRESS
endif
