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

bin_PROGRAMS   =
lib_LTLIBRARIES             = libGamsCoin.la
libGamsCoin_la_SOURCES      =
libGamsCoin_la_LIBADD       = ../GamsUtils/libGamsUtils.la
libGamsCoin_la_DEPENDENCIES = ../GamsUtils/libGamsUtils.la
libGamsCoin_la_LDFLAGS      = -module -no-undefined

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsUtils` \
 -I`$(CYGPATH_W) $(LIBLTDLSRCDIR)`

gamsinstallfiles =

if GAMSLINKS_BUILD_CBC
libGamsCoin_la_SOURCES += GamsCbc.cpp GamsOsiHelper.cpp GamsMessageHandler.cpp

AM_CPPFLAGS += \
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
 
if COIN_HAS_CPX
  libGamsCoin_la_LIBADD       += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la $(CPXLIB)
  libGamsCoin_la_DEPENDENCIES += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la
endif

if GAMSIO_IS_UNIX
bin_PROGRAMS        += gmscbcux.out
gmscbcux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmscbcux_out_SOURCES = gmscbc.cpp
gmscbcux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmscbcux.out gmscbcus.run install/gmscbcun.dat
endif

if GAMSIO_IS_WINDOWS
bin_PROGRAMS     += gmscbcnx
gmscbcnx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmscbcnx_SOURCES  = gmscbc.cpp
gmscbcnx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles += gmscbcnx.exe gmscbcnt.cmd install/gmscbcnt.dat
endif

gamsinstallfiles += optcbcd.def
EXTRA_DIST       += optcbcd.def optcbcd.gms optcbcd.txt gmscbcnt.cmd gmscbcus.run install/gmscbcnt.dat install/gmscbcun.dat
endif

if GAMSLINKS_BUILD_IPOPT
libGamsCoin_la_SOURCES += GamsIpopt.cpp GamsNLP.cpp GamsJournal.cpp

includecoindir = $(includedir)/coin
includecoin_HEADERS = \
  $(IPOPTSRCDIR)/src/Algorithm/IpIpoptCalculatedQuantities.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpIpoptNLP.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpNLPScaling.hpp \
  $(IPOPTSRCDIR)/src/LinAlg/IpScaledMatrix.hpp \
  $(IPOPTSRCDIR)/src/LinAlg/IpSymScaledMatrix.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpIpoptData.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpIteratesVector.hpp \
  $(IPOPTSRCDIR)/src/LinAlg/IpCompoundVector.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpTimingStatistics.hpp \
  $(IPOPTSRCDIR)/src/Interfaces/IpTNLPAdapter.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpOrigIpoptNLP.hpp

AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/LinearSolverLoader` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`

libGamsCoin_la_LIBADD       += $(IPOPTOBJDIR)/src/Interfaces/libipopt.la `cat $(IPOPTOBJDIR)/src/Interfaces/ipopt_addlibs_cpp.txt`
libGamsCoin_la_DEPENDENCIES += $(IPOPTOBJDIR)/src/Interfaces/libipopt.la

if GAMSIO_IS_UNIX
bin_PROGRAMS        += gmsipdux.out
gmsipdux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsipdux_out_SOURCES = gmsipd.cpp
gmsipdux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmsipdux.out gmsipdus.run install/gmsipdun.dat
endif

if GAMSIO_IS_WINDOWS
bin_PROGRAMS     += gmsipdnx
gmsipdnx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsipdnx_SOURCES  = gmsipd.cpp
gmsipdnx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles += gmsipdnx.exe gmsipdnt.cmd install/gmsipdnt.dat
endif

gamsinstallfiles += 
EXTRA_DIST       += gmsipdnt.cmd gmsipdus.run install/gmsipdnt.dat install/gmsipdun.dat

endif

#ltdl need the GamsCoin.la present for preloading, so we need to install one for debug and static builds
#on the other hand, for shared library builds, only no or the libGamsCoin.la from the lib dir can be used
install-exec-local: install-binPROGRAMS install-libLTLIBRARIES
	mv libGamsCoin.la libGamsCoin.la.save
	cp $(DESTDIR)$(libdir)/libGamsCoin.la libGamsCoin.la
	zip gmscoin_.zip $(gamsinstallfiles) libGamsCoin.la
	$(install_sh_DATA) gmscoin_.zip $(DESTDIR)$(bindir)/gmscoin_.zip
	rm -f gmscoin_.zip
	mv -f libGamsCoin.la.save libGamsCoin.la

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmscoin_.zip

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

include ../../../BuildTools/Makemain.inc
