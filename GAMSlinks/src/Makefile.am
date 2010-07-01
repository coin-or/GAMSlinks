# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

SUBDIRS = GamsUtils SolverInterfaces

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
