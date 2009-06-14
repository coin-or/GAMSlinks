# Copyright (C) 2006 GAMS Development
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id: Makefile.am 324 2008-02-07 19:14:52Z stefan $

# Author:  Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = doxydoc/doxygen.conf INSTALL LICENCE README

########################################################################
#                          Subdirectories                              #
########################################################################

# subdirs is set by configure as the list of all subdirectories to recurse
# into
SUBDIRS = $(subdirs)

########################################################################
#                           Extra Targets                              #
########################################################################

#test: all
#	cd GamsUtils; $(MAKE) test

#unitTest: test

#tests: all
#	for dir in $(subdirs); do \
#	  if test -r $$dir/test/Makefile; then \
#	    (cd $$dir; $(MAKE) test) \
#	  fi; \
#	done

#unitTests: tests

doxydoc:
	cd $(srcdir); doxygen doxydoc/doxygen.conf
	
#.PHONY: test unitTest tests unitTests gams-install doxydoc
.PHONY: doxydoc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include BuildTools/Makemain.inc
