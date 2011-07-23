# Copyright (C) GAMS Development 2007-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

if ALWAYS_FALSE
  SUBDIRS += test
endif

EXTRA_DIST = README LICENSE AUTHORS

test: all
	cd test; $(MAKE) test

unitTest: test

if COIN_HAS_GAMSSYSTEM
gams-install: install
	echo "Installation of solvers in GAMS distribution at @GAMS_PATH@."
	cp $(DESTDIR)$(bindir)/gms*_.zip "@GAMS_PATH@"
	cd "@GAMS_PATH@"; ./gamsinst -a
else
gams-install: 
	echo "Installation of solvers in GAMS distribution fails because path of GAMS system not known."
	exit 1
endif

.PHONY: test unitTest gams-install

# Stuff to clean up
clean-local:
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

distclean-local:
	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

install-exec-local: install-doc

uninstall-local: uninstall-doc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include BuildTools/Makemain.inc
