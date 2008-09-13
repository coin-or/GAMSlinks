# Copyright (C) GAMS Development 2007-2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmsos_nt.cmd gmsos_us.run install/gmsos_nt.dat install/gmsos_un.dat

bin_PROGRAMS = GamsOS
GamsOS_SOURCES = GamsOS.cpp Smag2OSiL.cpp OSrL2Smag.cpp
GamsOS_LDADD = ../GamsIO/libSmagIO.la ../GamsIO/libSmagStricmp.la
GamsOS_DEPENDENCIES = ../GamsIO/libSmagIO.la ../GamsIO/libSmagStricmp.la

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSAgent` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSParsers` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)`

#OSSolverInterface.la need to come before OSCommonInterfaces.la
if COIN_OS_SOLVER
  AM_CPPFLAGS += -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSSolverInterfaces`
  GamsOS_LDADD += $(OSOBJDIR)/src/OSSolverInterfaces/libOSSolverInterfaces.la
  GamsOS_DEPENDENCIES += $(OSOBJDIR)/src/OSSolverInterfaces/libOSSolverInterfaces.la
endif

GamsOS_LDADD += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 ../GamsIO/libSmagIO.la
 
GamsOS_DEPENDENCIES += \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 ../GamsIO/libSmagIO.la $(SMAG_LIBS)

LIBS += `cat $(OSOBJDIR)/os_addlibs.txt $(GAMSLINKSOBJDIR)/smagio_addlibs.txt`

if COIN_OS_SOLVER

if COIN_HAS_BONMIN
  AM_CPPFLAGS += \
	 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Interfaces` \
	 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms` \
	 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/CbcBonmin` \
	 -I`$(CYGPATH_W) $(BONMINOBJDIR)/inc`
  GamsOS_LDADD += \
	 $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la
  GamsOS_DEPENDENCIES += \
	 $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la
endif

if COIN_HAS_CBC
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiCbc` \
   -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
   -I`$(CYGPATH_W) $(CBCOBJDIR)/inc`
  GamsOS_LDADD += \
   $(OSIOBJDIR)/src/OsiCbc/libOsiCbc.la \
   $(CBCOBJDIR)/src/libCbc.la
  GamsOS_DEPENDENCIES += \
   $(OSIOBJDIR)/src/OsiCbc/libOsiCbc.la \
   $(CBCOBJDIR)/src/libCbc.la
  LIBS += `cat $(CBCOBJDIR)/cbc_addlibs.txt`
endif

if COIN_HAS_CLP
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiClp` \
   -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
   -I`$(CYGPATH_W) $(CLPOBJDIR)/inc`
  GamsOS_LDADD += \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  GamsOS_DEPENDENCIES += \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  LIBS += `cat $(CLPOBJDIR)/clp_addlibs.txt`
endif

if COIN_HAS_GLPK
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` \
   -I`$(CYGPATH_W) $(GLPKINCDIR)`
  GamsOS_LDADD += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
  GamsOS_DEPENDENCIES += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
endif

if COIN_HAS_DYLP
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiDylp` \
   -I`$(CYGPATH_W) $(DYLPSRCDIR)/src/Dylp` \
   -I`$(CYGPATH_W) $(DYLPSRCDIR)/src/DylpStdLib` \
   -I`$(CYGPATH_W) $(DYLPOBJDIR)/inc`
  GamsOS_LDADD += \
    $(OSIOBJDIR)/src/OsiDylp/libOsiDylp.la \
    $(DYLPOBJDIR)/src/Dylp/libDylp.la
  GamsOS_DEPENDENCIES += \
    $(OSIOBJDIR)/src/OsiDylp/libOsiDylp.la \
    $(DYLPOBJDIR)/src/Dylp/libDylp.la
  LIBS += `cat $(DYLPOBJDIR)/dylp_addlibs.txt`
endif  

if COIN_HAS_VOL
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiVol` \
   -I`$(CYGPATH_W) $(VOLSRCDIR)/src`
  GamsOS_LDADD += \
    $(OSIOBJDIR)/src/OsiVol/libOsiVol.la \
    $(VOLOBJDIR)/src/libVol.la
  GamsOS_DEPENDENCIES += \
    $(OSIOBJDIR)/src/OsiVol/libOsiVol.la \
    $(VOLOBJDIR)/src/libVol.la
  LIBS += `cat $(VOLOBJDIR)/vol_addlibs.txt`
endif

if COIN_HAS_SYMPHONY
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiSym` \
   -I`$(CYGPATH_W) $(SYMPHONYSRCDIR)/include`
  GamsOS_LDADD += \
    $(OSIOBJDIR)/src/OsiSym/libOsiSym.la \
    $(SYMPHONYOBJDIR)/src/libSym.la
  GamsOS_DEPENDENCIES += \
    $(OSIOBJDIR)/src/OsiSym/libOsiSym.la \
    $(SYMPHONYOBJDIR)/src/libSym.la
  LIBS += `cat $(SYMPHONYOBJDIR)/sym_addlibs.txt`
endif

if COIN_HAS_CGL
  AM_CPPFLAGS += \
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
   -I`$(CYGPATH_W) $(CGLOBJDIR)/inc` 
  GamsOS_LDADD += $(CGLOBJDIR)/src/libCgl.la
  GamsOS_DEPENDENCIES += $(CGLOBJDIR)/src/libCgl.la
  LIBS += `cat $(CGLOBJDIR)/cgl_addlibs.txt`
endif

if COIN_HAS_OSI
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
   -I`$(CYGPATH_W) $(OSIOBJDIR)/inc`
  GamsOS_LDADD += \
   $(OSIOBJDIR)/src/libOsi.la
  GamsOS_DEPENDENCIES += \
   $(OSIOBJDIR)/src/libOsi.la
  LIBS += `cat $(OSIOBJDIR)/osi_addlibs.txt`
endif

if COIN_HAS_IPOPT
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
   -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
   -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
   -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
   -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
   -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`
  GamsOS_LDADD += \
   $(IPOPTOBJDIR)/src/Interfaces/libipopt.la
  GamsOS_DEPENDENCIES += \
   $(IPOPTOBJDIR)/src/Interfaces/libipopt.la
  LIBS += `cat $(IPOPTOBJDIR)/src/Interfaces/ipopt_addlibs_cpp.txt`
endif

if COIN_HAS_CPX
  AM_CPPFLAGS += \
	-I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiCpx` \
	-I`$(CYGPATH_W) $(CPXINCDIR)`
  GamsOS_LDADD += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la $(CPXLIB)
  GamsOS_DEPENDENCIES += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la
endif

# end of check for COIN_OS_SOLVER
endif

GamsOS_LDADD += \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

GamsOS_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = gmsos_.zip

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsOS gmsos_ux.out
	zip gmsos_.zip gmsos_ux.out gmsos_us.run optos.def install/gmsos_un.dat
	$(install_sh_DATA) gmsos_.zip $(DESTDIR)$(bindir)/gmsos_.zip
	rm -f gmsos_ux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsOS.exe gmsos_nx.exe
	zip gmsos_.zip gmsos_nx.exe gmsos_nt.cmd optos.def install/gmsos_nt.dat
	$(install_sh_DATA) gmsos_.zip $(DESTDIR)$(bindir)/gmsos_.zip
	rm -f gmsos_nx.exe
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsos_.zip

include ../../../BuildTools/Makemain.inc
