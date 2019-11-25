# Copyright (C) GAMS Development and others 2010-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = optdoc

optdoc_SOURCES = optdoc.cpp

optdoc_LDADD = ../../src/SolverInterfaces/libGamsCoin.la

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/../../src` -I`$(CYGPATH_W) $(srcdir)/../../src/SolverInterfaces` $(GAMSSOLVERLIB_CFLAGS)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =
EXTRA_DIST = 

include ../../BuildTools/Makemain.inc
