# Copyright (C) 2010 GAMS Development and others 
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: ac_coin_scip.m4 348 2008-02-23 14:44:19Z stefan $
#
# Author: Stefan Vigerske

###########################################################################
#                             COIN_HAS_SOPLEX                             #
###########################################################################
#
# This macro checks for the SoPlex package. 
#
# The macro first uses COIN_HAS_USER_LIBRARY to see if the user has specified
# a preexisting library (this allows the use of any scip version, if the user
# is fussy). The macro then checks for ThirdParty/SoPlex.
#
# This macro will define the following variables:
#   coin_has_soplex true or false
#   SOPLEXLIB		    link flags for SCIP if library is user supplied
#   SOPLEXINCDIR		location of SCIP include files
#   SOPLEXOBJDIR    location of SCIP library (libcoinscip.la) if built in ThirdParty/SCIP
#   COIN_HAS_SOPLEX	Preprocessor symbol, defined to 1
#   COIN_HAS_SOPLEX	Automake conditional
#   COIN_BUILD_SOPLEX	Automake conditional, defined only if SCIP is to be built in ThirdParty/SCIP

AC_DEFUN([AC_COIN_HAS_SOPLEX],[
if test "$PACKAGE_NAME" = ThirdPartySoplex; then
  coin_soplexobjdir=.
else
  if test -d ../ThirdParty/SoPlex ; then
    coin_soplexobjdir=../ThirdParty/SoPlex
  elif test -d ../SoPlex ; then
    coin_soplexobjdir=../SoPlex
  else
    coin_soplexobjdir=../../ThirdParty/SoPlex
  fi
fi
coin_soplexsrcdir=$abs_source_dir/$coin_soplexobjdir

# Check for the requested component. If the user specified an external SOPLEX
# library don't force a ThirdParty build, let the error propagate.
# this macro defined SOPLEXLIB, SOPLEXINCDIR.
AC_COIN_HAS_USER_LIBRARY([Soplex],[SOPLEX],[soplex.h])

# If the user has supplied an external library (coin_has_soplex=true), use it.
# Otherwise, we consider a build in ThirdParty/SoPlex.

if test x"$coin_has_soplex" = xfalse && test x"$SOPLEXLIB" = x ; then
  MAKEOKFILE=.MakeOk
  # Check if the SoPlex's ThirdParty project has been configured
  if test "$PACKAGE_NAME" != ThirdPartySoplex; then
    if test -r $coin_soplexobjdir/.MakeOk; then
      use_thirdpartysoplex=build
    else
      use_thirdpartysoplex=no
    fi
  else
    use_thirdpartysoplex=build
  fi
else
  #we use user provided SoPlex
  use_thirdpartysoplex=no
fi

# If we're building, set the library and include directory variables, create a
# preprocessor symbol, define a variable that says we're using SoPlex, and
# another to indicate a link check is a bad idea (hard to do before the library
# exists).

if test x"$use_thirdpartysoplex" = xbuild ; then
  SOPLEXINCDIR="$coin_soplexsrcdir/soplex/src"
  SOPLEXOBJDIR=`cd $coin_soplexobjdir; pwd`
  AC_SUBST(SOPLEXINCDIR)
  AC_SUBST(SOPLEXOBJDIR)
  AC_DEFINE(COIN_HAS_SOPLEX,[1],[Define to 1 if $1 package is available])
  coin_has_soplex=true
  soplex_libcheck=no
  AC_MSG_NOTICE([Using SoPlex in ThirdParty/SoPlex])
fi

# Define the necessary automake conditionals.

AM_CONDITIONAL(COIN_HAS_SOPLEX, [test x"$coin_has_soplex" = xtrue])
AM_CONDITIONAL(COIN_BUILD_SOPLEX, [test x"$use_thirdpartysoplex" = xbuild])

]) # AC_COIN_HAS_SCIP
