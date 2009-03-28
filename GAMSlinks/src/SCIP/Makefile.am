# Copyright (C) 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmssc_nt.cmd gmssc_us.run install/gmssc_nt.dat install/gmssc_un.dat

bin_PROGRAMS = GamsScip
GamsScip_SOURCES = GamsScip.cpp ScipBCH.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CLPOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(SCIPINCDIR)`

if COIN_BUILD_SCIP
GamsScip_LDADD = $(SCIPOBJDIR)/libcoinscip.la ../GamsIO/libSmagStricmp.la
GamsScip_DEPENDENCIES = $(SCIPOBJDIR)/libcoinscip.la ../GamsIO/libSmagStricmp.la
else
GamsScip_LDADD = $(SCIPLIB) ../GamsIO/libSmagStricmp.la
GamsScip_DEPENDENCIES =
endif 

GamsScip_LDADD += \
 ../GamsIO/libSmagIO.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la

GamsScip_DEPENDENCIES += \
 ../GamsIO/libSmagIO.la $(SMAG_LIBS) \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(CLPOBJDIR)/src/libClp.la

LIBS += `cat $(CLPOBJDIR)/clp_addlibs.txt $(GAMSLINKSOBJDIR)/smagio_addlibs.txt $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

GamsScip_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsScip gmssc_ux.out
	zip gmssc_.zip gmssc_ux.out gmssc_us.run install/gmssc_un.dat
	$(install_sh_DATA) gmssc_.zip $(DESTDIR)$(bindir)/gmssc_.zip
	rm -f gmssc_ux.out gmssc_.zip
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsScip.exe gmssc_nx.exe
	zip gmssc_.zip gmssc_nx.exe gmssc_nt.cmd install/gmssc_nt.dat
	$(install_sh_DATA) gmssc_.zip $(DESTDIR)$(bindir)/gmssc_.zip
	rm -f gmssc_nx.exe gmssc_.zip
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmssc_.zip

include ../../../BuildTools/Makemain.inc
