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

noinst_PROGRAMS             =
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
noinst_PROGRAMS     += gmscbcux.out
gmscbcux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmscbcux_out_SOURCES = gmscbc.cpp
gmscbcux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmscbcux.out gmscbcus.run install/gmscbcun.dat
endif

if GAMSIO_IS_WINDOWS
noinst_PROGRAMS  += gmscbcnx
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
noinst_PROGRAMS     += gmsipdux.out
gmsipdux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsipdux_out_SOURCES = gmsipd.cpp
gmsipdux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmsipdux.out gmsipdus.run install/gmsipdun.dat
endif

if GAMSIO_IS_WINDOWS
noinst_PROGRAMS  += gmsipdnx
gmsipdnx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsipdnx_SOURCES  = gmsipd.cpp
gmsipdnx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles += gmsipdnx.exe gmsipdnt.cmd install/gmsipdnt.dat
endif

EXTRA_DIST       += gmsipdnt.cmd gmsipdus.run install/gmsipdnt.dat install/gmsipdun.dat

endif

if GAMSLINKS_BUILD_BONMIN
libGamsCoin_la_SOURCES += GamsBonmin.cpp GamsMINLP.cpp

AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/CbcBonmin` \
 -I`$(CYGPATH_W) $(BONMINOBJDIR)/inc`

libGamsCoin_la_LIBADD       += $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la
libGamsCoin_la_DEPENDENCIES += $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la

if GAMSIO_IS_UNIX
noinst_PROGRAMS     += gmsbmdux.out
gmsbmdux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsbmdux_out_SOURCES = gmsbmd.cpp
gmsbmdux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmsbmdux.out gmsbmdus.run install/gmsbmdun.dat
endif

if GAMSIO_IS_WINDOWS
noinst_PROGRAMS  += gmsbmdnx
gmsbmdnx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsbmdnx_SOURCES  = gmsbmd.cpp
gmsbmdnx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles += gmsbmdnx.exe gmsbmdnt.cmd install/gmsbmdnt.dat
endif

EXTRA_DIST       += gmsbmdnt.cmd gmsbmdus.run install/gmsbmdnt.dat install/gmsbmdun.dat

endif

if GAMSLINKS_BUILD_GLPK
AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` \
 -I`$(CYGPATH_W) $(GLPKINCDIR)`
libGamsCoin_la_LIBADD       += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
libGamsCoin_la_DEPENDENCIES += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
endif

if GAMSLINKS_BUILD_COUENNE
libGamsCoin_la_SOURCES += GamsCouenne.cpp GamsCouenneSetup.cpp

AM_CPPFLAGS += \
  -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms/OaGenerators` \
  -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms/Branching` \
  -I`$(CYGPATH_W) $(CGLSRCDIR)/src/CglLandP` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/bound_tightening` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/branch` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/convex` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/disjunctive` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/expression` \
  -I`$(CYGPATH_W) $(COUENNESRCDIR)/src/expression/operators` \
  -I`$(CYGPATH_W) $(COUENNESRCDIR)/src/expression/simplify` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/main` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/problem` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/readnl` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/standardize` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src/util` \
	-I`$(CYGPATH_W) $(COUENNESRCDIR)/src` \
	-I`$(CYGPATH_W) $(COUENNEOBJDIR)/inc`
	
libGamsCoin_la_LIBADD       += $(COUENNEOBJDIR)/src/libCouenne.la
libGamsCoin_la_DEPENDENCIES += $(COUENNEOBJDIR)/src/libCouenne.la

if GAMSIO_IS_UNIX
noinst_PROGRAMS     += gmscouux.out
gmscouux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmscouux_out_SOURCES = gmscou.cpp
gmscouux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmscouux.out gmscouus.run install/gmscouun.dat
endif

if GAMSIO_IS_WINDOWS
noinst_PROGRAMS  += gmscounx
gmscounx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmscounx_SOURCES  = gmscou.cpp
gmscounx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles += gmscounx.exe gmscount.cmd install/gmscount.dat
endif

