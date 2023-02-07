# Copyright (C) GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

SUBDIRS = amplsolver

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

if COIN_HAS_HIGHS
  SUBDIRS += highs
endif

install-exec-local : gamsconfig.yaml
	@mkdir_p@ $(pkgdatadir)
	$(srcdir)/install-gamsconfig.py gamsconfig.yaml $(pkgdatadir)

uninstall-local :
	rm -rf $(pkgdatadir)/gamsconfig.yaml
