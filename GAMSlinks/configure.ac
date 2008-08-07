# Copyright (C) 2006-2008 GAMS Development and others
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

AC_INIT([GAMSlinks],[0.3],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) 2006-2008 GAMS Development and others 
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

case "$CXX" in
  cl* | */cl*)
    CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-MTd//g' -e 's/-MT//g'`
  ;;
esac

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
#                              COIN components                              #
#############################################################################

AC_COIN_HAS_PROJECT(GAMSlinks)
AC_COIN_HAS_PROJECT(Cgl)
AC_COIN_HAS_PROJECT(CoinUtils)
AC_COIN_HAS_PROJECT(Osi)
AC_COIN_HAS_PROJECT(Clp)
AC_COIN_HAS_PROJECT(Vol)
AC_COIN_HAS_PROJECT(DyLP)
AC_COIN_HAS_PROJECT(SYMPHONY)
AC_COIN_HAS_PROJECT(Cbc)
AC_COIN_HAS_PROJECT(Ipopt)
AC_COIN_HAS_PROJECT(Bonmin)
AC_COIN_HAS_PROJECT(cppad)
AC_COIN_HAS_PROJECT(OS)

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

###########
# va_copy #
###########

AC_CHECK_DECL([va_copy],[AC_DEFINE([HAVE_VA_COPY],[1],
                                   [Define to 1 if va_copy is avaliable])],,[
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

AC_CHECK_FUNCS([stricmp strcasecmp],[break],,)
AC_CHECK_FUNCS([snprintf _snprintf],[break],,)
AC_CHECK_FUNCS([vsnprintf _vsnprintf],[break],,)

##########################################################################

# Additional libraries and packages

# We need to restore the value of LIBS before those test, otherwise libtool
# will hick-up
LIBS_without_ADDLIBS="$LIBS"

################
# GLPK LIBRARY #
################

AC_COIN_HAS_GLPK(Glpk)

################
# SCIP LIBRARY #
################

AC_COIN_HAS_SCIP

# including scip.h gives some warnings that result in errors if -pedantic-errors is used
# thus, we disable these warnings if SCIP is used
if test "x$coin_has_scip" = xtrue ; then
  case "$CC" in
    *gcc* )
      CXXFLAGS="$CXXFLAGS -Wno-unknown-pragmas -Wno-long-long"
    ;;
  esac
fi


########################
# OS Solver Interfaces #
########################

#if enable-os-solver is set, then we link the Gams/OS binary with all available coin solvers that have an OS interface and allow a "local" solve 
AC_MSG_CHECKING([whether to include solvers into GamsOS binary])
AC_ARG_ENABLE(os-solver,
  AC_HELP_STRING([--enable-os-solver],[whether to include solver interfaces to available solvers into the GamsOS binary (default=no)]),
  [os_solver=yes
   AC_DEFINE([COIN_OS_SOLVER],[1],[If defined, the Gams/OS interface should allow local solves])],
  [os_solver=no] )
  
AM_CONDITIONAL(COIN_OS_SOLVER, [test x"$os_solver" = xyes])
AC_MSG_RESULT([$os_solver])

################
# CBC parallel #
################

AC_ARG_ENABLE([cbc-parallel],,
  AC_DEFINE([CBC_THREAD],[1],[Define to 1 if the SMP version of CBC is compiled])
)

######################
# CPLEX for Gams/OSI #
######################

AC_COIN_HAS_USER_LIBRARY([Cplex],[CPX],[cplex.h],[CPXgetstat])

####################################
# GAMS system and GAMS I/O LIBRARY #
####################################

AC_COIN_HAVE_GAMS
if test $coin_has_gamsio = no; then
  AC_MSG_ERROR([need GAMS I/O libraries to build GAMSlinks])
fi
AC_COIN_USE_GAMS

#################################################################################
# Check which OSI solvers we build and which interfaces we can (and want) build #
#################################################################################

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

if test "x$coin_has_dylp" != xunavailable && test "x$coin_has_dylp" != xskipping ; then
  coin_build_dylp=1
else
  coin_build_dylp=0
fi
AC_SUBST(coin_build_dylp)

if test "x$coin_has_symphony" != xunavailable && test "x$coin_has_symphony" != xskipping ; then
  coin_build_symphony=1
else
  coin_build_symphony=0
fi
AC_SUBST(coin_build_symphony)

if test "x$coin_has_vol" != xunavailable && test "x$coin_has_vol" != skipping ; then
  coin_build_vol=1
else
  coin_build_vol=0
fi
AC_SUBST(coin_build_vol)

if test "x$coin_has_ipopt" != "xunavailable" && test "x$coin_has_ipopt" != "xskipping"; then
  gamslinks_build_ipopt=1
else
  gamslinks_build_ipopt=0
fi
AC_SUBST(gamslinks_build_ipopt)
AM_CONDITIONAL([GAMSLINKS_BUILD_IPOPT], [test $gamslinks_build_ipopt = 1])

if test "x$coin_has_coinutils" != xunavailable && test "x$coin_has_coinutils" != xskipping &&
   test "x$coin_has_osi" != xunavailable && test "x$coin_has_osi" != skipping ; then
  gamslinks_build_gamsio=1
else
  gamslinks_build_gamsio=0
fi
AM_CONDITIONAL([GAMSLINKS_BUILD_GAMSIO],[test $gamslinks_build_gamsio = 1])

if test $gamslinks_build_gamsio = 1 && test "x$coin_has_glpk" = xtrue ; then
  gamslinks_build_glpk=1
else
  gamslinks_build_glpk=0
fi
AC_SUBST(gamslinks_build_glpk)
AM_CONDITIONAL([GAMSLINKS_BUILD_GLPK],[test $gamslinks_build_glpk = 1])

if test $gamslinks_build_gamsio = 1 && test $coin_build_clp = 1 && test $coin_build_cbc = 1 ; then
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

if test $gamslinks_build_gamsio = 1 &&
   ( test $coin_build_dylp = 1 || test $coin_build_symphony = 1 || test $coin_build_vol = 1 || test $coin_has_cpx = true ) ; then
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
AM_CONDITIONAL([GAMSLINKS_BUILD_OS], [test $gamslinks_build_os = 1])

if test $gamslinks_build_gamsio = 1 && test "x$coin_has_scip" = xtrue && test $coin_build_clp = 1 ; then
  gamslinks_build_scip=1
else
  gamslinks_build_scip=0
fi
AC_SUBST(gamslinks_build_scip)
AM_CONDITIONAL([GAMSLINKS_BUILD_SCIP],[test $gamslinks_build_scip = 1])

##############################################################################
#                VPATH links for GAMS installation files                     #
##############################################################################

# In case this is a VPATH configuration we need to make sure that the
# GAMS solver installation files are available in the VPATH directory.

AC_COIN_VPATH_LINK(src/Cbc/gmscc_us.run)
AC_COIN_VPATH_LINK(src/Cbc/gmscc_nt.cmd)
AC_COIN_VPATH_LINK(src/Cbc/install/gmscc_un.dat)
AC_COIN_VPATH_LINK(src/Cbc/install/gmscc_nt.dat)
AC_COIN_VPATH_LINK(src/Cbc/optcbc.def)
AC_COIN_VPATH_LINK(src/Glpk/gmscg_us.run)
AC_COIN_VPATH_LINK(src/Glpk/gmscg_nt.cmd)
AC_COIN_VPATH_LINK(src/Glpk/install/gmscg_un.dat)
AC_COIN_VPATH_LINK(src/Glpk/install/gmscg_nt.dat)
AC_COIN_VPATH_LINK(src/Glpk/optglpk.def)
AC_COIN_VPATH_LINK(src/Osi/gmsosius.run)
AC_COIN_VPATH_LINK(src/Osi/gmsosint.cmd)
AC_COIN_VPATH_LINK(src/Osi/install/gmsosiun.dat)
AC_COIN_VPATH_LINK(src/Osi/install/gmsosint.dat)
AC_COIN_VPATH_LINK(src/Osi/optosi.def)
AC_COIN_VPATH_LINK(src/Ipopt/gmsip_us.run)
AC_COIN_VPATH_LINK(src/Ipopt/gmsip_nt.cmd)
AC_COIN_VPATH_LINK(src/Ipopt/install/gmsip_un.dat)
AC_COIN_VPATH_LINK(src/Ipopt/install/gmsip_nt.dat)
AC_COIN_VPATH_LINK(src/Bonmin/gmsbm_us.run)
AC_COIN_VPATH_LINK(src/Bonmin/gmsbm_nt.cmd)
AC_COIN_VPATH_LINK(src/Bonmin/install/gmsbm_un.dat)
AC_COIN_VPATH_LINK(src/Bonmin/install/gmsbm_nt.dat)
AC_COIN_VPATH_LINK(src/OS/gmsos_us.run)
AC_COIN_VPATH_LINK(src/OS/gmsos_nt.cmd)
AC_COIN_VPATH_LINK(src/OS/install/gmsos_un.dat)
AC_COIN_VPATH_LINK(src/OS/install/gmsos_nt.dat)
AC_COIN_VPATH_LINK(src/OS/optos.def)
AC_COIN_VPATH_LINK(src/SCIP/gmssc_us.run)
AC_COIN_VPATH_LINK(src/SCIP/gmssc_nt.cmd)
AC_COIN_VPATH_LINK(src/SCIP/install/gmssc_un.dat)
AC_COIN_VPATH_LINK(src/SCIP/install/gmssc_nt.dat)
AC_COIN_VPATH_LINK(examples/adbench/gmsad_us.run)
AC_COIN_VPATH_LINK(examples/adbench/gmsad_nt.cmd)
AC_COIN_VPATH_LINK(examples/adbench/install/gmsad_un.dat)
AC_COIN_VPATH_LINK(examples/adbench/install/gmsad_nt.dat)

############
# END PART #
############

AC_LANG_POP(C++)

AC_CONFIG_FILES([Makefile
                 gamsio_addlibs.txt smagio_addlibs.txt
                 src/Makefile
                 src/GamsIO/Makefile
                 src/Cbc/Makefile
                 src/Glpk/Makefile
                 src/Osi/Makefile
                 src/Ipopt/Makefile
                 src/Bonmin/Makefile
                 src/OS/Makefile
                 src/SCIP/Makefile
                 test/Makefile test/run_tests
                 examples/adbench/Makefile
                ])

AC_CONFIG_HEADER([inc/config_gamslinks.h])

AC_COIN_FINALIZE
