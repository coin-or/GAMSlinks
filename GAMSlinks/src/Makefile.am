# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

SUBDIRS = GamsUtils
if GAMSLINKS_BUILD_GAMSIO
  SUBDIRS += GamsIO
endif
if GAMSLINKS_BUILD_CBC
  SUBDIRS += Cbc
endif
if GAMSLINKS_BUILD_GLPK
  SUBDIRS += Glpk
endif
if GAMSLINKS_BUILD_IPOPT
  SUBDIRS += Ipopt
endif
if GAMSLINKS_BUILD_BONMIN
  SUBDIRS += Bonmin
endif
if GAMSLINKS_BUILD_OSI
  SUBDIRS += Osi
endif
if GAMSLINKS_BUILD_OS
  SUBDIRS += OS
endif
if GAMSLINKS_BUILD_SCIP
  SUBDIRS += SCIP
endif

EXTRA_DIST =

DISTCLEANFILES =

includecoindir = $(includedir)/coin
includecoin_HEADERS = ../inc/config_gamslinks.h

ConfigHeader = GAMSlinksConfig.h

install-exec-local:
	echo "#ifndef HAVE_CONFIG_H" >bla
	echo "#define HAVE_CONFIG_H" >>bla
	echo "#endif" >> bla
	cat $(srcdir)/$(ConfigHeader) >> bla
	$(install_sh_DATA) bla $(DESTDIR)$(includecoindir)/$(ConfigHeader)
	rm -f bla

uninstall-local:
	rm -f $(DESTDIR)$(includecoindir)/$(ConfigHeader)

include ../../BuildTools/Makemain.inc
