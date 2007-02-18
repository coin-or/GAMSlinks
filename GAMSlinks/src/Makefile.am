# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

SUBDIRS = GamsIO
if COIN_HAS_CBC
  SUBDIRS += Cbc
endif
if COIN_HAS_GLPK
  SUBDIRS += Glpk
endif
if COIN_HAS_IPOPT
  SUBDIRS += Ipopt
endif
#if COIN_HAS_BONMIN
#  SUBDIRS += Bonmin
#endif

EXTRA_DIST =

DISTCLEANFILES =

ConfigHeader = GAMSlinksConfig.h

install-exec-local:
	echo "#ifndef HAVE_CONFIG_H" >bla
	echo "#define HAVE_CONFIG_H" >>bla
	echo "#endif" >> bla
	cat $(srcdir)/$(ConfigHeader) >> bla
	$(install_sh_DATA) bla $(DESTDIR)$(includedir)/$(ConfigHeader)
	rm -f bla

uninstall-local:
	rm -f $(DESTDIR)$(includedir)/$(ConfigHeader)

include ../../BuildTools/Makemain.inc
	