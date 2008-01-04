# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

lib_LTLIBRARIES = libGamsIO.la libSmagIO.la
libGamsIO_la_SOURCES = \
  GamsModel.cpp GamsFinalize.cpp GamsMessageHandler.cpp GamsOptions.cpp GamsBCH.cpp
libGamsIO_la_LDFLAGS =
libGamsIO_la_LIBADD = $(GAMSLINKSOBJDIR)/../ThirdParty/GAMSIO/libGamsOptions.la \
  $(GAMSLINKSOBJDIR)/../ThirdParty/GAMSIO/libGamsGDX.la

libSmagIO_la_SOURCES = \
  GamsOptions.cpp SmagMessageHandler.cpp
if COIN_HAS_IPOPT
libSmagIO_la_SOURCES += SmagJournal.cpp
endif
libSmagIO_la_LDFLAGS =
libSmagIO_la_LIBADD = $(GAMSLINKSOBJDIR)/../ThirdParty/GAMSIO/libGamsOptions.la

includecoindir = $(includedir)/coin
includecoin_HEADERS = ../../inc/config_gamslinks.h \
  GamsModel.hpp GamsMessageHandler.hpp GamsFinalize.hpp GamsOptions.hpp GamsBCH.hpp \
  SmagJournal.hpp SmagMessageHandler.hpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc`

if COIN_HAS_IPOPT
AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`
endif

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../../../BuildTools/Makemain.inc
