# Copyright (C) GAMS Development and others 2009-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST     =
DISTCLEANFILES =

AM_CPPFLAGS = \
  -I`$(CYGPATH_W) $(srcdir)/..` \
  -I`$(CYGPATH_W) $(srcdir)/../GamsUtils` \
  -I`$(CYGPATH_W) $(srcdir)/../GamsUtils/$(GAMS_VERSION)` \
  $(GAMSSOLVERLIB_CFLAGS)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I..

lib_LTLIBRARIES = libGamsCoin.la
libGamsCoin_la_SOURCES      = GamsSolver.cpp
libGamsCoin_la_LIBADD       = ../GamsUtils/libGamsUtils.la $(GAMSSOLVERLIB_LIBS)
libGamsCoin_la_DEPENDENCIES = ../GamsUtils/libGamsUtils.la $(GAMSSOLVERLIB_DEPENDENCIES)
libGamsCoin_la_LDFLAGS      = -module -no-undefined

includecoindir = $(includedir)/coin
includecoin_HEADERS = 

bin_SCRIPTS =
if COIN_IS_WINDOWS
  SCRIPTSUFFIX=nt.cmd
else
  SCRIPTSUFFIX=us.run
endif

if COIN_HAS_OSI
  libGamsCoin_la_SOURCES += GamsMessageHandler.cpp GamsOsiHelper.cpp GamsOsi.cpp
  includecoin_HEADERS    += GamsMessageHandler.hpp GamsOsiHelper.hpp GamsOsi.hpp
endif

if COIN_HAS_OSIGLPK
  bin_SCRIPTS += optmyglpk.def
endif

if COIN_HAS_CBC
  libGamsCoin_la_SOURCES += GamsCbc.cpp
  includecoin_HEADERS    += GamsCbc.hpp
  bin_SCRIPTS += optmycbc.def
endif

if COIN_HAS_IPOPT
  libGamsCoin_la_SOURCES += GamsJournal.cpp GamsNLP.cpp GamsIpopt.cpp 
  includecoin_HEADERS    += GamsJournal.hpp GamsNLP.hpp GamsIpopt.hpp
endif

if COIN_HAS_SCIP
#  libGamsCoin_la_SOURCES += GamsScip.cpp
endif

if COIN_HAS_BONMIN
  libGamsCoin_la_SOURCES += GamsMINLP.cpp GamsBonmin.cpp
endif

if COIN_HAS_COUENNE
  libGamsCoin_la_SOURCES += GamsCouenne.cpp
endif

if COIN_HAS_OS
#  libGamsCoin_la_SOURCES += GamsOS.cpp GamsOSxL.cpp
  bin_SCRIPTS += optmyos.def
endif
