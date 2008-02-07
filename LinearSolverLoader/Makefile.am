# Copyright (C) 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 313 2008-01-20 12:39:42Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

#if ALWAYS_FALSE
#  SUBDIRS += test
#endif

EXTRA_DIST = \
	README \
	LICENSE \
	AUTHORS

.PHONY:

########################################################################
#                  Installation of the addlibs file                    #
########################################################################

#addlibsfile1 = gamsio_addlibs.txt

#install-exec-local: install-doc
#	$(install_sh_DATA) $(addlibsfile1) $(DESTDIR)$(libdir)/$(addlibsfile1)

#uninstall-local: uninstall-doc
#	rm -f $(DESTDIR)$(libdir)/$(addlibsfile1)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../BuildTools/Makemain.inc
