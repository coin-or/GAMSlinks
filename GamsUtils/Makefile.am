# Copyright (C) GAMS Development 2009 
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 382 2008-03-04 12:10:06Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

if COIN_HAS_GAMSIO
SUBDIRS = src/GamsUtils
endif

#if ALWAYS_FALSE
#  SUBDIRS += test
#endif

EXTRA_DIST = \
	README \
	LICENSE \
	AUTHORS

# tmpdir = tmpdir

#test: all
#	cd test; $(MAKE) test

#unitTest: test

#.PHONY: test unitTest gams-install

# Stuff to clean up
#clean-local:
#	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

#distclean-local:
#	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

########################################################################
#                  Installation of the addlibs file                    #
########################################################################

addlibsfile = gamsutils_addlibs.txt

install-exec-local: install-doc
	$(install_sh_DATA) $(addlibsfile) $(DESTDIR)$(libdir)/$(addlibsfile)

uninstall-local: uninstall-doc
	rm -f $(DESTDIR)$(libdir)/$(addlibsfile)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../BuildTools/Makemain.inc
