# Copyright (C) 2006-2009 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([GAMSlinks],[trunk],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) 2006-2011 GAMS Development and others 
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/GamsIO/GamsModel.hpp)

# Where should everything be installed by default?  Here, we want it
# to be installed directly in 'bin', 'lib', 'include' subdirectories
# of the directory where configure is run.  The default would be
# /usr/local.
AC_PREFIX_DEFAULT([`pwd`])

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the system type
AC_CANONICAL_BUILD

#turn off warnings and pedantic mode since it seem to make trouble with gams *cc files
#gcc 4.2.1 error: ISO C forbids conversion of object pointer to function pointer type
coin_skip_warn_cflags=yes

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(GAMSlinks)

# We need egrep and ln -s somewhere
AC_PROG_EGREP
AC_PROG_LN_S

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Get the C++ runtime libraries in case we want to link with a C or
# Fortran compiler
AC_COIN_CXXLIBS

# Check if we have a Fortran compiler; if we do, acquire the appropriate
# options. This has to be done before we call AC_COIN_INIT_AUTO_TOOLS, else
# it will helpfully set FFLAGS to an incorrect value.
AC_COIN_FIND_F77
if test "$F77" != unavailable ; then
  AC_COIN_PROG_F77
  AC_COIN_F77_WRAPPERS
fi

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

#############################################################################
#                      COIN-OR and other components                         #
#############################################################################

AC_COIN_HAS_PROJECT(GAMSlinks)
AC_COIN_HAS_PROJECT(libltdl)
AC_COIN_HAS_PROJECT(Cgl)
AC_COIN_HAS_PROJECT(CoinUtils)
AC_COIN_HAS_PROJECT(Osi)
AC_COIN_HAS_PROJECT(Clp)
AC_COIN_HAS_PROJECT(Cbc)
AC_COIN_HAS_PROJECT(Ipopt)
AC_COIN_HAS_PROJECT(Bonmin)
AC_COIN_HAS_PROJECT(cppad)
AC_COIN_HAS_PROJECT(Couenne)
AC_COIN_HAS_PROJECT(OS)
AC_COIN_HAS_PROJECT(libltdl)

############################################################################
############################################################################
#                   Stuff that we need for C++ programs                    #
############################################################################
############################################################################

AC_LANG_PUSH(C++)

################
# Header Files #
################


AC_COIN_CHECK_CXX_CHEADER(assert)
AC_COIN_CHECK_CXX_CHEADER(stdio)
AC_COIN_CHECK_CXX_CHEADER(stdlib)
AC_COIN_CHECK_CXX_CHEADER(string)
AC_COIN_CHECK_CXX_CHEADER(stdarg)
AC_COIN_CHECK_CXX_CHEADER(limits)
AC_COIN_CHECK_CXX_CHEADER(ctype)
AC_COIN_CHECK_CXX_CHEADER(errno)
AC_COIN_CHECK_CXX_CHEADER(math)
AC_COIN_CHECK_HEADER([windows.h])

###########
# va_copy #
###########

