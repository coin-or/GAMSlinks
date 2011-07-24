# Copyright (C) 2006-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
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
under the Eclipse Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/SolverInterfaces/GamsSolver.hpp)

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
#coin_skip_warn_cflags=yes

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT(GAMSlinks)

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(GAMSlinks)

# We need egrep and ln -s somewhere
#AC_PROG_EGREP
#AC_PROG_LN_S

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Find out how to call Fortran from C and determine Fortran runtime libraries
#AC_COIN_F77_WRAPPERS

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

#############################################################################
#                      COIN-OR and other components                         #
#############################################################################

# check which solvers are available
AC_COIN_CHECK_PACKAGE(Osi,     [osi],        [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OsiCpx,  [osi-cplex],  [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OsiGlpk, [osi-glpk],   [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OsiGrb,  [osi-gurobi], [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OsiMsk,  [osi-mosek],  [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OsiSpx,  [osi-soplex], [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OsiXpr,  [osi-xpress], [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(Cbc,     [cbc],        [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(Ipopt,   [ipopt],      [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(Bonmin,  [bonmin],     [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(Couenne, [couenne],    [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(OS,      [os],         [GamsSolverLib])
AC_COIN_CHECK_PACKAGE(SCIP,    [coinscip],   [GamsSolverLib])

############################################################################
#                   Stuff that we need for C++ programs                    #
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

AC_CHECK_FUNCS([stricmp strcasecmp],  [break])
AC_CHECK_FUNCS([snprintf _snprintf],  [break])
AC_CHECK_FUNCS([vsnprintf _vsnprintf],[break])

#############################
# parallel BLAS recognition #
#############################

if test $coin_has_ipopt = yes ; then
  LIBS_save="$LIBS"
  LIBS="$IPOPT_LIBS $LIBS"
  AC_CHECK_FUNC([goto_set_num_threads],
    AC_DEFINE([HAVE_GOTO_SETNUMTHREADS],[1],[Define to 1 if the Blas library has goto_set_num_threads])
  )
  AC_CHECK_FUNC([MKL_Domain_Set_Num_Threads],
    AC_DEFINE([HAVE_MKL_SETNUMTHREADS],[1],[Define to 1 if the Blas library has mkl_set_num_threads])
  )
  LIBS="$LIBS_save"
fi

AC_LANG_POP(C++)

###############
# GAMS system #
###############

AC_COIN_HAVE_GAMS

####################
# Windos or *nix ? #
####################

case $build in
  *-cygwin* | *-mingw* )
    coin_is_windows=yes
  ;;
  *)
    coin_is_windows=no
  ;;
esac
AM_CONDITIONAL(COIN_IS_WINDOWS, test $coin_is_windows = yes)  

############
# END PART #
############

#                 examples/adbench/Makefile
#                 examples/osil2gms/Makefile
#                 doc/optdoc/Makefile
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/GamsUtils/Makefile
                 src/SolverInterfaces/Makefile
                 test/Makefile
                ])
AC_CONFIG_FILES([test/run_quality], [chmod +x test/run_quality])
AC_CONFIG_FILES([test/run_gmstest], [chmod +x test/run_gmstest])

AC_CONFIG_HEADER([src/config.h src/config_gamslinks.h])

AC_COIN_FINALIZE
