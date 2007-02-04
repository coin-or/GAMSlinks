# Copyright (C) GAMS Development 2007
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

if ALWAYS_FALSE
  SUBDIRS += test
endif

EXTRA_DIST = \
	README \
	INSTALL \
	LICENSE \
	AUTHORS

# tmpdir = tmpdir

test: all
	cd test; $(MAKE) test

unitTest: test

.PHONY: test unitTest

# Stuff to clean up
clean-local:
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

distclean-local:
	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

.PHONY: test unitTest

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =
				
include ../BuildTools/Makemain.inc