AC_CHECK_DECL([va_copy],[AC_DEFINE([HAVE_VA_COPY],[1],
                                   [Define to 1 if va_copy is available])],,[
#ifdef HAVE_CSTDARG
# include <cstdarg>
#else
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# endif
#endif])

###############################################
# stricmp and strcasecmp; snprintf; vsnprintf #
###############################################

AC_CHECK_FUNCS([stricmp strcasecmp],[break])
AC_CHECK_FUNCS([snprintf _snprintf],[break])
AC_CHECK_FUNCS([vsnprintf _vsnprintf],[break])

##########################################################################

# Additional libraries and packages

# We need to restore the value of LIBS before those test, otherwise libtool
# will hick-up
LIBS_save="$LIBS"
ADDLIBS_save="$ADDLIBS"

################
# GLPK LIBRARY #
################

AC_COIN_HAS_GLPK(Glpk)

##################
# SOPLEX LIBRARY #
##################

AC_COIN_HAS_SOPLEX

################
# SCIP LIBRARY #
################

AC_COIN_HAS_SCIP

# including scip.h gives warnings about long long that result in errors if -pedantic-errors is used
# thus, we disable this warning if SCIP is used
if test "x$coin_has_scip" = xtrue ; then
  case "$CC" in
    *gcc* )
      CXXFLAGS="$CXXFLAGS -Wno-long-long"
    ;;
  esac
fi

################
# CBC parallel #
################

AC_ARG_ENABLE([cbc-parallel],,
  AC_DEFINE([CBC_THREAD],[1],[Define to 1 if the SMP version of CBC is compiled])
)

#############################
# parallel BLAS recognition #
#############################

LIBS_save="$LIBS"
ADDLIBS_save="$ADDLIBS"
LIBS="$BLAS_LIBS $LIBS"
AC_COIN_HAS_BLAS
AC_CHECK_FUNC([goto_set_num_threads],
  AC_DEFINE([HAVE_GOTO_SETNUMTHREADS],[1],[Define to 1 if the Blas library has goto_set_num_threads])
)
AC_CHECK_FUNC([MKL_Domain_Set_Num_Threads],
  AC_DEFINE([HAVE_MKL_SETNUMTHREADS],[1],[Define to 1 if the Blas library has mkl_set_num_threads])
)
LIBS="$LIBS_save"
ADDLIBS="$ADDLIBS_save"

#################################
# LP/MIP Solvers for Gams/OSI   #
#################################

AC_COIN_HAS_USER_LIBRARY([Cplex],[CPX],[cplex.h],[CPXgetstat])
AC_COIN_HAS_USER_LIBRARY([Gurobi],[GRB],[gurobi_c.h],[GRBloadenv])
AC_COIN_HAS_USER_LIBRARY([Soplex],[SPX],[spxsolver.h])
AC_COIN_HAS_USER_LIBRARY([Xpress],[XPR],[xprs.h],[XPRSinit])
AC_COIN_HAS_USER_LIBRARY([Mosek],[MSK],[mosek.h],[MSK_makeenv])

LIBS=${LIBS_save}
ADDLIBS=${ADDLIBS_save}

####################################
# GAMS system and GAMS I/O LIBRARY #
####################################

AC_COIN_HAVE_GAMS

# at least for running gdb, pthread may need to be linked into executables
case $GAMSIO_CODE in
  LNX | LEG | LX3 | LEI | SIG | DII | DIG )
      GAMS_ADDLIBS="-ldl -lpthread"
    ;;
esac
AC_SUBST(GAMS_ADDLIBS)

#############################################################################
# Check which solvers we build and which interfaces we can (and want) build #
#############################################################################

if test "x$coin_has_clp" != xunavailable && test "x$coin_has_clp" != skipping ; then
  coin_build_clp=1
else
  coin_build_clp=0
fi
AC_SUBST(coin_build_clp)

if test "x$coin_has_cbc" != xunavailable && test "x$coin_has_cbc" != skipping &&
   test "x$coin_has_cgl" != xunavailable && test "x$coin_has_cgl" != skipping ; then
  coin_build_cbc=1
else
  coin_build_cbc=0
fi
AC_SUBST(coin_build_cbc)

if test "x$coin_has_glpk" = xtrue ; then
  coin_build_glpk=1
else
  coin_build_glpk=0
fi
AC_SUBST(coin_build_glpk)

if test "x$coin_has_ipopt" != "xunavailable" && test "x$coin_has_ipopt" != "xskipping"; then
  gamslinks_build_ipopt=1
else
  gamslinks_build_ipopt=0
fi
AC_SUBST(gamslinks_build_ipopt)
AM_CONDITIONAL([GAMSLINKS_BUILD_IPOPT], [test $gamslinks_build_ipopt = 1])

if test "x$coin_has_coinutils" != xunavailable && test "x$coin_has_coinutils" != xskipping &&
   test "x$coin_has_osi" != xunavailable && test "x$coin_has_osi" != skipping &&
   test "x$coin_has_glpk" = xtrue ; then
  gamslinks_build_glpk=1
else
  gamslinks_build_glpk=0
fi
AC_SUBST(gamslinks_build_glpk)
AM_CONDITIONAL([GAMSLINKS_BUILD_GLPK],[test $gamslinks_build_glpk = 1])

if test "x$coin_has_coinutils" != xunavailable && test "x$coin_has_coinutils" != xskipping &&
   test "x$coin_has_osi" != xunavailable && test "x$coin_has_osi" != skipping &&
   test $coin_build_clp = 1 && test $coin_build_cbc = 1 ; then
  gamslinks_build_cbc=1
else
  gamslinks_build_cbc=0
fi
AC_SUBST(gamslinks_build_cbc)
AM_CONDITIONAL([GAMSLINKS_BUILD_CBC],[test $gamslinks_build_cbc = 1])


if test $gamslinks_build_cbc = 1 && test $gamslinks_build_ipopt = 1 &&
   test "x$coin_has_bonmin" != xunavailable && test "x$coin_has_bonmin" != xskipping ; then
  gamslinks_build_bonmin=1
else
  gamslinks_build_bonmin=0
fi
AC_SUBST(gamslinks_build_bonmin)
AM_CONDITIONAL([GAMSLINKS_BUILD_BONMIN],[test $gamslinks_build_bonmin = 1])

