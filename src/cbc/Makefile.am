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
  $(GAMSCBC_CFLAGS)

# TODO better way to get to cplex header, or avoid it
AM_CPPFLAGS += $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsCbc.la
#FIXME  GamsOsiHelper.cpp
libGamsCbc_la_SOURCES = GamsCbc.cpp GamsMessageHandler.cpp
libGamsCbc_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSCBC_LFLAGS)
libGamsCbc_la_LDFLAGS = $(LT_LDFLAGS)

bin_SCRIPTS = optmycbc.def