EXTRA_DIST       += gmscount.cmd gmscouus.run install/gmscount.dat install/gmscouun.dat

endif

if GAMSLINKS_BUILD_OS
libGamsCoin_la_SOURCES += GamsOS.cpp Gams2OSiL.cpp OSrL2Gams.cpp

AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSAgent` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSParsers` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)`

#OSSolverInterface.la need to come before OSCommonInterfaces.la
if COIN_OS_SOLVER
  AM_CPPFLAGS   += -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSSolverInterfaces`
  libGamsCoin_la_LIBADD       += $(OSOBJDIR)/src/OSSolverInterfaces/libOSSolverInterfaces.la
  libGamsCoin_la_DEPENDENCIES += $(OSOBJDIR)/src/OSSolverInterfaces/libOSSolverInterfaces.la
endif

libGamsCoin_la_LIBADD += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 `cat $(OSOBJDIR)/os_addlibs.txt`

libGamsCoin_la_DEPENDENCIES += \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la

if COIN_OS_SOLVER
if COIN_HAS_CPX
  #might be duplicated if GAMSLINKS_BUILD_CBC is true
  libGamsCoin_la_LIBADD       += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la $(CPXLIB)
  libGamsCoin_la_DEPENDENCIES += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la
endif

if COIN_HAS_DYLP
  libGamsCoin_la_LIBADD       += $(OSIOBJDIR)/src/OsiDylp/libOsiDylp.la $(DYLPOBJDIR)/src/Dylp/libDylp.la `cat $(DYLPOBJDIR)/dylp_addlibs.txt`
  libGamsCoin_la_DEPENDENCIES += $(OSIOBJDIR)/src/OsiDylp/libOsiDylp.la $(DYLPOBJDIR)/src/Dylp/libDylp.la
endif  

if COIN_HAS_VOL
  libGamsCoin_la_LIBADD       += $(OSIOBJDIR)/src/OsiVol/libOsiVol.la $(VOLOBJDIR)/src/libVol.la `cat $(VOLOBJDIR)/vol_addlibs.txt`
  libGamsCoin_la_DEPENDENCIES += $(OSIOBJDIR)/src/OsiVol/libOsiVol.la $(VOLOBJDIR)/src/libVol.la
endif

if COIN_HAS_SYMPHONY
  libGamsCoin_la_LIBADD       += $(OSIOBJDIR)/src/OsiSym/libOsiSym.la $(SYMPHONYOBJDIR)/src/libSym.la `cat $(SYMPHONYOBJDIR)/sym_addlibs.txt`
  libGamsCoin_la_DEPENDENCIES += $(OSIOBJDIR)/src/OsiSym/libOsiSym.la $(SYMPHONYOBJDIR)/src/libSym.la
endif
endif

if GAMSIO_IS_UNIX
noinst_PROGRAMS     += gmsosdux.out
gmsosdux_out_LDFLAGS = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsosdux_out_SOURCES = gmsosd.cpp
gmsosdux_out_LDADD   = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles    += gmsosdux.out gmsosdus.run install/gmsosdun.dat
endif

if GAMSIO_IS_WINDOWS
noinst_PROGRAMS  += gmsosdnx
gmsosdnx_LDFLAGS  = $(ADDLIBS) -rpath $(libdir) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt` -dlopen libGamsCoin.la
gmsosdnx_SOURCES  = gmsosd.cpp
gmsosdnx_LDADD    = $(GAMSIO_OBJDIR)/libGamsGMO.la $(LIBLTDLOBJDIR)/libltdlc.la
gamsinstallfiles += gmsosdnx.exe gmsosdnt.cmd install/gmsosdnt.dat
endif

gamsinstallfiles += optosd.def
EXTRA_DIST       += optosd.def optosd.gms optosd.txt gmsosdnt.cmd gmsosdus.run install/gmsosdnt.dat install/gmsosdun.dat
endif

#ltdl need the GamsCoin.la present for preloading, so we need to install one for debug and static builds
#on the other hand, for shared library builds, only no or the libGamsCoin.la from the lib dir can be used
install-exec-local: install-libLTLIBRARIES
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
