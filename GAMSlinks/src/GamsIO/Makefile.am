# Copyright (C) 2006-2009 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 815 2009-12-02 17:06:24Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

lib_LTLIBRARIES = libSmagStricmp.la
if COIN_HAS_COINUTILS
lib_LTLIBRARIES += libGamsIO.la libSmagIO.la
endif

libGamsIO_la_SOURCES = \
  GamsModel.cpp GamsMessageHandler.cpp GamsOptions.cpp GamsDictionary.cpp GamsBCH.cpp GamsHandlerIOLib.cpp GamsGDX.cpp
if COIN_HAS_OSI
libGamsIO_la_SOURCES += GamsFinalize.cpp
endif
libGamsIO_la_LDFLAGS =
libGamsIO_la_LIBADD = $(GAMSIO_OBJDIR)/libGamsOptions.la $(GAMSIO_OBJDIR)/libGamsGDX.la

#we do not add SmagJournal to SmagIO to avoid dependence on Ipopt
libSmagIO_la_SOURCES = \
  GamsOptions.cpp GamsMessageHandler.cpp SmagExtra.c GamsHandlerSmag.cpp GamsBCH.cpp GamsDictionary.cpp GamsGDX.cpp GamsNLinstr.c
libSmagIO_la_LDFLAGS =
libSmagIO_la_LIBADD = $(GAMSIO_OBJDIR)/libGamsOptions.la $(GAMSIO_OBJDIR)/libGamsGDX.la

libSmagStricmp_la_SOURCES = SmagStricmp.c

includecoindir = $(includedir)/coin
includecoin_HEADERS = \
  GamsModel.hpp GamsMessageHandler.hpp GamsFinalize.hpp GamsOptions.hpp GamsDictionary.hpp GamsBCH.hpp GamsHandler.hpp GamsHandlerIOLib.hpp \
  SmagJournal.hpp GamsHandlerSmag.hpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) $(FORTRAN_NAMEMANGLING_CFLAG) -D$(GAMSIO_CODE) -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc`

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../../../BuildTools/Makemain.inc
