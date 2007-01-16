# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([GAMSlinks],[0.1],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([
Copyright 2006 GAMS Development 2006
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/GamsIO/GamsModel.hpp)

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
AC_COIN_DEBUG_COMPILE(GAMSlinks)

# We need egrep and ln -s somewhere
#AC_PROG_EGREP
#AC_PROG_LN_S

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

case "$CXX" in
  cl* | */cl*)
    CXXFLAGS=`echo $CXXFLAGS | sed -e 's/-MTd//g' -e 's/-MT//g'`
  ;;
esac


# Get the C++ runtime libraries in case we want to link with a C or
# Fortran compiler
# AC_COIN_CXXLIBS

# Get the name of the Fortran compiler and appropriate compiler options
#AC_COIN_PROG_F77

# Find out how to call Fortran from C and determine Fortran runtime libraries
#AC_COIN_F77_WRAPPERS

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

# A Fix for the AIX compiler
#AM_CONDITIONAL([AIX_FIX_COMPILER_BUG], test "x$AIX_FIX_COMPILER_BUG" != x)

#############################################################################
#                              COIN components                              #
#############################################################################

AC_COIN_HAS_PROJECT(GAMSlinks)
AC_COIN_HAS_PROJECT(Cgl)
AC_COIN_HAS_PROJECT(CoinUtils)
AC_COIN_HAS_PROJECT(Osi)
AC_COIN_HAS_PROJECT(Clp)
AC_COIN_HAS_PROJECT(Cbc)

############################################################################
############################################################################
#                   Stuff that we need for C++ programs                    #
############################################################################
############################################################################

AC_LANG_PUSH(C++)

################
# Header Files #
################


AC_COIN_CHECK_CXX_CHEADER(assert)
AC_COIN_CHECK_CXX_CHEADER(stdio)
AC_COIN_CHECK_CXX_CHEADER(stdlib)
AC_COIN_CHECK_CXX_CHEADER(string)
#AC_COIN_CHECK_CXX_CHEADER(math)
#AC_COIN_CHECK_CXX_CHEADER(ctype)
#AC_COIN_CHECK_CXX_CHEADER(stdarg)
#AC_COIN_CHECK_CXX_CHEADER(float)
#AC_COIN_CHECK_CXX_CHEADER(time)

##########################################################################

# Additional libraries and packages

# We need to restore the value of LIBS before those test, otherwise libtool
# will hick-up
LIBS_without_ADDLIBS="$LIBS"

################
# GLPK LIBRARY #
################

#coin_glpkobjdir=../ThirdParty/GLPK
#coin_glpksrcdir=$srcdir/$coin_glpkobjdir

#AC_ARG_WITH([glpk],
#            AC_HELP_STRING([--with-glpk-incdir],
#                           [specify the directory with the header files for library Glpk]),
#            [glpk_incdir=$withval],
#            [glpk_incdir=])
#AC_ARG_WITH([glpk],
#            AC_HELP_STRING([--with-glpk-lib],
#                           [specify the flags to link with the library Glpk]),
#            [glpk_lib=$withval],
#            [glpk_lib=])

#if test x$glpk_incdir = x; 
#then
#	AC_CHECK_FILE([$coin_glpkobjdir/glpk-4.9/Makefile],[
#     glpk_incdir=$coin_glpksrcdir/glpk-4.9/include
#	   glpk_lib=$coin_glpkobjdir/glpk-4.9/src/libglpk.a
#	   GLPKINCDIR=$glpk_incdir
#	   GLPKLIB=$glpk_lib
#	   coin_has_glpk=true],
#	  [coin_has_glpk=false])
#else
#	AC_MSG_CHECKING([whether user-given GLPK library works])
# AC_LANG_PUSH(C)
#	LIBS="$glpk_lib $LIBS"
#	CPPFLAGS_save=$CPPFLAGS
#	CPPFLAGS="-I$glpk_incdir $CPPFLAGS"
#	AC_TRY_LINK([#include "glpk.h"],[glp_lpx_simplex()],
#              [AC_MSG_RESULT(yes)
#               CPPFLAGS=$CPPFLAGS_save ],
#			        [AC_MSG_RESULT(no)
#               AC_MSG_ERROR([GLPK with include at $glpk_incdir and link flags $glpk_lib does not work])])
#  AC_DEFINE(COIN_HAS_GLPK,[1],[Define to 1 if the GLPK package is used])
#  GLPKINCDIR=$glpk_incdir
#  GLPKLIB=$glpk_lib
#  coin_has_glpk=true
# AC_LANG_POP(C)
#fi

