# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  $(GAMSIPOPT_CFLAGS)

lib_LTLIBRARIES = libGamsIpopt.la
libGamsIpopt_la_SOURCES = GamsIpopt.cpp GamsNLP.cpp GamsJournal.cpp \
  ../utils/GamsHelper.c \
  ../utils/gmomcc.c ../utils/gevmcc.c
libGamsIpopt_la_LIBADD  = $(GAMSIPOPT_LFLAGS)
libGamsIpopt_la_LDFLAGS = $(LT_LDFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc
