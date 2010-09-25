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

# these flags are required for compiling every library and executable  
COMMON_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/..` -I`$(CYGPATH_W) $(srcdir)/../GamsUtils`

# these flags are common when compiling and linking the executables
if COIN_HAS_LIBLTDL
GMSXXX_CPPFLAGS = -I`$(CYGPATH_W) $(LIBLTDLSRCDIR)` $(COMMON_CPPFLAGS)
GMSXXX_LDADD    = ../GamsUtils/gevmcc.lo ../GamsUtils/gmomcc.lo $(LIBLTDLOBJDIR)/libltdlc.la
GMSXXX_LDFLAGS  = -rpath $(libdir) -dlopen libGamsCoin.la
else
GMSXXX_CPPFLAGS = $(COMMON_CPPFLAGS)
GMSXXX_LDADD    = libGamsCoin.la
GMSXXX_LDFLAGS  = -rpath $(libdir) 
endif

LIBS += $(ADDLIBS) `cat $(GAMSLINKSOBJDIR)/gams_addlibs.txt`

noinst_PROGRAMS             =
lib_LTLIBRARIES             = libGamsCoin.la
libGamsCoin_la_SOURCES      = GamsSolver.cpp
libGamsCoin_la_CPPFLAGS     = $(COMMON_CPPFLAGS)
libGamsCoin_la_LIBADD       = ../GamsUtils/libGamsUtils.la
if COIN_HAS_LIBLTDL
libGamsCoin_la_LDFLAGS      = -module -no-undefined
else
libGamsCoin_la_LDFLAGS      =
endif

gamsinstallfiles =

if GAMSLINKS_BUILD_OSI
libGamsCoin_la_SOURCES += GamsOsi.cpp GamsOsiHelper.cpp GamsMessageHandler.cpp

libGamsCoin_la_CPPFLAGS += \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc`

libGamsCoin_la_LIBADD += \
 $(OSIOBJDIR)/src/libOsi.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
LIBS += `cat $(OSIOBJDIR)/osi_addlibs.txt $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

if COIN_HAS_CBC
  libGamsCoin_la_LIBADD  += \
   $(OSIOBJDIR)/src/OsiCbc/libOsiCbc.la \
   $(CBCOBJDIR)/src/libCbc.la \
   $(CGLOBJDIR)/src/libCgl.la \
   $(OSIOBJDIR)/src/OsiClp/libOsiClp.la \
   $(CLPOBJDIR)/src/libClp.la
  LIBS += `cat $(CGLOBJDIR)/cgl_addlibs.txt $(CBCOBJDIR)/cbc_addlibs.txt $(CLPOBJDIR)/clp_addlibs.txt`

  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiCbc` \
   -I`$(CYGPATH_W) $(CBCSRCDIR)/src` \
   -I`$(CYGPATH_W) $(CBCOBJDIR)/inc`
endif

if COIN_HAS_CPX
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiCpx/libOsiCpx.la
#  libGamsCoin_la_LDFLAGS  += $(CPXLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiCpx` -I`$(CYGPATH_W) $(CPXINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsocpux.out
  gmsocpux_out_SOURCES  = gmsmain.cpp
  gmsocpux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiCplex\" -DCREATEFUNCNAME=createNewGamsOsiCplex
  gmsocpux_out_LDADD    = $(GMSXXX_LDADD)
  gmsocpux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsocpux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsocpux.out gmsocpus.run install/gmsocpun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsocpnx
  gmsocpnx_SOURCES  = gmsmain.cpp
  gmsocpnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiCplex\" -DCREATEFUNCNAME=createNewGamsOsiCplex
  gmsocpnx_LDADD    = $(GMSXXX_LDADD)
  gmsocpnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsocpnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsocpnx.exe gmsocpnt.cmd install/gmsocpnt.dat
endif

  EXTRA_DIST       += gmsocpnt.cmd gmsocpus.run install/gmsocpnt.dat install/gmsocpun.dat
endif

