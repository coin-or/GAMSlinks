# Copyright (C) 2006-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_INIT([GAMSlinks],[master],[https://github.com/coin-or/GAMSlinks])

AC_COPYRIGHT([Copyright (C) 2006-2011 GAMS Development and others 
All Rights Reserved.
This file is part of the open source package GAMSlinks which is distributed
under the Eclipse Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/utils/GamsLinksConfig.h)

AC_COIN_INITIALIZE

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
AC_COIN_CHK_PKG(OsiCpx,  [GamsOsi],     [osi-cplex])
AC_COIN_CHK_PKG(OsiGrb,  [GamsOsi],     [osi-gurobi])
AC_COIN_CHK_PKG(OsiMsk,  [GamsOsi],     [osi-mosek])
AC_COIN_CHK_PKG(OsiXpr,  [GamsOsi],     [osi-xpress])
if test $coin_has_osicpx$coin_has_osigrb$coin_has_osimsk$coin_has_osixpr != "nononono" ; then
  AC_COIN_CHK_PKG(Osi,   [GamsOsi],     [osi])
else
  AM_CONDITIONAL(COIN_HAS_OSI,false)
fi
AC_COIN_CHK_PKG(Cbc,     [GamsCbc],     [cbc])
AC_COIN_CHK_PKG(Ipopt,   [GamsIpopt],   [ipopt])
AC_COIN_CHK_PKG(Bonmin,  [GamsBonmin],  [bonmin])
AC_COIN_CHK_PKG(Couenne, [GamsCouenne], [couenne])

AC_COIN_CHK_LIB(SoPlex,[GamsSoplex],[-lsoplex],[],[],[])
AC_COIN_CHK_LIB(SCIP,[GamsScip],[-lscipsolver],[],[],[SCIPcreate])
AC_COIN_CHK_LIB(GCG,[GamsScip],[-lgcg],[],[],[GCGisMaster])
AC_COIN_CHK_LIB(Cplex,[GamsOsi GamsBonmin GamsCouenne GamsScip GamsCbc],[-lcplex -lpthread -lm -ldl],[],[],[CPXgetstat])
AC_COIN_CHK_LIB(Mosek,[GamsOsi GamsScip],[-lmosek64 -lpthread],[],[],[MSK_makeenv])
AC_COIN_CHK_LIB(Xpress,[GamsOsi],[-lxprs -lxprl],[],[],[XPRSinit])
AC_COIN_CHK_LIB(Gurobi,[GamsOsi],[-lgurobi -lpthread -lm],[],[],[GRBloadenv])

############################################################################
#                          Check for system specifics                      #
############################################################################

AC_LANG_PUSH(C)

# check if intptr_t is available, which can be used to avoid errors when compiling the GAMS library loader codes
GAMSLIBCFLAGS=
AC_CHECK_TYPE([intptr_t],[GAMSLIBCFLAGS=-DHAVE_INTPTR_T])
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

AC_CHECK_LIB(dl,[dlopen],[
  GAMSBONMIN_LFLAGS="$GAMSBONMIN_LFLAGS -ldl"
  GAMSCBC_LFLAGS="$GAMSCBC_LFLAGS -ldl"
  GAMSCOUENNE_LFLAGS="$GAMSCOUENNE_LFLAGS -ldl"
  GAMSIPOPT_LFLAGS="$GAMSIPOPT_LFLAGS -ldl"
  GAMSOSI_LFLAGS="$GAMSOSI_LFLAGS -ldl"
  GAMSSCIP_LFLAGS="$GAMSSCIP_LFLAGS -ldl"
  GAMSSOPLEX_LFLAGS="$GAMSSOPLEX_LFLAGS -ldl"
],[])

AC_LANG_POP(C)

# name of gensolver script and exe
case $build in
  *-mingw* | *-cygwin* | *-msys* )
    GMSGENSCRIPT=gmsgennt.cmd
    GMSGENEXE=gmsgennx.exe
    ;;
  *)
    GMSGENSCRIPT=gmsgenus.run
    GMSGENEXE=gmsgenux.out
    ;;
esac
AC_SUBST(GMSGENSCRIPT)
AC_SUBST(GMSGENEXE)

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

############
# END PART #
############

AC_COIN_FINALIZE_FLAGS([GamsOsi GamsCbc GamsIpopt GamsBonmin GamsCouenne GamsSoplex GamsScip])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/bonmin/Makefile
                 src/cbc/Makefile
                 src/couenne/Makefile
                 src/ipopt/Makefile
                 src/osi/Makefile
                 src/scip/Makefile
                 src/soplex/Makefile
                 test/Makefile
                 src/gamsconfig.yaml
                 doc/Doxyfile
                ])
AC_CONFIG_FILES([test/run_quality], [chmod +x test/run_quality])
AC_CONFIG_FILES([test/run_gmstest], [chmod +x test/run_gmstest])

AC_CONFIG_HEADER([src/utils/config.h])

AC_COIN_FINALIZE
