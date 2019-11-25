# Copyright (C) GAMS Development and others 2008-2011 
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

#######################################################################
#                        COIN_HAVE_GAMS                               #
#######################################################################

# This macro will define the following variables:
#
# GAMS_PATH            path of GAMS system if available
# COIN_HAS_GAMSSYSTEM  AM_CONDITIONAL that tells whether gams is available

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

if test "$GAMS_PATH" = UNAVAILABLE; then
  AC_MSG_WARN([no GAMS system found, build and tests will not work])
fi

])
