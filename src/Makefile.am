# Copyright (C) GAMS Development 2006-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

SUBDIRS = utils

if COIN_HAS_BONMIN
  SUBDIRS += bonmin
endif

if COIN_HAS_CBC
  SUBDIRS += cbc
endif

if COIN_HAS_COUENNE
  SUBDIRS += couenne
endif

if COIN_HAS_IPOPT
  SUBDIRS += ipopt
endif

if COIN_HAS_OSI
  SUBDIRS += osi
endif

if COIN_HAS_SCIP
  SUBDIRS += scip
endif

if COIN_HAS_SOPLEX
  SUBDIRS += soplex
endif

bin_SCRIPTS = gamsinst.sh
