# Copyright (C) 2008 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id: configure.ac 130 2007-07-17 23:07:33Z andreasw $

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([ThirdPartyScip],[0.0],[http://projects.coin-or.org/BuildTools/newticket])

AC_COPYRIGHT([
Copyright 2007 Stefan Vigerske and others.
All Rights Reserved.
This file is part of the open source package Coin which is distributed
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
#coin_skip_warn_cxxflags=yes

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(SCIP)

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# Initialize automake
AC_COIN_INIT_AUTO_TOOLS

#AC_COIN_MAIN_SUBDIRS(../../CoinUtils)
#AC_COIN_MAIN_SUBDIRS(../../Clp)
#AC_COIN_HAS_PROJECT(../../CoinUtils)
#AC_COIN_HAS_PROJECT(../../Clp)

###########################################################################
#                             COIN_HAS_SCIP                               #
###########################################################################
#
# This macro checks for the SCIP package. 
#
# The macro first uses COIN_HAS_USER_LIBRARY to see if the user has specified
# a preexisting library (this allows the use of any scip version, if the user
# is fussy). The macro then checks for ThirdParty/SCIP.
#
# This macro will define the following variables for Glpk:
#   coin_has_scip	true or false
#   SCIPLIB		link flags for SCIP (if user supplied)
#   SCIPINCDIR		location of SCIP include files
#   COIN_HAS_SCIP	Preprocessor symbol, defined to 1
#   COIN_HAS_SCIP	Automake conditional
#   COIN_BUILD_SCIP	Automake conditional, defined only if SCIP is to be
#			built in ThirdParty/SCIP

AC_DEFUN([AC_COIN_HAS_SCIP],
[
if test "$PACKAGE_NAME" = ThirdPartyScip; then
  coin_scipobjdir=.
else
  coin_scipobjdir=../ThirdParty/SCIP
fi
coin_scipsrcdir=$abs_source_dir/$coin_scipobjdir

use_thirdpartyscip=no

# Check for the requested component. If the user specified an external SCIP
# library don't force a ThirdParty build, let the error propagate.

AC_COIN_HAS_USER_LIBRARY([Scip],[SCIP],[scip.h],[_SCIPcreate SCIPcreate])
if test x"$coin_has_scip" = xfalse && test x"$SCIPLIB" = x ; then
   use_thirdpartyscip=try
fi

# If the user has supplied an external library, use it.
# Otherwise, consider a build in ThirdParty/SCIP.

if test x"$use_thirdpartyscip" = xtry ; then
  MAKEOKFILE=.MakeOk
  # Check if the SCIP's ThirdParty project has been configured
  if test "$PACKAGE_NAME" != ThirdPartyScip; then
    if test -r $coin_scipobjdir/.MakeOk; then
      use_thirdpartyscip=build
    else
      use_thirdpartyscip=no
    fi
  else
    use_thirdpartyscip=build
  fi

# If we're building, set the library and include directory variables, create a
# preprocessor symbol, define a variable that says we're using SCIP, and
# another to indicate a link check is a bad idea (hard to do before the library
# exists).

  if test x"$use_thirdpartyscip" = xbuild ; then
    SCIPINCDIR="$coin_scipsrcdir/scip/src"
    AC_SUBST(SCIPINCDIR)
    AC_DEFINE(COIN_HAS_SCIP,[1],[Define to 1 if $1 package is available])
    coin_has_scip=true
    scip_libcheck=no
    AC_MSG_NOTICE([Using SCIP in ThirdParty/SCIP])
  fi
fi

# Define the necessary automake conditionals.

AM_CONDITIONAL(COIN_HAS_SCIP, [test x"coin_has_scip" = xtrue])
AM_CONDITIONAL(COIN_BUILD_SCIP, [test x"$use_thirdpartyscip" = xbuild])

]) # AC_COIN_HAS_SCIP


## Check if SCIP is already provided by user
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

SCIPDIR=scip
AC_SUBST(SCIPDIR)

AC_CONFIG_FILES([Makefile])

#dummy config file just to get -DHAVE_CONFIG_H
AC_CONFIG_HEADER([config_scip.h])

AC_COIN_FINALIZE
