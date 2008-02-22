# Copyright (C) 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 298 2008-01-04 14:03:20Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmssc_nt.cmd gmssc_us.run install/gmssc_nt.dat install/gmssc_un.dat

bin_PROGRAMS = GamsScip
GamsScip_SOURCES = GamsScip.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CLPOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(SCIPINCDIR)`

GamsScip_LDADD = \
 ../GamsIO/libSmagIO.la \
 ../../$(SCIPOBJDIR)/libcoinscip.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
GamsScip_DEPENDENCIES = \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(CLPOBJDIR)/src/libClp.la \
 ../GamsIO/libSmagIO.la $(SMAG_LIBS) \
 ../../$(SCIPOBJDIR)/libcoinscip.la

LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt $(CLPOBJDIR)/clp_addlibs.txt $(GAMSLINKSOBJDIR)/smagio_addlibs.txt`

GamsScip_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = gmssc_.zip

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsScip gmssc_ux.out
	zip gmssc_.zip gmssc_ux.out gmssc_us.run optscip.def install/gmssc_un.dat
	$(install_sh_DATA) gmssc_.zip $(DESTDIR)$(bindir)/gmssc_.zip
	rm -f gmssc_ux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsScip.exe gmssc_nx.exe
	zip gmssc_.zip gmssc_nx.exe gmssc_nt.cmd optscip.def install/gmssc_nt.dat
	$(install_sh_DATA) gmssc_.zip $(DESTDIR)$(bindir)/gmssc_.zip
	rm -f gmssc_nx.exe
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmssc_.zip

include ../../../BuildTools/Makemain.inc