if COIN_HAS_GLPK
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` -I`$(CYGPATH_W) $(GLPKINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsoglux.out
  gmsoglux_out_SOURCES  = gmsmain.cpp
  gmsoglux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGlpk\" -DCREATEFUNCNAME=createNewGamsOsiGlpk
  gmsoglux_out_LDADD    = $(GMSXXX_LDADD)
  gmsoglux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsoglux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsoglux.out gmsoglus.run install/gmsoglun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsoglnx
  gmsoglnx_SOURCES  = gmsmain.cpp
  gmsoglnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGlpk\" -DCREATEFUNCNAME=createNewGamsOsiGlpk
  gmsoglnx_LDADD    = $(GMSXXX_LDADD)
  gmsoglnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsoglnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsoglnx.exe gmsoglnt.cmd install/gmsoglnt.dat
endif

  gamsinstallfiles += optglpk.def
  EXTRA_DIST       += optglpk.def optglpk.gms optglpk.txt gmsoglnt.cmd gmsoglus.run install/gmsoglnt.dat install/gmsoglun.dat
endif

if COIN_HAS_GRB
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiGrb/libOsiGrb.la
#  libGamsCoin_la_LDFLAGS  += $(GRBLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGrb` -I`$(CYGPATH_W) $(GRBINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsoguux.out
  gmsoguux_out_SOURCES  = gmsmain.cpp
  gmsoguux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGurobi\" -DCREATEFUNCNAME=createNewGamsOsiGurobi
  gmsoguux_out_LDADD    = $(GMSXXX_LDADD)
  gmsoguux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsoguux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsoguux.out gmsoguus.run install/gmsoguun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsogunx
  gmsogunx_SOURCES  = gmsmain.cpp
  gmsogunx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGurobi\" -DCREATEFUNCNAME=createNewGamsOsiGurobi
  gmsogunx_LDADD    = $(GMSXXX_LDADD)
  gmsogunx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsogunx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsogunx.exe gmsogunt.cmd install/gmsogunt.dat
endif

  EXTRA_DIST       += gmsogunt.cmd gmsoguus.run install/gmsogunt.dat install/gmsoguun.dat
endif

if COIN_HAS_MSK
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiMsk/libOsiMsk.la
#  libGamsCoin_la_LDFLAGS  += $(MSKLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiMsk` -I`$(CYGPATH_W) $(MSKINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsomsux.out
  gmsomsux_out_SOURCES  = gmsmain.cpp
  gmsomsux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiMosek\" -DCREATEFUNCNAME=createNewGamsOsiMosek
  gmsomsux_out_LDADD    = $(GMSXXX_LDADD)
  gmsomsux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsomsux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsomsux.out gmsomsus.run install/gmsomsun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsomsnx
  gmsomsnx_SOURCES  = gmsmain.cpp
  gmsomsnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiMosek\" -DCREATEFUNCNAME=createNewGamsOsiMosek
  gmsomsnx_LDADD    = $(GMSXXX_LDADD)
  gmsomsnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsomsnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsomsnx.exe gmsomsnt.cmd install/gmsomsnt.dat
endif

  EXTRA_DIST       += gmsomsnt.cmd gmsomsus.run install/gmsomsnt.dat install/gmsomsun.dat
endif

if COIN_HAS_XPR
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiXpr/libOsiXpr.la 
#  libGamsCoin_la_LDFLAGS  += $(XPRLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiXpr` -I`$(CYGPATH_W) $(XPRINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsoxpux.out
  gmsoxpux_out_SOURCES  = gmsmain.cpp
  gmsoxpux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiXpress\" -DCREATEFUNCNAME=createNewGamsOsiXpress
  gmsoxpux_out_LDADD    = $(GMSXXX_LDADD)
  gmsoxpux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsoxpux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsoxpux.out gmsoxpus.run install/gmsoxpun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsoxpnx
  gmsoxpnx_SOURCES  = gmsmain.cpp
  gmsoxpnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiXpress\" -DCREATEFUNCNAME=createNewGamsOsiXpress
  gmsoxpnx_LDADD    = $(GMSXXX_LDADD)
  gmsoxpnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsoxpnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsoxpnx.exe gmsoxpnt.cmd install/gmsoxpnt.dat
endif

  EXTRA_DIST       += gmsoxpnt.cmd gmsoxpus.run install/gmsoxpnt.dat install/gmsoxpun.dat
endif

endif

if GAMSLINKS_BUILD_CBC
libGamsCoin_la_SOURCES  += GamsCbc.cpp

libGamsCoin_la_CPPFLAGS += \
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
 -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiClp` \
 -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CLPOBJDIR)/inc`

libGamsCoin_la_LIBADD   += $(CBCOBJDIR)/src/libCbcSolver.la 
 
if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmscbcux.out
  gmscbcux_out_SOURCES  = gmsmain.cpp
  gmscbcux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Cbc\" -DCREATEFUNCNAME=createNewGamsCbc
  gmscbcux_out_LDADD    = $(GMSXXX_LDADD)
  gmscbcux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmscbcux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmscbcux.out gmscbcus.run install/gmscbcun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmscbcnx
  gmscbcnx_SOURCES  = gmsmain.cpp
  gmscbcnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Cbc\" -DCREATEFUNCNAME=createNewGamsCbc
  gmscbcnx_LDADD    = $(GMSXXX_LDADD)
  gmscbcnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmscbcnx_DEPENDENCIES = libGamsCoin.la
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

libGamsCoin_la_CPPFLAGS += \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/LinearSolverLoader` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`

libGamsCoin_la_LIBADD += $(IPOPTOBJDIR)/src/Interfaces/libipopt.la 
LIBS  += `cat $(IPOPTOBJDIR)/src/Interfaces/ipopt_addlibs_cpp.txt`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsipdux.out
  gmsipdux_out_SOURCES  = gmsmain.cpp
  gmsipdux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Ipopt\" -DCREATEFUNCNAME=createNewGamsIpopt
  gmsipdux_out_LDADD    = $(GMSXXX_LDADD)
  gmsipdux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsipdux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsipdux.out gmsipdus.run install/gmsipdun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsipdnx
  gmsipdnx_SOURCES  = gmsmain.cpp
  gmsipdnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Ipopt\" -DCREATEFUNCNAME=createNewGamsIpopt
  gmsipdnx_LDADD    = $(GMSXXX_LDADD)
  gmsipdnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsipdnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsipdnx.exe gmsipdnt.cmd install/gmsipdnt.dat
endif

EXTRA_DIST       += gmsipdnt.cmd gmsipdus.run install/gmsipdnt.dat install/gmsipdun.dat

endif

if GAMSLINKS_BUILD_SCIP
libGamsCoin_la_SOURCES  += GamsScip.cpp
libGamsCoin_la_CPPFLAGS += \
 -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CLPOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(SCIPINCDIR)`

if COIN_BUILD_SCIP
libGamsCoin_la_LIBADD   += $(SCIPOBJDIR)/libcoinscip.la
else
libGamsCoin_la_LDFLAGS  += $(SCIPLIB)
endif 

if COIN_HAS_SOPLEX
if COIN_BUILD_SOPLEX
libGamsCoin_la_LIBADD   += $(SOPLEXOBJDIR)/libcoinsoplex.la
else
libGamsCoin_la_LDFLAGS  += $(SOPLEXLIB)
endif
endif
 
if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsscdux.out
  gmsscdux_out_SOURCES  = gmsmain.cpp
  gmsscdux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Scip\" -DCREATEFUNCNAME=createNewGamsScip
  gmsscdux_out_LDADD    = $(GMSXXX_LDADD)
  gmsscdux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsscdux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsscdux.out gmsscdus.run install/gmsscdun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsscdnx
  gmsscdnx_SOURCES  = gmsmain.cpp
  gmsscdnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Scip\" -DCREATEFUNCNAME=createNewGamsScip
  gmsscdnx_LDADD    = $(GMSXXX_LDADD)
  gmsscdnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsscdnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsscdnx.exe gmsscdnt.cmd install/gmsscdnt.dat
endif

EXTRA_DIST       += gmsscdnt.cmd gmsscdus.run install/gmsscdnt.dat install/gmsscdun.dat
endif

if GAMSLINKS_BUILD_BONMIN
libGamsCoin_la_SOURCES += GamsBonmin.cpp GamsMINLP.cpp

libGamsCoin_la_CPPFLAGS += \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Algorithms` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(BONMINSRCDIR)/src/CbcBonmin` \
 -I`$(CYGPATH_W) $(BONMINOBJDIR)/inc`

libGamsCoin_la_LIBADD   += $(BONMINOBJDIR)/src/CbcBonmin/libbonmin.la

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsbmdux.out
  gmsbmdux_out_SOURCES  = gmsmain.cpp
  gmsbmdux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Bonmin\" -DCREATEFUNCNAME=createNewGamsBonmin
  gmsbmdux_out_LDADD    = $(GMSXXX_LDADD)
  gmsbmdux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsbmdux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsbmdux.out gmsbmdus.run install/gmsbmdun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsbmdnx
  gmsbmdnx_SOURCES  = gmsmain.cpp
  gmsbmdnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Bonmin\" -DCREATEFUNCNAME=createNewGamsBonmin
  gmsbmdnx_LDADD    = $(GMSXXX_LDADD)
  gmsbmdnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsbmdnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsbmdnx.exe gmsbmdnt.cmd install/gmsbmdnt.dat
endif

EXTRA_DIST       += gmsbmdnt.cmd gmsbmdus.run install/gmsbmdnt.dat install/gmsbmdun.dat

endif

if GAMSLINKS_BUILD_COUENNE
libGamsCoin_la_SOURCES += GamsCouenne.cpp

libGamsCoin_la_CPPFLAGS += \
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
	
libGamsCoin_la_LIBADD += $(COUENNEOBJDIR)/src/libCouenne.la $(COUENNEOBJDIR)/src/main/libBonCouenne.la

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmscouux.out
  gmscouux_out_SOURCES  = gmsmain.cpp
  gmscouux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Couenne\" -DCREATEFUNCNAME=createNewGamsCouenne
  gmscouux_out_LDADD    = $(GMSXXX_LDADD)
  gmscouux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmscouux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmscouux.out gmscouus.run install/gmscouun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmscounx
  gmscounx_SOURCES  = gmsmain.cpp
  gmscounx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Couenne\" -DCREATEFUNCNAME=createNewGamsCouenne
  gmscounx_LDADD    = $(GMSXXX_LDADD)
  gmscounx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmscounx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmscounx.exe gmscount.cmd install/gmscount.dat
endif

EXTRA_DIST       += gmscount.cmd gmscouus.run install/gmscount.dat install/gmscouun.dat

endif

if GAMSLINKS_BUILD_OS
libGamsCoin_la_SOURCES += GamsOS.cpp GamsOSxL.cpp

libGamsCoin_la_CPPFLAGS += \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSAgent` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSParsers` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)`

libGamsCoin_la_LIBADD += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la
LIBS += `cat $(OSOBJDIR)/os_addlibs.txt`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmsosdux.out
  gmsosdux_out_SOURCES  = gmsmain.cpp
  gmsosdux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OS\" -DCREATEFUNCNAME=createNewGamsOS
  gmsosdux_out_LDADD    = $(GMSXXX_LDADD)
  gmsosdux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsosdux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsosdux.out gmsosdus.run install/gmsosdun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsosdnx
  gmsosdnx_SOURCES  = gmsmain.cpp
  gmsosdnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OS\" -DCREATEFUNCNAME=createNewGamsOS
  gmsosdnx_LDADD    = $(GMSXXX_LDADD)
  gmsosdnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsosdnx_DEPENDENCIES = libGamsCoin.la
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
