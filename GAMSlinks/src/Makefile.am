# Copyright (C) GAMS Development 2006-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

SUBDIRS = GamsUtils SolverInterfaces

EXTRA_DIST =

DISTCLEANFILES =

includecoindir = $(includedir)/coin

install-exec-local:
	$(install_sh_DATA) config_gamslinks.h $(DESTDIR)$(includecoindir)/GAMSlinksConfig.h

uninstall-local:
	rm -f $(DESTDIR)$(includecoindir)/GAMSlinksConfig.h

include ../BuildTools/Makemain.inc
