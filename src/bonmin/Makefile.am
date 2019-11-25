# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  -I$(srcdir)/../cbc \
  -I"$(GAMS_PATH)/apifiles/C/api" \
  $(GAMSBONMIN_CFLAGS)

# TODO better way to get to cplex header, or avoid it
AM_CPPFLAGS += $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsBonmin.la
#FIXME GamsJournal GamsNLP?
libGamsBonmin_la_SOURCES = GamsMINLP.cpp GamsBonmin.cpp
libGamsBonmin_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSBONMIN_LFLAGS)
libGamsBonmin_la_LDFLAGS = $(LT_LDFLAGS)
