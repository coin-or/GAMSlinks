# Copyright (C) 2010 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id: configure.ac 928 2010-09-14 12:18:54Z stefan $

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([ThirdPartySoplex],[0.0],[http://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([
Copyright 2010 Stefan Vigerske and others.
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
AC_COIN_DEBUG_COMPILE(SOPLEX)

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Initialize automake
AC_COIN_INIT_AUTO_TOOLS

AC_COIN_HAS_SOPLEX

rm -f $MAKEOKFILE
if test x"$use_thirdpartysoplex" = xbuild ; then
  touch $MAKEOKFILE
  
  CXXFLAGS="$CXXFLAGS -DNO_ADDITIONAL_CHECKS"
  
#  AC_MSG_CHECKING([SOPLEX version])
#  scipver=`awk '/define SCIP_VERSION/ {print $3}' ${SCIPINCDIR}/scip/def.h`
#  scipsubver=`awk '/define SCIP_SUBVERSION/ {print $3}' ${SCIPINCDIR}/scip/def.h`
#  AC_MSG_RESULT([${scipver}.${scipsubver}])
#  if test x$scipver == x200 ; then
#     AM_CONDITIONAL(SCIPDEVVERSION, [test 1 = 1])
#  elif test x$scipver != x120 ; then
#    AC_MSG_ERROR(["Incorrect SCIP version. This build system is made for SCIP 120.x"])
#  fi
  
 
else
  if test x$coin_has_soplex = xtrue ; then
    AC_MSG_NOTICE([The SoPlex sources don't need to be compiled])
  else
    AC_MSG_WARN([SoPlex source is available but will not be built. No valid external library is detected. Are you sure this is what you want?])
  fi
  MAKEOKFILE=
fi
AC_SUBST(MAKEOKFILE)

#for the local Makefile.am
SOPLEXDIR=soplex
AC_SUBST(SOPLEXDIR)

AC_CONFIG_FILES([Makefile])

#dummy header file to avoid many extra compiler flags
AC_CONFIG_HEADER([config_soplex.h])
 
AC_COIN_FINALIZE
