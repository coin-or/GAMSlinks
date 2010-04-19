# Copyright (C) GAMS Development and others 2010
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 382 2008-03-04 12:10:06Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = optdoc

optdoc_SOURCES = optdoc.cpp

optdoc_LDADD = ../../src/SolverInterfaces/libGamsCoin.la

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/../../src`

if COIN_HAS_IPOPT
AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/LinearSolverLoader` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`
endif

if COIN_HAS_COINUTILS
AM_CPPFLAGS += -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
  -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src`
endif

if COIN_HAS_CLP
AM_CPPFLAGS +=  -I`$(CYGPATH_W) $(CLPOBJDIR)/inc` \
  -I`$(CYGPATH_W) $(CLPSRCDIR)/src`
endif

if COIN_HAS_OSI
AM_CPPFLAGS += -I`$(CYGPATH_W) $(OSIOBJDIR)/inc` \
  -I`$(CYGPATH_W) $(OSISRCDIR)/src`
endif

if COIN_HAS_CGL
AM_CPPFLAGS += -I`$(CYGPATH_W) $(CGLOBJDIR)/inc` \
  -I`$(CYGPATH_W) $(CGLSRCDIR)/src`
endif

if COIN_HAS_CBC
AM_CPPFLAGS += -I`$(CYGPATH_W) $(CBCOBJDIR)/inc` \
  -I`$(CYGPATH_W) $(CBCSRCDIR)/src`
endif

if COIN_HAS_BONMIN
AM_CPPFLAGS += -I`$(CYGPATH_W) $(BONMINOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/CbcBonmin`
endif

if COIN_HAS_COUENNE
AM_CPPFLAGS += -I`$(CYGPATH_W) $(COUENNEOBJDIR)/inc` \
  -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms/OaGenerators` \
  -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms/Branching` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/bound_tightening` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/branch` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/convex` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/disjunctive` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/expression` \
  -I`$(CYGPATH_W) $(COUENNESRCDIR)/src/expression/operators` \
  -I`$(CYGPATH_W) $(COUENNESRCDIR)/src/expression/simplify` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/main` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/problem` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/readnl` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/standardize` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/util` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src`
endif

if GAMSLINKS_BUILD_SCIP
AM_CPPFLAGS += -I`$(CYGPATH_W) $(SCIPINCDIR)`
endif

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =
EXTRA_DIST = 

include ../../../BuildTools/Makemain.inc
