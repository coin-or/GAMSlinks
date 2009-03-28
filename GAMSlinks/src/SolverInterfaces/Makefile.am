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

lib_LTLIBRARIES             = libGamsCoin.la
libGamsCoin_la_SOURCES      =
libGamsCoin_la_LIBADD       =
libGamsCoin_la_DEPENDENCIES =
libGamsCoin_la_LDFLAGS      =

AM_CPPFLAGS =

gamsinstallfiles =

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
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 `cat $(CGLOBJDIR)/cgl_addlibs.txt $(OSIOBJDIR)/osi_addlibs.txt $(CBCOBJDIR)/cbc_addlibs.txt $(CLPOBJDIR)/clp_addlibs.txt $(COINUTILSOBJDIR)/coinutils_addlibs.txt`
 
libGamsCoin_la_DEPENDENCIES += \
 $(CBCOBJDIR)/src/libCbcSolver.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
 $(CLPOBJDIR)/src/libClp.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la

if GAMSIO_IS_UNIX
bin_PROGRAMS         = gmscbcux.out
gmscbcux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt`
gmscbcux_out_SOURCES = gmscbc.cpp ../GamsUtils/GamsLibLoader.c
gmscbcux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la
gamsinstallfiles    += gmscbcux.out gmscbcus.run install/gmscbcun.dat
endif

if GAMSIO_IS_WINDOWS
bin_PROGRAMS      = gmscbcnx
gmscbcnx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt`
gmscbcnx_SOURCES  = gmscbc.cpp ../GamsUtils/GamsLibLoader.c
gmscbcnx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la
gamsinstallfiles += gmscbcnx.exe gmscbcnt.cmd install/gmscbcnt.dat
endif

gamsinstallfiles += optcbcd.def
EXTRA_DIST       += optcbcd.def optcbcd.gms optcbcd.txt gmscbcnt.cmd gmscbcus.run install/gmscbcnt.dat install/gmscbcun.dat
endif

libGamsCoin_la_LIBADD       += ../GamsUtils/libGamsUtils.la
libGamsCoin_la_DEPENDENCIES += ../GamsUtils/libGamsUtils.la
libGamsCoin_la_LDFLAGS      += -export-dynamic

install-exec-local: install-binPROGRAMS install-libLTLIBRARIES
	test -e libGamsCoin.$(SHAREDLIBEXT) || ln -sf $(DESTDIR)$(libdir)/libGamsCoin.$(SHAREDLIBEXT) libGamsCoin.$(SHAREDLIBEXT)
	zip gmscoin_.zip $(gamsinstallfiles) libGamsCoin.$(SHAREDLIBEXT)
	$(install_sh_DATA) gmscoin_.zip $(DESTDIR)$(bindir)/gmscoin_.zip
	rm -f gmscoin_.zip

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmscoin_.zip

DISTCLEANFILES += gmscoin_.zip

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

include ../../../BuildTools/Makemain.inc
