# Copyright (C) GAMS Development 2007
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmsos_nt.cmd gmsos_us.run install/gmsos_nt.dat install/gmsos_un.dat

bin_PROGRAMS = GamsOS
GamsOS_SOURCES = GamsOS.cpp Smag2OSiL.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)/..`

GamsOS_LDADD = \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(SMAG_LIBS)

GamsOS_DEPENDENCIES = \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la

LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

GamsOS_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = gmsos_.zip

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsOS gmsos_ux.out
	zip gmsos_.zip gmsos_ux.out gmsos_us.run install/gmsos_un.dat
	$(install_sh_DATA) gmsos_.zip $(DESTDIR)$(bindir)/gmsos_.zip
	rm -f gmsos_ux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsOS.exe gmsos_nx.exe
	zip gmsos_.zip gmsos_nx.exe gmsos_nt.cmd install/gmsos_nt.dat
	$(install_sh_DATA) gmsos_.zip $(DESTDIR)$(bindir)/gmsos_.zip
	rm -f gmsos_nx.exe
endif
	
uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsos_.zip

include ../../../BuildTools/Makemain.inc
