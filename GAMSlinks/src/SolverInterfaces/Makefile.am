# Copyright (C) GAMS Development and others 2009
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST     =
DISTCLEANFILES =

lib_LTLIBRARIES          = libGamsCoin.la
libGamsCoin_la_SOURCES      =
libGamsCoin_la_LIBADD       =
libGamsCoin_la_DEPENDENCIES =

AM_CPPFLAGS =

installgoals   =
uninstallgoals =

if GAMSLINKS_BUILD_CBC
libGamsCoin_la_SOURCES += GamsCbc.cpp GamsOsiHelper.cpp GamsMessageHandler.cpp

AM_CPPFLAGS += $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsUtils` \
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

libGamsCoin_la_LIBADD += \
 $(CBCOBJDIR)/src/libCbcSolver.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
libGamsCoin_la_DEPENDENCIES += \
 $(CBCOBJDIR)/src/libCbcSolver.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la

if GAMSIO_IS_UNIX
bin_PROGRAMS = gmscbcux.out
gmscbcux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
gmscbcux_out_SOURCES = gmscbc.cpp
gmscbcux_out_LDADD = libGamsCoin.la

installcbc:
	ln -sf $(DESTDIR)$(bindir)/gmscbcux.out gmscbcux.out
	zip gmscbc_.zip gmscbcux.out gmscbcus.run optcbcd.def install/gmscbcun.dat
	$(install_sh_DATA) gmscbc_.zip $(DESTDIR)$(bindir)/gmscbc_.zip
	rm -f gmscbcux.out
endif

if GAMSIO_IS_WINDOWS
bin_PROGRAMS = gmscbcnx
gmscbcnx_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
gmscbcnx_SOURCES = gmscbc.cpp
gmscbcnx_LDADD   = libGamsCoin.la

installcbc:
	ln -sf $(DESTDIR)$(bindir)/gmscbcnx.exe gmscbcnx.exe
	zip gmscbc_.zip gmscbcnx.exe gmscbcnt.cmd optcbcd.def install/gmscbcnt.dat
	$(install_sh_DATA) gmscbc_.zip $(DESTDIR)$(bindir)/gmscbc_.zip
	rm -f gmscbcnx.exe
endif

uninstallcbc:
	rm -f $(DESTDIR)$(bindir)/gmscbc_.zip

EXTRA_DIST     += optcbcd.def optcbcd.gms optcbcd.txt gmscbcnt.cmd gmscbcus.run install/gmscbcnt.dat install/gmscbcun.dat
DISTCLEANFILES += gmscbc_.zip

installgoals   += installcbc
uninstallgoals += uninstallcbc

endif

libGamsCoin_la_LIBADD       += ../GamsUtils/libGamsUtils.la
libGamsCoin_la_DEPENDENCIES += ../GamsUtils/libGamsUtils.la

LIBS += `cat $(CGLOBJDIR)/cgl_addlibs.txt $(OSIOBJDIR)/osi_addlibs.txt $(CBCOBJDIR)/cbc_addlibs.txt $(CLPOBJDIR)/clp_addlibs.txt $(COINUTILSOBJDIR)/coinutils_addlibs.txt $(GAMSLINKSOBJDIR)/gams_addlibs.txt`

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

install-exec-local: install-binPROGRAMS $(installgoals)

uninstall-local: $(uninstallgoals)

include ../../../BuildTools/Makemain.inc
