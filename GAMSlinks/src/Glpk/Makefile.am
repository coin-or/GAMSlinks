# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

bin_PROGRAMS = GamsCoinGlpk
GamsCoinGlpk_SOURCES = GamsCoinGlpk.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(srcdir)/../Cbc` \
 -I`$(CYGPATH_W) $(GLPKINCDIR)` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CBCOBJDIR)/inc`

GamsCoinGlpk_LDADD = \
 ../GamsIO/libGamsIO.la \
 $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(GAMSIO_LIBS) $(GLPKLIB)

ADDLIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt $(OSIOBJDIR)/osi_addlibs.txt`

GamsCoinGlpk_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
 
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_LINUX
install-exec-local:
	ln -sf $(DESTDIR)$(bindir)/GamsCoinGlpk gmscg_ux.out
	zip gmscg_.zip gmscg_ux.out gmscg_us.run install/gmscg_un.dat
	$(install_sh_DATA) gmscg_.zip $(DESTDIR)$(bindir)/gmscg_.zip
	rm -f gmscg_ux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local:
	ln -sf $(DESTDIR)$(bindir)/GamsCoinGlpk.exe gmscg_nx.exe
	zip gmscg_.zip gmscg_nx.exe gmscg_nt.cmd install/gmscg_nt.dat
	$(install_sh_DATA) gmscg_.zip $(DESTDIR)$(bindir)/gmscg_.zip
	rm -f gmscg_nx.exe  
endif
	
uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmscg_.zip

include ../../../BuildTools/Makemain.inc
