# Copyright (C) GAMS Development 2007
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmsbm_nt.cmd gmsbm_us.run install/gmsbm_nt.dat install/gmsbm_un.dat

bin_PROGRAMS = GamsBonmin
GamsBonmin_SOURCES = GamsBonmin.cpp \
  SmagMINLP.cpp SmagMINLP.hpp \
  SmagMessageHandler.cpp SmagMessageHandler.hpp \
  ../Ipopt/SmagNLP.cpp ../Ipopt/SmagJournal.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../Ipopt` \
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
 -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CBCOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/CbcBonmin` \
 -I`$(CYGPATH_W) $(BONMINOBJDIR)/inc`

GamsBonmin_LDADD = \
 $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(IPOPTOBJDIR)/src/Interfaces/libipopt.la \
 $(SMAG_LIBS)

GamsBonmin_DEPENDENCIES = \
 $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la \
 $(CBCOBJDIR)/src/libCbc.la \
 $(CGLOBJDIR)/src/libCgl.la \
 $(OSIOBJDIR)/src/libOsi.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la \
 $(IPOPTOBJDIR)/src/Interfaces/libipopt.la

if COIN_HAS_CLP
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiClp` \
   -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
   -I`$(CYGPATH_W) $(CLPOBJDIR)/inc`
  GamsBonmin_LDADD += \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  GamsBonmin_DEPENDENCIES += \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  LIBS += `cat $(CLPOBJDIR)/clp_addlibs.txt`
endif

if COIN_HAS_GLPK
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` \
   -I`$(CYGPATH_W) $(GLPKINCDIR)`
  GamsBonmin_LDADD += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
  GamsBonmin_DEPENDENCIES += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
endif

LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt $(CGLOBJDIR)/cgl_addlibs.txt $(OSIOBJDIR)/osi_addlibs.txt $(CBCOBJDIR)/cbc_addlibs.txt $(IPOPTOBJDIR)/src/Interfaces/ipopt_addlibs_cpp.txt`

GamsBonmin_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
 
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = gmsbm_.zip

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsBonmin gmsbm_ux.out
	zip gmsbm_.zip gmsbm_ux.out gmsbm_us.run install/gmsbm_un.dat
	$(install_sh_DATA) gmsbm_.zip $(DESTDIR)$(bindir)/gmsbm_.zip
	rm -f gmsbm_ux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsBonmin.exe gmsbm_nx.exe
	zip gmsbm_.zip gmsbm_nx.exe gmsbm_nt.cmd install/gmsbm_nt.dat
	$(install_sh_DATA) gmsbm_.zip $(DESTDIR)$(bindir)/gmsbm_.zip
	rm -f gmsbm_nx.exe
endif
	
uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsbm_.zip

include ../../../BuildTools/Makemain.inc
