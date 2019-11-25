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
  $(GAMSSCIP_CFLAGS)

# TODO better way to get to cplex header, but maybe we don't need it at all?
AM_CPPFLAGS += $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsScip.la
libGamsScip_la_SOURCES = GamsScip.cpp reader_gmo.c event_solvetrace.c prop_defaultbounds.c
libGamsScip_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSSCIP_LFLAGS)
libGamsScip_la_LDFLAGS = $(LT_LDFLAGS)
