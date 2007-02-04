# Copyright (C) GAMS Development 2006
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

AC_INIT([GAMSlinks],[0.2],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) GAMS Development 2006
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(get.LX3)

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
AC_COIN_DEBUG_COMPILE(GAMSIO)

# We need egrep and ln -s somewhere
#AC_PROG_EGREP
#AC_PROG_LN_S

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

#optcc does not compile with pedantic warnings because dlsym returns a
#pointer to an object instead of a pointer to a function
CFLAGS=`echo $CFLAGS | sed -e 's/-pedantic-errors//g'`
AC_MSG_NOTICE([changed C compiler options to $CFLAGS])

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

# A Fix for the AIX compiler
#AM_CONDITIONAL([AIX_FIX_COMPILER_BUG], test "x$AIX_FIX_COMPILER_BUG" != x)

##########################################################################
#                              GAMSIO stuff                              #
##########################################################################


GAMSIO_CODE=unsupportedarchitecture
case $build in
  x86_64-*-linux-*)
      GAMSIO_CODE=LEI
      CIA_CODE=CIA_LEX
    ;;
  i?86-*-linux-*)
      GAMSIO_CODE=LX3
      CIA_CODE=CIA_LEX
    ;;
  *-cygwin* | *-mingw*)
      GAMSIO_CODE=VIS
      CIA_CODE=CIA_VIS
    ;;
  *-darwin*)
      GAMSIO_CODE=DAR
      CIA_CODE=CIA_DAR
    ;;
esac

AC_SUBST(GAMSIO_CODE)
AC_SUBST(CIA_CODE)

############
# END PART #
############

AC_CONFIG_FILES([Makefile])

#but actually not used
AC_CONFIG_HEADER([config_gamsio.h])

AC_COIN_FINALIZE
