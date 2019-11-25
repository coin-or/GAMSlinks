# Copyright (C) 2006-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([GAMSlinks],[master],[https://github.com/coin-or/GAMSlinks])

AC_COPYRIGHT([Copyright (C) 2006-2011 GAMS Development and others 
All Rights Reserved.
This file is part of the open source package GAMSlinks which is distributed
under the Eclipse Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/GAMSlinksConfig.h)

AC_COIN_INITIALIZE(GAMSlinks)

#############################################################################
#                         Standard build tools stuff                        #
#############################################################################

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Initialize libtool
AC_COIN_PROG_LIBTOOL

# Doxygen
AC_COIN_DOXYGEN

#############################################################################
#                      COIN-OR and other components                         #
#############################################################################

# check which solvers are available
AC_COIN_CHK_PKG(Osi,     [GamsSolverLib], [osi])
AC_COIN_CHK_PKG(OsiCpx,  [GamsSolverLib], [osi-cplex])
AC_COIN_CHK_PKG(OsiGrb,  [GamsSolverLib], [osi-gurobi])
AC_COIN_CHK_PKG(OsiMsk,  [GamsSolverLib], [osi-mosek])
AC_COIN_CHK_PKG(OsiXpr,  [GamsSolverLib], [osi-xpress])
AC_COIN_CHK_PKG(Cbc,     [GamsSolverLib], [cbc])
AC_COIN_CHK_PKG(Ipopt,   [GamsSolverLib], [ipopt])
AC_COIN_CHK_PKG(Bonmin,  [GamsSolverLib], [bonmin])
AC_COIN_CHK_PKG(Couenne, [GamsSolverLib], [couenne])
AC_COIN_CHK_PKG(SoPlex,  [GamsSolverLib], [coinsoplex])
AC_COIN_CHK_PKG(SCIP,    [GamsSolverLib], [coinscip])

############################################################################
#                          Check for system specifics                      #
############################################################################

AC_LANG_PUSH(C)

# check if intptr_t is available, which can be used to avoid errors when compiling the GAMS library loader codes
GAMSLIBCFLAGS=
AC_CHECK_TYPE([intptr_t],[GAMSLIBCFLAGS=-DHAVE_INTPTR_T])  #FIXME
AC_SUBST(GAMSLIBCFLAGS)

# check if va_copy is available, to be used in GamsJournal
AC_CHECK_DECL([va_copy],
  [AC_DEFINE([HAVE_VA_COPY],[1],[Define to 1 if va_copy is available])],,
  [
#include "stdarg.h"
  ])

# check for snprintf variants
AC_CHECK_FUNCS([snprintf _snprintf],[break])
AC_CHECK_FUNCS([vsnprintf _vsnprintf],[break])

# check for strtok variants
AC_CHECK_FUNCS([strtok_r strtok_s strtok],[break])

# if we have Ipopt, check if we have GotoBlas or MKL Blas functions to set number of threads
if test $coin_has_ipopt = yes ; then
  AC_COIN_TRY_LINK([goto_set_num_threads],[$ipopt_lflags],[$ipopt_pcfiles],
    AC_DEFINE([HAVE_GOTO_SETNUMTHREADS],[1],[Define to 1 if Ipopt has goto_set_num_threads])
  )
  AC_COIN_TRY_LINK([omp_set_num_threads],[$ipopt_lflags],[$ipopt_pcfiles],
    AC_DEFINE([HAVE_OMP_SETNUMTHREADS],[1],[Define to 1 if Ipopt has omp_set_num_threads])
  )
fi

AC_LANG_POP(C)

###############
# GAMS system #
###############

GAMS_PATH=UNAVAILABLE
AC_ARG_WITH([gams],
  AC_HELP_STRING([--with-gams],[specify directory of GAMS distribution]),
  [if test "$withval" != no ; then
     AC_CHECK_FILE([$withval/gams],[GAMS_PATH=$withval],[GAMS_PATH=UNAVAILABLE])
   fi
  ],
  [AC_PATH_PROG(gamspath, [gams], UNAVAILABLE,,)
   GAMS_PATH="${gamspath/%gams/}"
  ])
AC_SUBST(GAMS_PATH)

if test "$GAMS_PATH" = UNAVAILABLE; then
  AC_MSG_ERROR([no GAMS system found])
fi

####################
# Windos or *nix ? #
####################

case $build in
  *-cygwin* | *-mingw* | *-msys* )
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

AC_COIN_FINALIZE_FLAGS([GamsSolverLib])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/GamsUtils/Makefile
                 src/SolverInterfaces/Makefile
                 src/SolverInterfaces/gamsinst.sh
                 test/Makefile
                 doc/optdoc/Makefile
                ])
AC_CONFIG_FILES([test/run_quality], [chmod +x test/run_quality])
AC_CONFIG_FILES([test/run_gmstest], [chmod +x test/run_gmstest])

AC_CONFIG_HEADER([src/config.h])

AC_COIN_FINALIZE