#AC_SUBST(GLPKINCDIR)
#AC_SUBST(GLPKLIB)
#AM_CONDITIONAL(COIN_HAS_GLPK, test $coin_has_glpk = true)


AC_LANG_PUSH(C)
ADDLIBS_save=$ADDLIBS
AC_COIN_HAS_USER_LIBRARY([Glpk],[GLPK],[glpk.h],[glp_lpx_simplex])
# not every package will make use of this library
ADDLIBS=$ADDLIBS_save
AC_LANG_POP(C)

####################
# GAMS I/O LIBRARY #
####################

# This macro checks for a library containing the GAMS I/O library.
# It defines the makefile conditional and preprocessor macro
# COIN_HAS_GAMSIO, if GAMS I/O is available.

GAMSIO_CODE=unsupportedarchitecture
case $build in
  x86_64-*-linux-*)
      GAMSIO_CODE=LEI
      gamsio_system=Linux
    ;;
  i?86-*-linux-*)
      GAMSIO_CODE=LX3
      gamsio_system=Linux
    ;;
  *-cygwin* | *-mingw*)
      GAMSIO_CODE=VIS
      gamsio_system=Windows
    ;;
  *-darwin*)
      GAMSIO_CODE=DAR
      gamsio_system=Darwin
    ;;
esac

coin_gamsioobjdir=../ThirdParty/GAMSIO/$GAMSIO_CODE
coin_gamsiosrcdir=$abs_source_dir/$coin_gamsioobjdir

#if user provided GAMS I/O libraries, use this one, otherwise check ThirdParty/COINIO directory
AC_ARG_WITH([gamsio],
            AC_HELP_STRING([--with-gamsio],
                           [specify directory of GAMS I/O libraries and header files]),
            [use_gamsio=$withval],
	         [AC_CHECK_FILE([$coin_gamsiosrcdir/iolib.h],[use_gamsio=$coin_gamsiosrcdir],[use_gamsio=])]  )
						
