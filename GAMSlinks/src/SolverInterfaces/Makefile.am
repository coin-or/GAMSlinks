# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST     =
DISTCLEANFILES =

AM_CPPFLAGS = $(GAMSSOLVERLIB_CFLAGS) \
  -I`$(CYGPATH_W) $(srcdir)/..` \
  -I`$(CYGPATH_W) $(srcdir)/../GamsUtils` \
  -I`$(CYGPATH_W) $(srcdir)/../GamsUtils/$(GAMS_VERSION)` \
  $(GAMSSOLVERLIB_CFLAGS)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I..

lib_LTLIBRARIES = libGamsCoin.la
libGamsCoin_la_SOURCES      = GamsSolver.cpp
libGamsCoin_la_LIBADD       = ../GamsUtils/libGamsUtils.la ../GamsUtils/gevmcc.lo ../GamsUtils/gmomcc.lo ../GamsUtils/gclgms.lo $(GAMSSOLVERLIBS_LIBS)
libGamsCoin_la_DEPENDENCIES = ../GamsUtils/libGamsUtils.la ../GamsUtils/gevmcc.lo ../GamsUtils/gmomcc.lo ../GamsUtils/gclgms.lo $(GAMSSOLVERLIBS_DEPENDENCIES)
libGamsCoin_la_LDFLAGS      = -module -no-undefined

bin_SCRIPTS =
if COIN_IS_WINDOWS
  SCRIPTSUFFIX=nt.cmd
else
  SCRIPTSUFFIX=us.run
endif

if COIN_HAS_OSI
#  libGamsCoin_la_SOURCES += GamsOsi.cpp GamsOsiHelper.cpp GamsMessageHandler.cpp
endif

if COIN_HAS_OSICPX
  bin_SCRIPTS += gmszcp$(SCRIPTSUFFIX)
endif

if COIN_HAS_OSIGLPK
  bin_SCRIPTS += gmszgl$(SCRIPTSUFFIX) optmyglpk.def
endif

if COIN_HAS_OSIGRB
  bin_SCRIPTS += gmszgu$(SCRIPTSUFFIX)
endif

if COIN_HAS_OSIMSK
  bin_SCRIPTS += gmszmk$(SCRIPTSUFFIX)
endif

if COIN_HAS_OSISPX
  bin_SCRIPTS += gmszsp$(SCRIPTSUFFIX)
endif

if COIN_HAS_OSIXPR
  bin_SCRIPTS += gmszxp$(SCRIPTSUFFIX)
endif

if COIN_HAS_CBC
#  libGamsCoin_la_SOURCES += GamsCbc.cpp
  bin_SCRIPTS += gmscbc$(SCRIPTSUFFIX) optmycbc.def
endif

if COIN_HAS_IPOPT
#  libGamsCoin_la_SOURCES += GamsIpopt.cpp GamsNLP.cpp GamsJournal.cpp
  bin_SCRIPTS += gmsip_$(SCRIPTSUFFIX)
endif

if COIN_HAS_SCIP
#  libGamsCoin_la_SOURCES += GamsScip.cpp
  bin_SCRIPTS += gmssc_$(SCRIPTSUFFIX)
endif

if COIN_HAS_BONMIN
#  libGamsCoin_la_SOURCES += GamsBonmin.cpp GamsMINLP.cpp
  bin_SCRIPTS += gmsbm_$(SCRIPTSUFFIX)
endif

if COIN_HAS_COUENNE
#  libGamsCoin_la_SOURCES += GamsCouenne.cpp
  bin_SCRIPTS += gmscou$(SCRIPTSUFFIX)
endif

if COIN_HAS_OS
#  libGamsCoin_la_SOURCES += GamsOS.cpp GamsOSxL.cpp
  bin_SCRIPTS += gmsos_$(SCRIPTSUFFIX) optmyos.def
endif

include ../../BuildTools/Makemain.inc
