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
  $(GAMSCOUENNE_CFLAGS)

# TODO better way to get to cplex header, but maybe we don't need it at all?
AM_CPPFLAGS += $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsCouenne.la
# FIXME GamsMINLP.cpp GamsJournal
libGamsCouenne_la_SOURCES = GamsCouenne.cpp
libGamsCouenne_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSCOUENNE_LFLAGS)
libGamsCouenne_la_LDFLAGS = $(LT_LDFLAGS)
