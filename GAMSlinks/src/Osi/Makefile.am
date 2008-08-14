# Copyright (C) GAMS Development 2007
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = \
  gmsosint.cmd gmsosius.run install/gmsosint.dat install/gmsosiun.dat

bin_PROGRAMS = GamsCoinOsi
GamsCoinOsi_SOURCES = GamsCoinOsi.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc`

GamsCoinOsi_LDADD = \
 ../GamsIO/libGamsIO.la
 
GamsCoinOsi_DEPENDENCIES = \
 ../GamsIO/libGamsIO.la $(GAMSIO_LIBS)

LIBS += `cat $(OSIOBJDIR)/osi_addlibs.txt $(GAMSLINKSOBJDIR)/gamsio_addlibs.txt`

if COIN_HAS_CBC
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiCbc` \
   -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
   -I`$(CYGPATH_W) $(CBCOBJDIR)/inc`
  GamsCoinOsi_LDADD += \
   $(OSIOBJDIR)/src/OsiCbc/libOsiCbc.la \
   $(CBCOBJDIR)/src/libCbc.la
  GamsCoinOsi_DEPENDENCIES += \
   $(OSIOBJDIR)/src/OsiCbc/libOsiCbc.la \
   $(CBCOBJDIR)/src/libCbc.la
  LIBS += `cat $(CBCOBJDIR)/cbc_addlibs.txt`
endif

if COIN_HAS_CLP
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiClp` \
   -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
   -I`$(CYGPATH_W) $(CLPOBJDIR)/inc`
  GamsCoinOsi_LDADD += \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  GamsCoinOsi_DEPENDENCIES += \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  LIBS += `cat $(CLPOBJDIR)/clp_addlibs.txt`
endif

if COIN_HAS_GLPK
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` \
   -I`$(CYGPATH_W) $(GLPKINCDIR)`
  GamsCoinOsi_LDADD += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
  GamsCoinOsi_DEPENDENCIES += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
endif

if COIN_HAS_DYLP
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiDylp` \
   -I`$(CYGPATH_W) $(DYLPSRCDIR)/src/Dylp` \
   -I`$(CYGPATH_W) $(DYLPSRCDIR)/src/DylpStdLib` \
   -I`$(CYGPATH_W) $(DYLPOBJDIR)/inc`
  GamsCoinOsi_LDADD += \
    $(OSIOBJDIR)/src/OsiDylp/libOsiDylp.la \
    $(DYLPOBJDIR)/src/Dylp/libDylp.la
  GamsCoinOsi_DEPENDENCIES += \
    $(OSIOBJDIR)/src/OsiDylp/libOsiDylp.la \
    $(DYLPOBJDIR)/src/Dylp/libDylp.la
  LIBS += `cat $(DYLPOBJDIR)/dylp_addlibs.txt`
endif  

if COIN_HAS_VOL
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiVol` \
   -I`$(CYGPATH_W) $(VOLSRCDIR)/src`
  GamsCoinOsi_LDADD += \
    $(OSIOBJDIR)/src/OsiVol/libOsiVol.la \
    $(VOLOBJDIR)/src/libVol.la
  GamsCoinOsi_DEPENDENCIES += \
    $(OSIOBJDIR)/src/OsiVol/libOsiVol.la \
    $(VOLOBJDIR)/src/libVol.la
  LIBS += `cat $(VOLOBJDIR)/vol_addlibs.txt`
endif

if COIN_HAS_SYMPHONY
  AM_CPPFLAGS += \
   -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiSym` \
   -I`$(CYGPATH_W) $(SYMPHONYSRCDIR)/include`
  GamsCoinOsi_LDADD += \
    $(OSIOBJDIR)/src/OsiSym/libOsiSym.la \
    $(SYMPHONYOBJDIR)/src/libSym.la
  GamsCoinOsi_DEPENDENCIES += \
    $(OSIOBJDIR)/src/OsiSym/libOsiSym.la \
    $(SYMPHONYOBJDIR)/src/libSym.la
  LIBS += `cat $(SYMPHONYOBJDIR)/sym_addlibs.txt`
endif

if COIN_HAS_SYMPHONY
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
  GamsCoinOsi_LDADD += $(CGLOBJDIR)/src/libCgl.la
  GamsCoinOsi_DEPENDENCIES += $(CGLOBJDIR)/src/libCgl.la
  LIBS += `cat $(CGLOBJDIR)/cgl_addlibs.txt`
endif

if COIN_HAS_CBC
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
  GamsCoinOsi_LDADD += $(CGLOBJDIR)/src/libCgl.la
  GamsCoinOsi_DEPENDENCIES += $(CGLOBJDIR)/src/libCgl.la
  LIBS += `cat $(CGLOBJDIR)/cgl_addlibs.txt`
endif

if COIN_HAS_CPX
  AM_CPPFLAGS += \
	-I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiCpx` \
	-I`$(CYGPATH_W) $(CPXINCDIR)`
  GamsCoinOsi_LDADD += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la $(CPXLIB)
  GamsCoinOsi_DEPENDENCIES += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la
endif

GamsCoinOsi_DEPENDENCIES += $(OSIOBJDIR)/src/libOsi.la $(COINUTILSOBJDIR)/src/libCoinUtils.la
GamsCoinOsi_LDADD += $(OSIOBJDIR)/src/libOsi.la $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
LIBS+=`cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

GamsCoinOsi_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
 
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES = gmsosi_.zip

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsCoinOsi gmsosiux.out
	zip gmsosi_.zip gmsosiux.out gmsosius.run install/gmsosiun.dat optosi.def
	$(install_sh_DATA) gmsosi_.zip $(DESTDIR)$(bindir)/gmsosi_.zip
	rm -f gmsosiux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsCoinOsi.exe gmsosinx.exe
	zip gmsosi_.zip gmsosinx.exe gmsosint.cmd install/gmsosint.dat optosi.def
	$(install_sh_DATA) gmsosi_.zip $(DESTDIR)$(bindir)/gmsosi_.zip
	rm -f gmsosinx.exe
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsosi_.zip

include ../../../BuildTools/Makemain.inc
