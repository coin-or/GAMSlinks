# Copyright (C) 2008 GAMS Development and others 
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

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
# This macro will define the following variables:
#   coin_has_scip	true or false
#   SCIPLIB		    link flags for SCIP if library is user supplied
#   SCIPINCDIR		location of SCIP include files
#   SCIPOBJDIR    location of SCIP library (libcoinscip.la) if built in ThirdParty/SCIP
#   COIN_HAS_SCIP	Preprocessor symbol, defined to 1
#   COIN_HAS_SCIP	Automake conditional
#   COIN_BUILD_SCIP	Automake conditional, defined only if SCIP is to be built in ThirdParty/SCIP

AC_DEFUN([AC_COIN_HAS_SCIP],[
if test "$PACKAGE_NAME" = ThirdPartyScip; then
  coin_scipobjdir=.
else
  coin_scipobjdir=../ThirdParty/SCIP
fi
coin_scipsrcdir=$abs_source_dir/$coin_scipobjdir

# Check for the requested component. If the user specified an external SCIP
# library don't force a ThirdParty build, let the error propagate.
# this macro defined SCIPLIB, SCIPINCDIR, 
AC_COIN_HAS_USER_LIBRARY([Scip],[SCIP],[scip/scip.h],[SCIPcreate _SCIPcreate])

# If the user has supplied an external library (coin_has_scip=true), use it.
# Otherwise, we consider a build in ThirdParty/SCIP.

if test x"$coin_has_scip" = xfalse && test x"$SCIPLIB" = x ; then
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
else
  #we use user provided SCIP
  use_thirdpartyscip=no
fi

# If we're building, set the library and include directory variables, create a
# preprocessor symbol, define a variable that says we're using SCIP, and
# another to indicate a link check is a bad idea (hard to do before the library
# exists).

if test x"$use_thirdpartyscip" = xbuild ; then
  SCIPINCDIR="$coin_scipsrcdir/scip/src"
  SCIPOBJDIR=`cd $coin_scipobjdir; pwd`
  AC_SUBST(SCIPINCDIR)
  AC_SUBST(SCIPOBJDIR)
  AC_DEFINE(COIN_HAS_SCIP,[1],[Define to 1 if $1 package is available])
  coin_has_scip=true
  scip_libcheck=no
  AC_MSG_NOTICE([Using SCIP in ThirdParty/SCIP])
fi

# Define the necessary automake conditionals.

AM_CONDITIONAL(COIN_HAS_SCIP, [test x"coin_has_scip" = xtrue])
AM_CONDITIONAL(COIN_BUILD_SCIP, [test x"$use_thirdpartyscip" = xbuild])

]) # AC_COIN_HAS_SCIP