if test "x$coin_has_coinutils" != xunavailable && test "x$coin_has_coinutils" != xskipping &&
   test "x$coin_has_osi" != xunavailable && test "x$coin_has_osi" != skipping &&
   ( test $coin_has_glpk = true || test $coin_has_cpx = true || test $coin_has_msk = true || test $coin_has_grb = true || test $coin_has_xpr = true ) ; then
  gamslinks_build_osi=1
else
  gamslinks_build_osi=0
fi
AC_SUBST(gamslinks_build_osi)
AM_CONDITIONAL([GAMSLINKS_BUILD_OSI],[test $gamslinks_build_osi = 1])

if test "x$coin_has_coinutils" != xunavailable && test "x$coin_has_coinutils" != xskipping &&
   test "x$coin_has_os" != xunavailable && test "x$coin_has_os" != xskipping &&
   test "x$coin_has_cppad" != xunavailable && test "x$coin_has_cppad" != xskipping ; then
  gamslinks_build_os=1
else
  gamslinks_build_os=0
fi
AC_SUBST(gamslinks_build_os)
AM_CONDITIONAL([GAMSLINKS_BUILD_OS], [test $gamslinks_build_os = 1])

if test "x$coin_has_coinutils" != xunavailable && test "x$coin_has_coinutils" != xskipping &&
   test "x$coin_has_osi" != xunavailable && test "x$coin_has_osi" != skipping &&
   test "x$coin_has_scip" = xtrue && (test $use_thirdpartyscip = 0 || (test $coin_build_clp = 1 && test $gamslinks_build_ipopt = 1) || (test $coin_has_soplex = true && test $gamslinks_build_ipopt = 1)) ; then
  gamslinks_build_scip=1
else
  gamslinks_build_scip=0
fi
AC_SUBST(gamslinks_build_scip)
AM_CONDITIONAL([GAMSLINKS_BUILD_SCIP],[test $gamslinks_build_scip = 1])

if test $gamslinks_build_cbc = 1 && test $gamslinks_build_ipopt = 1 && test $gamslinks_build_bonmin = 1 &&
   test "x$coin_has_couenne" != xunavailable && test "x$coin_has_couenne" != xskipping ; then
  gamslinks_build_couenne=1
else
  gamslinks_build_couenne=0
fi
AC_SUBST(gamslinks_build_couenne)
AM_CONDITIONAL([GAMSLINKS_BUILD_COUENNE],[test $gamslinks_build_couenne = 1])

##############################################################################
#                VPATH links for GAMS installation files                     #
##############################################################################

# In case this is a VPATH configuration we need to make sure that the
# GAMS solver installation files are available in the VPATH directory.

AC_COIN_VPATH_LINK(src/SolverInterfaces/gmsbm_us.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmsbm_nt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmsbm_un.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmsbm_nt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmscbcus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmscbcnt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmscbcun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmscbcnt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/optmycbc.def)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmscouus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmscount.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmscouun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmscount.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmsip_us.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmsip_nt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmsip_un.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmsip_nt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmsos_us.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmsos_nt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmsos_un.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmsos_nt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/optmyos.def)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszcpus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszcpnt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszcpun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszcpnt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszglus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszglnt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszglun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszglnt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/optmyglpk.def)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszguus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszgunt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszguun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszgunt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszmkus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszmknt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszmkun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszmknt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszspus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszspnt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszspun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszspnt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszxpus.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmszxpnt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszxpun.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmszxpnt.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmssc_us.run)
AC_COIN_VPATH_LINK(src/SolverInterfaces/gmssc_nt.cmd)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmssc_un.dat)
AC_COIN_VPATH_LINK(src/SolverInterfaces/install/gmssc_nt.dat)
AC_COIN_VPATH_LINK(examples/adbench/gmsad_us.run)
AC_COIN_VPATH_LINK(examples/adbench/gmsad_nt.cmd)
AC_COIN_VPATH_LINK(examples/adbench/install/gmsad_un.dat)
AC_COIN_VPATH_LINK(examples/adbench/install/gmsad_nt.dat)

############
# END PART #
############

AC_LANG_POP(C++)

AC_CONFIG_FILES([Makefile
                 gams_addlibs.txt
                 src/Makefile
                 src/GamsUtils/Makefile
                 src/SolverInterfaces/Makefile
                 test/Makefile
                 examples/adbench/Makefile
                 examples/osil2gms/Makefile
                 doc/optdoc/Makefile
                ])
AC_CONFIG_FILES([test/run_quality], [chmod +x test/run_quality])
AC_CONFIG_FILES([test/run_gmstest], [chmod +x test/run_gmstest])

AC_CONFIG_HEADER([inc/config_gamslinks.h])

AC_COIN_FINALIZE
