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
	LICENSE \
	AUTHORS

# tmpdir = tmpdir

test: all
	cd test; $(MAKE) test

unitTest: test

if COIN_HAS_GAMSSYSTEM
gams-install: install
	echo "Installation of solvers in GAMS distribution at @GAMS_PATH@."
	cp $(DESTDIR)$(bindir)/gms*_.zip @GAMS_PATH@
	cd @GAMS_PATH@; ./gamsinst -a
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

########################################################################
#                  Installation of the addlibs file                    #
########################################################################

addlibsfile1 = gamsio_addlibs.txt
addlibsfile2 = smagio_addlibs.txt

install-exec-local: install-doc
	$(install_sh_DATA) $(addlibsfile1) $(DESTDIR)$(libdir)/$(addlibsfile1)
	$(install_sh_DATA) $(addlibsfile2) $(DESTDIR)$(libdir)/$(addlibsfile2)

uninstall-local: uninstall-doc
	rm -f $(DESTDIR)$(libdir)/$(addlibsfile1)
	rm -f $(DESTDIR)$(libdir)/$(addlibsfile2)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../BuildTools/Makemain.inc