# Check if GAMS I/O libs work
if test x"$use_gamsio" != x; then

  #but first, we check also for a GAMS system
	AC_ARG_WITH([gamssystem],
            AC_HELP_STRING([--with-gamssystem],
                           [specify directory of GAMS distribution]),
            [use_gamssys=$withval],
						[AC_PATH_PROG(gamspath, [gams],,,)
						 use_gamssys=${gamspath/%gams/}
             if test x"$use_gamssys" != x; then
               AC_MSG_NOTICE([found GAMS system at $use_gamssys])
						 elif test x"$gamsio_system" != xLinux; then
						   AC_MSG_NOTICE([no GAMS system found; tests will not work!])
						 else
               AC_MSG_ERROR([no GAMS system found; GAMS system required for linking])
						 fi
						])

	GAMS_PATH=$use_gamssys
	if test x"$use_gamssys" != x; then
		AC_MSG_CHECKING([whether GAMS I/O libraries at $use_gamsio and $use_gamssys work])
		if test x"$GAMSIO_CODE" = xLX3; then
		  GAMSIO_LIBS="$use_gamsio/iolib.a $use_gamsio/nliolib.a $use_gamsio/clicelib.a $use_gamsio/gclib.a $use_gamsio/libg2d.a -ldl -L$use_gamssys -lifcore -limf"
		elif test x"$GAMSIO_CODE" = xLEI; then
		  GAMSIO_LIBS="$use_gamsio/iolib.a $use_gamsio/nliolib.a $use_gamsio/clicelib.a $use_gamsio/gclib.a $use_gamsio/libg2d.a -ldl -L$use_gamssys -lifcore -limf -lirc"
		elif test x"$gamsio_system" = xWindows; then
		  GAMSIO_LIBS="$use_gamsio/iolib.lib $use_gamsio/nliolib.lib $use_gamsio/clicelib.lib $use_gamsio/gclib.lib $use_gamsio/g2d.lib"
		elif test x"$GAMSIO_CODE" = xDAR; then
		  GAMSIO_LIBS="$use_gamsio/iolib.a $use_gamsio/nliolib.a $use_gamsio/clicelib.a $use_gamsio/gclib.a $use_gamsio/libg2d.a -ldl -lSystemStubs"
		fi
		GAMSIO_CPPFLAGS="-I$use_gamsio"
		LIBS="$GAMSIO_LIBS $LIBS"
		CPPFLAGS_save=$CPPFLAGS
		CPPFLAGS="$GAMSIO_CPPFLAGS $CPPFLAGS"
		AC_TRY_LINK([
#include <cstdio>
#include "iolib.h"], [gfinit()],
											[AC_MSG_RESULT([yes])
											 LIBS=$LIBS_without_ADDLIBS
											 CPPFLAGS=$CPPFLAGS_save],
											[AC_MSG_RESULT([no])
											AC_MSG_ERROR([GAMS I/O library $use_gamsio does not work])])
  else
		AC_MSG_ERROR([no GAMS I/O libraries found])
 		use_gamsio=
  fi
else
	AC_MSG_ERROR([no GAMS I/O libraries found])
fi

AM_CONDITIONAL([COIN_HAS_GAMSIO],[test x"$use_gamsio" != x])

if test x"$use_gamsio" = x; then
  coin_has_gamsio=no
else
  coin_has_gamsio=yes
  AC_DEFINE([COIN_HAS_GAMSIO],[1],
            [If defined, the GAMS I/O Library is available.])
fi
AC_SUBST(coin_has_gamsio)
AC_SUBST(GAMS_PATH)
AC_SUBST(GAMSIO_CPPFLAGS)
AC_SUBST(GAMSIO_LIBS)
AC_SUBST(GAMSIO_CODE)
AM_CONDITIONAL([GAMSIO_IS_UNIX],[test x"$gamsio_system" != xWindows])
AM_CONDITIONAL([GAMSIO_IS_WINDOWS],[test x"$gamsio_system" = xWindows])

##############################################################################
#                VPATH links for GAMS installation files                     #
##############################################################################

# In case this is a VPATH configuration we need to make sure that the
# GAMS solver installation files are available in the VPATH directory.

AC_COIN_VPATH_LINK(src/Cbc/gmscc_us.run)
AC_COIN_VPATH_LINK(src/Cbc/gmscc_nt.cmd)
AC_COIN_VPATH_LINK(src/Cbc/install/gmscc_un.dat)
AC_COIN_VPATH_LINK(src/Cbc/install/gmscc_nt.dat)
AC_COIN_VPATH_LINK(src/Glpk/gmscg_us.run)
AC_COIN_VPATH_LINK(src/Glpk/gmscg_nt.cmd)
AC_COIN_VPATH_LINK(src/Glpk/install/gmscg_un.dat)
AC_COIN_VPATH_LINK(src/Glpk/install/gmscg_nt.dat)

############
# END PART #
############

AC_LANG_POP(C++)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/GamsIO/Makefile
                 src/Cbc/Makefile
                 src/Glpk/Makefile
                 test/Makefile test/run_tests
                ])

AC_CONFIG_HEADER([inc/config_gamslinks.h])

AC_COIN_FINALIZE
