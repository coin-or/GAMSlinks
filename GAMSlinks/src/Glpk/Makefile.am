# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 635 2009-03-28 19:48:00Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = optglpk.def optglpk.gms optglpk.txt \
  gmscg_nt.cmd gmscg_us.run install/gmscg_nt.dat install/gmscg_un.dat

bin_PROGRAMS = GamsCoinGlpk
GamsCoinGlpk_SOURCES = GamsCoinGlpk.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(srcdir)/../Cbc` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CBCOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(GLPKINCDIR)`

GamsCoinGlpk_LDADD = \
 ../GamsIO/libGamsIO.la \
 $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
GamsCoinGlpk_DEPENDENCIES = \
 ../GamsIO/libGamsIO.la $(GAMSIO_LIBS) \
 $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la 

LIBS += `cat $(OSIOBJDIR)/osi_addlibs.txt $(GAMSLINKSOBJDIR)/gamsio_addlibs.txt $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

GamsCoinGlpk_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsCoinGlpk gmscg_ux.out
	zip gmscg_.zip gmscg_ux.out gmscg_us.run optglpk.def install/gmscg_un.dat
	$(install_sh_DATA) gmscg_.zip $(DESTDIR)$(bindir)/gmscg_.zip
	rm -f gmscg_ux.out gmscg_.zip
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsCoinGlpk.exe gmscg_nx.exe
	zip gmscg_.zip gmscg_nx.exe gmscg_nt.cmd optglpk.def install/gmscg_nt.dat
	$(install_sh_DATA) gmscg_.zip $(DESTDIR)$(bindir)/gmscg_.zip
	rm -f gmscg_nx.exe gmscg_.zip
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmscg_.zip

include ../../../BuildTools/Makemain.inc
