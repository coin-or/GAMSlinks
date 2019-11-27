# Copyright (C) GAMS Development and others 2010-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = optdoc

optdoc_SOURCES = optdoc.cpp

optdoc_LDADD =

AM_CPPFLAGS = -I$(srcdir)/../../src

if COIN_HAS_BONMIN
  AM_CPPFLAGS += $(GAMSBONMIN_CFLAGS)
  optdoc_LDADD += ../../src/bonmin/libGamsBonmin.la
endif
if COIN_HAS_COUENNE
  AM_CPPFLAGS += $(GAMSCOUENNE_CFLAGS)
  optdoc_LDADD += ../../src/couenne/libGamsCouenne.la
endif
if COIN_HAS_CBC
  AM_CPPFLAGS += $(GAMSCBC_CFLAGS)
  optdoc_LDADD += ../../src/cbc/libGamsCbc.la
endif
if COIN_HAS_IPOPT
  AM_CPPFLAGS += $(GAMSIPOPT_CFLAGS)
  optdoc_LDADD += ../../src/ipopt/libGamsIpopt.la
endif
if COIN_HAS_SCIP
  AM_CPPFLAGS += -I$(srcdir)/../../src/scip $(GAMSSCIP_CFLAGS)
  optdoc_LDADD += ../../src/scip/libGamsScip.la
endif
if COIN_HAS_SOPLEX
  AM_CPPFLAGS += $(GAMSSOPLEX_CFLAGS)
  optdoc_LDADD += ../../src/soplex/libGamsSoplex.la
endif
