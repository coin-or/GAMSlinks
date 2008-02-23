# Copyright (C) 2008 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([ThirdPartyScip],[0.0],[http://projects.coin-or.org/BuildTools/newticket])

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

coin_skip_warn_cflags=yes
coin_skip_warn_cxxflags=yes

# Check if user wants to produce debugging code
# AC_COIN_DEBUG_COMPILE(SCIP)

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Initialize automake
AC_COIN_INIT_AUTO_TOOLS

#TODO: check for Clp and CoinUtils
#AC_COIN_MAIN_SUBDIRS(../../CoinUtils)
#AC_COIN_MAIN_SUBDIRS(../../Clp)
#AC_COIN_HAS_PROJECT(../../CoinUtils)
#AC_COIN_HAS_PROJECT(../../Clp)

AC_COIN_HAS_SCIP

rm -f $MAKEOKFILE
if test x"$use_thirdpartyscip" = xbuild ; then
  touch $MAKEOKFILE
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

AC_COIN_FINALIZE
