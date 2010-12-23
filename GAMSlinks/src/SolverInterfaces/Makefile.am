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
COMMON_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/..` -I`$(CYGPATH_W) $(srcdir)/../GamsUtils` -I`$(CYGPATH_W) $(srcdir)/../GamsUtils/$(GAMS_VERSION)`

# these flags are common when compiling and linking the executables
if COIN_HAS_LIBLTDL
GMSXXX_CPPFLAGS = -I`$(CYGPATH_W) $(LIBLTDLSRCDIR)` $(COMMON_CPPFLAGS)
GMSXXX_LDADD    = ../GamsUtils/gevmcc.lo ../GamsUtils/gmomcc.lo ../GamsUtils/gclgms.lo $(LIBLTDLOBJDIR)/libltdlc.la
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
  noinst_PROGRAMS      += gmszcpux.out
  gmszcpux_out_SOURCES  = gmsmain.cpp
  gmszcpux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiCplex\" -DCREATEFUNCNAME=createNewGamsOsiCplex
  gmszcpux_out_LDADD    = $(GMSXXX_LDADD)
  gmszcpux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszcpux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmszcpux.out gmszcpus.run install/gmszcpun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmszcpnx
  gmszcpnx_SOURCES  = gmsmain.cpp
  gmszcpnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiCplex\" -DCREATEFUNCNAME=createNewGamsOsiCplex
  gmszcpnx_LDADD    = $(GMSXXX_LDADD)
  gmszcpnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszcpnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmszcpnx.exe gmszcpnt.cmd install/gmszcpnt.dat
endif

  EXTRA_DIST       += gmszcpnt.cmd gmszcpus.run install/gmszcpnt.dat install/gmszcpun.dat
endif

if COIN_HAS_GLPK
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiGlpk/libOsiGlpk.la
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGlpk` -I`$(CYGPATH_W) $(GLPKINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmszglux.out
  gmszglux_out_SOURCES  = gmsmain.cpp
  gmszglux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGlpk\" -DCREATEFUNCNAME=createNewGamsOsiGlpk
  gmszglux_out_LDADD    = $(GMSXXX_LDADD)
  gmszglux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszglux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmszglux.out gmszglus.run install/gmszglun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmszglnx
  gmszglnx_SOURCES  = gmsmain.cpp
  gmszglnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGlpk\" -DCREATEFUNCNAME=createNewGamsOsiGlpk
  gmszglnx_LDADD    = $(GMSXXX_LDADD)
  gmszglnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszglnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmszglnx.exe gmszglnt.cmd install/gmszglnt.dat
endif

  gamsinstallfiles += optmyglpk.def
  EXTRA_DIST       += optmyglpk.def optmyglpk.gms optmyglpk.txt gmszglnt.cmd gmszglus.run install/gmszglnt.dat install/gmszglun.dat
endif

if COIN_HAS_GRB
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiGrb/libOsiGrb.la
#  libGamsCoin_la_LDFLAGS  += $(GRBLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiGrb` -I`$(CYGPATH_W) $(GRBINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmszguux.out
  gmszguux_out_SOURCES  = gmsmain.cpp
  gmszguux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGurobi\" -DCREATEFUNCNAME=createNewGamsOsiGurobi
  gmszguux_out_LDADD    = $(GMSXXX_LDADD)
  gmszguux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszguux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmszguux.out gmszguus.run install/gmszguun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmszgunx
  gmszgunx_SOURCES  = gmsmain.cpp
  gmszgunx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiGurobi\" -DCREATEFUNCNAME=createNewGamsOsiGurobi
  gmszgunx_LDADD    = $(GMSXXX_LDADD)
  gmszgunx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszgunx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmszgunx.exe gmszgunt.cmd install/gmszgunt.dat
endif

  EXTRA_DIST       += gmszgunt.cmd gmszguus.run install/gmszgunt.dat install/gmszguun.dat
endif

if COIN_HAS_MSK
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiMsk/libOsiMsk.la
#  libGamsCoin_la_LDFLAGS  += $(MSKLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiMsk` -I`$(CYGPATH_W) $(MSKINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmszmkux.out
  gmszmkux_out_SOURCES  = gmsmain.cpp
  gmszmkux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiMosek\" -DCREATEFUNCNAME=createNewGamsOsiMosek
  gmszmkux_out_LDADD    = $(GMSXXX_LDADD)
  gmszmkux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszmkux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmszmkux.out gmszmkus.run install/gmszmkun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmszmknx
  gmszmknx_SOURCES  = gmsmain.cpp
  gmszmknx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiMosek\" -DCREATEFUNCNAME=createNewGamsOsiMosek
  gmszmknx_LDADD    = $(GMSXXX_LDADD)
  gmszmknx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszmknx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmszmknx.exe gmszmknt.cmd install/gmszmknt.dat
endif

  EXTRA_DIST       += gmszmknt.cmd gmszmkus.run install/gmszmknt.dat install/gmszmkun.dat
endif

if COIN_HAS_XPR
  libGamsCoin_la_LIBADD   += $(OSIOBJDIR)/src/OsiXpr/libOsiXpr.la 
#  libGamsCoin_la_LDFLAGS  += $(XPRLIB)
  libGamsCoin_la_CPPFLAGS += -I`$(CYGPATH_W) $(OSISRCDIR)/src/OsiXpr` -I`$(CYGPATH_W) $(XPRINCDIR)`

if GAMSIO_IS_UNIX
  noinst_PROGRAMS      += gmszxpux.out
  gmszxpux_out_SOURCES  = gmsmain.cpp
  gmszxpux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiXpress\" -DCREATEFUNCNAME=createNewGamsOsiXpress
  gmszxpux_out_LDADD    = $(GMSXXX_LDADD)
  gmszxpux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszxpux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmszxpux.out gmszxpus.run install/gmszxpun.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmszxpnx
  gmszxpnx_SOURCES  = gmsmain.cpp
  gmszxpnx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OsiXpress\" -DCREATEFUNCNAME=createNewGamsOsiXpress
  gmszxpnx_LDADD    = $(GMSXXX_LDADD)
  gmszxpnx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmszxpnx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmszxpnx.exe gmszxpnt.cmd install/gmszxpnt.dat
endif

  EXTRA_DIST       += gmszxpnt.cmd gmszxpus.run install/gmszxpnt.dat install/gmszxpun.dat
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

gamsinstallfiles += optmycbc.def
EXTRA_DIST       += optmycbc.def optmycbc.gms optmycbc.txt gmscbcnt.cmd gmscbcus.run install/gmscbcnt.dat install/gmscbcun.dat
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
  noinst_PROGRAMS      += gmsip_ux.out
  gmsip_ux_out_SOURCES  = gmsmain.cpp
  gmsip_ux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Ipopt\" -DCREATEFUNCNAME=createNewGamsIpopt
  gmsip_ux_out_LDADD    = $(GMSXXX_LDADD)
  gmsip_ux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsip_ux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsip_ux.out gmsip_us.run install/gmsip_un.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsip_nx
  gmsip_nx_SOURCES  = gmsmain.cpp
  gmsip_nx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Ipopt\" -DCREATEFUNCNAME=createNewGamsIpopt
  gmsip_nx_LDADD    = $(GMSXXX_LDADD)
  gmsip_nx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsip_nx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsip_nx.exe gmsip_nt.cmd install/gmsip_nt.dat
endif

EXTRA_DIST       += gmsip_nt.cmd gmsip_us.run install/gmsip_nt.dat install/gmsip_un.dat

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
  noinst_PROGRAMS      += gmssc_ux.out
  gmssc_ux_out_SOURCES  = gmsmain.cpp
  gmssc_ux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Scip\" -DCREATEFUNCNAME=createNewGamsScip
  gmssc_ux_out_LDADD    = $(GMSXXX_LDADD)
  gmssc_ux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmssc_ux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmssc_ux.out gmssc_us.run install/gmssc_un.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmssc_nx
  gmssc_nx_SOURCES  = gmsmain.cpp
  gmssc_nx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Scip\" -DCREATEFUNCNAME=createNewGamsScip
  gmssc_nx_LDADD    = $(GMSXXX_LDADD)
  gmssc_nx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmssc_nx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmssc_nx.exe gmssc_nt.cmd install/gmssc_nt.dat
endif

EXTRA_DIST       += gmssc_nt.cmd gmssc_us.run install/gmssc_nt.dat install/gmssc_un.dat
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
  noinst_PROGRAMS      += gmsbm_ux.out
  gmsbm_ux_out_SOURCES  = gmsmain.cpp
  gmsbm_ux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Bonmin\" -DCREATEFUNCNAME=createNewGamsBonmin
  gmsbm_ux_out_LDADD    = $(GMSXXX_LDADD)
  gmsbm_ux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsbm_ux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsbm_ux.out gmsbm_us.run install/gmsbm_un.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsbm_nx
  gmsbm_nx_SOURCES  = gmsmain.cpp
  gmsbm_nx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"Bonmin\" -DCREATEFUNCNAME=createNewGamsBonmin
  gmsbm_nx_LDADD    = $(GMSXXX_LDADD)
  gmsbm_nx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsbm_nx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsbm_nx.exe gmsbm_nt.cmd install/gmsbm_nt.dat
endif

EXTRA_DIST       += gmsbm_nt.cmd gmsbm_us.run install/gmsbm_nt.dat install/gmsbm_un.dat

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
  noinst_PROGRAMS      += gmsos_ux.out
  gmsos_ux_out_SOURCES  = gmsmain.cpp
  gmsos_ux_out_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OS\" -DCREATEFUNCNAME=createNewGamsOS
  gmsos_ux_out_LDADD    = $(GMSXXX_LDADD)
  gmsos_ux_out_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsos_ux_out_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles     += gmsos_ux.out gmsos_us.run install/gmsos_un.dat
endif

if GAMSIO_IS_WINDOWS
  noinst_PROGRAMS  += gmsos_nx
  gmsos_nx_SOURCES  = gmsmain.cpp
  gmsos_nx_CPPFLAGS = $(GMSXXX_CPPFLAGS) -DSOLVERNAME=\"OS\" -DCREATEFUNCNAME=createNewGamsOS
  gmsos_nx_LDADD    = $(GMSXXX_LDADD)
  gmsos_nx_LDFLAGS  = $(GMSXXX_LDFLAGS)
  gmsos_nx_DEPENDENCIES = libGamsCoin.la
  gamsinstallfiles += gmsos_nx.exe gmsos_nt.cmd install/gmsos_nt.dat
endif

gamsinstallfiles += optmyos.def
EXTRA_DIST       += optmyos.def optmyos.gms optmyos.txt gmsos_nt.cmd gmsos_us.run install/gmsos_nt.dat install/gmsos_un.dat
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
