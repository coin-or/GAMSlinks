# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  -I"$(GAMS_PATH)/apifiles/C/api" \
  $(GAMSIPOPT_CFLAGS)

lib_LTLIBRARIES = libGamsIpopt.la
libGamsIpopt_la_SOURCES = GamsIpopt.cpp GamsNLP.cpp GamsJournal.cpp
libGamsIpopt_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSIPOPT_LFLAGS)
libGamsIpopt_la_LDFLAGS = $(LT_LDFLAGS)
