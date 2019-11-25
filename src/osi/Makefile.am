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
  $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsOsi.la
#FIXME GamsMessageHandler.cpp
libGamsOsi_la_SOURCES = GamsOsiHelper.cpp GamsOsi.cpp
libGamsOsi_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSOSI_LFLAGS)
libGamsOsi_la_LDFLAGS = $(LT_LDFLAGS)
