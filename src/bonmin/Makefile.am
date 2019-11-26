# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../utils \
  -I$(srcdir)/../cbc \
  -I"$(GAMS_PATH)/apifiles/C/api" \
  $(GAMSBONMIN_CFLAGS)

# TODO better way to get to cplex header, or avoid it
AM_CPPFLAGS += $(GAMSOSI_CFLAGS)

lib_LTLIBRARIES = libGamsBonmin.la
libGamsBonmin_la_SOURCES = GamsBonmin.cpp GamsMINLP.cpp \
  ../cbc/GamsMessageHandler.cpp \
  ../ipopt/GamsJournal.cpp ../ipopt/GamsNLP.cpp
libGamsBonmin_la_LIBADD  = ../utils/libGamsUtils.la $(GAMSBONMIN_LFLAGS)
libGamsBonmin_la_LDFLAGS = $(LT_LDFLAGS)
