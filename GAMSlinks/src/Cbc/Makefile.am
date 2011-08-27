# Copyright (C) 2006-2008 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 639 2009-04-03 21:55:57Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = optcbc.def optcbc.gms optcbc.txt \
  gmscc_nt.cmd gmscc_us.run install/gmscc_nt.dat install/gmscc_un.dat

bin_PROGRAMS = GamsCoinCbc
GamsCoinCbc_SOURCES = GamsCoinCbc.cpp GamsCutGenerator.cpp GamsHeuristic.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglClique` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglDuplicateRow` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglFlowCover` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglGomory` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglKnapsackCover` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglMixedIntegerRounding2` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglOddHole` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglPreProcess` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglProbing` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglRedSplit` \
 -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglTwomir` \
 -I`$(CYGPATH_W) $(CGLOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiClp` \
 -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CLPOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CBCOBJDIR)/inc`

GamsCoinCbc_LDADD = \
 $(CBCOBJDIR)/src/libCbcSolver.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 ../GamsIO/libGamsIO.la
 
GamsCoinCbc_DEPENDENCIES = \
 ../GamsIO/libGamsIO.la $(GAMSIO_LIBS) \
 $(CBCOBJDIR)/src/libCbcSolver.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la

if COIN_HAS_CPX
  GamsCoinCbc_LDADD += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la $(CPXLIB)
  GamsCoinCbc_DEPENDENCIES += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la
endif

LIBS += `cat $(CGLOBJDIR)/cgl_addlibs.txt $(OSIOBJDIR)/osi_addlibs.txt $(CBCOBJDIR)/cbc_addlibs.txt $(CLPOBJDIR)/clp_addlibs.txt $(GAMSLINKSOBJDIR)/gamsio_addlibs.txt $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

GamsCoinCbc_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
 
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsCoinCbc gmscc_ux.out
	zip gmscc_.zip gmscc_ux.out gmscc_us.run optcbc.def install/gmscc_un.dat
	$(install_sh_DATA) gmscc_.zip $(DESTDIR)$(bindir)/gmscc_.zip
	rm -f gmscc_ux.out gmscc_.zip
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsCoinCbc.exe gmscc_nx.exe
	zip gmscc_.zip gmscc_nx.exe gmscc_nt.cmd optcbc.def install/gmscc_nt.dat
	$(install_sh_DATA) gmscc_.zip $(DESTDIR)$(bindir)/gmscc_.zip
	rm -f gmscc_nx.exe gmscc_.zip
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmscc_.zip

include ../../../BuildTools/Makemain.inc
