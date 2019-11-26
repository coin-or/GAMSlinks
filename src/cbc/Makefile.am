# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  -I"$(GAMS_PATH)/apifiles/C/api" \
  $(GAMSCBC_CFLAGS)

lib_LTLIBRARIES = libGamsCbc.la
libGamsCbc_la_SOURCES = GamsCbc.cpp GamsMessageHandler.cpp ../osi/GamsOsiHelper.cpp
libGamsCbc_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSCBC_LFLAGS)
libGamsCbc_la_LDFLAGS = $(LT_LDFLAGS)

bin_SCRIPTS = optmycbc.def
