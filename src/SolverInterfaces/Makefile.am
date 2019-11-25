# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = \
  -I$(srcdir)/.. \
  -I$(srcdir)/../GamsUtils \
  -I$(srcdir)/scip \
  -I"$(GAMS_PATH)/apifiles/C/api" \
  $(GAMSSOLVERLIB_CFLAGS)

lib_LTLIBRARIES = libGamsCoin.la
libGamsCoin_la_SOURCES      =
libGamsCoin_la_LIBADD       = ../GamsUtils/libGamsUtils.la $(GAMSSOLVERLIB_LFLAGS)
libGamsCoin_la_LDFLAGS      = $(LT_LDFLAGS)

bin_SCRIPTS = gamsinst.sh
if COIN_IS_WINDOWS
  SCRIPTSUFFIX=nt.cmd
else
  SCRIPTSUFFIX=us.run
endif

if COIN_HAS_OSI
  libGamsCoin_la_SOURCES += GamsMessageHandler.cpp GamsOsiHelper.cpp GamsOsi.cpp
endif

if COIN_HAS_CBC
  libGamsCoin_la_SOURCES += GamsCbc.cpp
  bin_SCRIPTS += optmycbc.def
endif

if COIN_HAS_IPOPT
  libGamsCoin_la_SOURCES += GamsJournal.cpp GamsNLP.cpp GamsIpopt.cpp 
endif

if COIN_HAS_SOPLEX
  libGamsCoin_la_SOURCES += GamsSoPlex.cpp
endif

if COIN_HAS_SCIP
  libGamsCoin_la_SOURCES += GamsScip.cpp scip/reader_gmo.c scip/event_solvetrace.c scip/prop_defaultbounds.c
endif

if COIN_HAS_BONMIN
  libGamsCoin_la_SOURCES += GamsMINLP.cpp GamsBonmin.cpp
endif

if COIN_HAS_COUENNE
  libGamsCoin_la_SOURCES += GamsCouenne.cpp
endif
