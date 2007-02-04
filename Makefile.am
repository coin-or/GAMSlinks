# Copyright (C) 2006 GAMS Development
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = doxydoc/doxygen.conf cbc.patch cgl.patch ipopt.patch osiglpk.patch \
  ThirdParty/GLPK/get.glpk.cl ThirdParty/GLPK/get.glpk.darwin ThirdParty/GLPK/get.glpk.linux \
  ThirdParty/GLPK/darwin.mak ThirdParty/GLPK/glpk.patch ThirdParty/GLPK/glpk/share

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

.PHONY: test unitTest tests unitTests doxydoc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include BuildTools/Makemain.inc
