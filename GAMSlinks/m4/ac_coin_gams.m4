# Copyright (C) GAMS Development and others 2008-2011 
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
## $Id$
#
# Author: Stefan Vigerske

#######################################################################
#                        COIN_HAVE_GAMS                               #
#######################################################################

# This macro will define the following variables:
#
# GAMS_PATH            path of GAMS system if available
# COIN_HAS_GAMSSYSTEM  AM_CONDITIONAL that tells whether gams is available
# GAMS_VERSION         major version number of GAMS system

AC_DEFUN([AC_COIN_HAVE_GAMS], [

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
AM_CONDITIONAL([COIN_HAS_GAMSSYSTEM], [test "$GAMS_PATH" != UNAVAILABLE])
AC_SUBST(GAMS_PATH)

GAMS_VERSION_DEFAULT="23.7" 
if test "$GAMS_PATH" = UNAVAILABLE; then
  AC_MSG_NOTICE([no GAMS system found, tests will not work, assuming build for GAMS version $GAMS_VERSION])
  GAMS_VERSION="$GAMS_VERSION_DEFAULT" 
else
  AC_MSG_CHECKING([for gams version number])
  GAMS_VERSION=""
  [GAMS_VERSION=`grep "Installation of" ${GAMS_PATH}/gamsinst.log | head -1 | sed -e 's/.* \([0-9][0-9]*\.[0-9][0-9]*\)\.[0-9][0-9]* .*/\1/'`]
  if test "x$GAMS_VERSION" != x ; then
    AC_MSG_RESULT([$GAMS_VERSION])
  else
    GAMS_VERSION="$GAMS_VERSION_DEFAULT" 
    AC_MSG_RESULT([unknown, assuming version $GAMS_VERSION])
  fi
fi
AC_SUBST(GAMS_VERSION)

])
