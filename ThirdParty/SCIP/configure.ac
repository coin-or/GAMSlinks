# Copyright (C) 2008, 2009 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([ThirdPartyScip],[0.0],[http://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([
Copyright 2008 Stefan Vigerske and others.
All Rights Reserved.
This file is part of the open source package COIN-OR which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(Makefile.am)

# Where should everything be installed by default?  Here, we want it
# to be installed directly in 'bin', 'lib', 'include' subdirectories
# of the directory where configure is run.  The default would be
# /usr/local.
AC_PREFIX_DEFAULT([`pwd`])

AC_COIN_PROJECTDIR_INIT

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the system type
AC_CANONICAL_BUILD

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(SCIP)

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Initialize automake
AC_COIN_INIT_AUTO_TOOLS

#TODO: check for Clp and CoinUtils and Ipopt
#AC_COIN_MAIN_SUBDIRS(../../CoinUtils)
#AC_COIN_MAIN_SUBDIRS(../../Clp)
#AC_COIN_HAS_PROJECT(../../CoinUtils)
#AC_COIN_HAS_PROJECT(../../Clp)

AC_COIN_HAS_SCIP

rm -f $MAKEOKFILE
if test x"$use_thirdpartyscip" = xbuild ; then
  touch $MAKEOKFILE
  
  EXTRAFLAGS="-DWITH_IPOPT"
  
  AC_CHECK_MEMBER(struct sigaction.sa_handler,,
    [EXTRAFLAGS="$EXTRAFLAGS -DNO_SIGACTION"],
    [#include <signal.h>])
    
  AC_CHECK_FUNC(rand_r,,
    [EXTRAFLAGS="$EXTRAFLAGS -DNO_RAND_R"])
    
  AC_CHECK_FUNC(strtok_r,,
    [EXTRAFLAGS="$EXTRAFLAGS -DNO_STRTOK_R"])

  AC_CHECK_FUNC(strerror_r,,
    [EXTRAFLAGS="$EXTRAFLAGS -DNO_STRERROR_R"])
    
  AC_CHECK_HEADER([fenv.h],[EXTRAFLAGS="$EXTRAFLAGS -DROUNDING_FE"],
    [AC_CHECK_HEADER([float.h],[EXTRAFLAGS="$EXTRAFLAGS -DROUNDING_FP"])])
 
  CFLAGS="$CFLAGS $EXTRAFLAGS"
  CXXFLAGS="$CXXFLAGS $EXTRAFLAGS"
  
  AC_MSG_CHECKING([SCIP version])
  scipver=`awk '/define SCIP_VERSION/ {print $3}' ${SCIPINCDIR}/scip/def.h`
  scipsubver=`awk '/define SCIP_SUBVERSION/ {print $3}' ${SCIPINCDIR}/scip/def.h`
  AC_MSG_RESULT([${scipver}.${scipsubver}])
  if (test x$scipver != x120) ; then
    AC_MSG_ERROR(["Incorrect SCIP version. This build system is made for SCIP 120.x"])
  fi
  AM_CONDITIONAL(SCIPDEVVERSION, [test x$scipsubver != x0])
  
#  if test x$minorver = x999 ; then
#    CFLAGS=`echo $CFLAGS | sed -e 's/-pedantic-errors//'`
#    CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-pedantic-errors//'`
#  else
  # SCIP uses long long that result in errors if -pedantic-errors is used
  # thus, we disable this warning
  case "$CC" in
    *gcc* )
    	CFLAGS="$CFLAGS -Wno-long-long -Wno-overlength-strings"
    	CXXFLAGS="$CXXFLAGS -Wno-long-long -Wno-overlength-strings"
    ;;
  esac
#  fi
  
else
  if test x$coin_has_scip = xtrue ; then
    AC_MSG_NOTICE([The SCIP sources don't need to be compiled])
  else
    AC_MSG_WARN([SCIP source is available but will not be built. No valid external library is detected. Are you sure this is what you want?])
  fi
  MAKEOKFILE=
fi
AC_SUBST(MAKEOKFILE)

#for the local Makefile.am
SCIPDIR=scip
AC_SUBST(SCIPDIR)

AC_CONFIG_FILES([Makefile])

#dummy header file to avoid many extra compiler flags
AC_CONFIG_HEADER([config_scip.h])
 
AC_COIN_FINALIZE
