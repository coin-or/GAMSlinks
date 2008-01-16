# Copyright (C) 2006 GAMS Development
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = doxydoc/doxygen.conf osiglpk.patch INSTALL LICENCE README

########################################################################
#                          Subdirectories                              #
########################################################################

# subdirs is set by configure as the list of all subdirectories to recurse
# into
SUBDIRS = $(subdirs)

########################################################################
#                           Extra Targets                              #
########################################################################

test: all
	cd GAMSlinks; $(MAKE) test

unitTest: test

tests: all
	for dir in $(subdirs); do \
	  if test -r $$dir/test/Makefile; then \
	    (cd $$dir; $(MAKE) test) \
	  fi; \
	done

unitTests: tests

doxydoc:
	cd $(srcdir); doxygen doxydoc/doxygen.conf
	
gams-install:
	cd GAMSlinks; $(MAKE) gams-install

.PHONY: test unitTest tests unitTests gams-install doxydoc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include BuildTools/Makemain.inc
