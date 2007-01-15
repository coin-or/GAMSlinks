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

test:
	cd test; make test

unitTest: test

.PHONY: test unitTest

########################################################################
#                  Installation of the addlibs file                    #
########################################################################

# install-exec-local:
# 	$(install_sh_DATA) $(addlibsfile) $(DESTDIR)$(libdir)/$(addlibsfile)
# 
# uninstall-local:
# 	rm -f $(DESTDIR)$(libdir)/$(addlibsfile)


# Stuff to clean up

DISTCLEANFILES =
				
include ../BuildTools/Makemain.inc
