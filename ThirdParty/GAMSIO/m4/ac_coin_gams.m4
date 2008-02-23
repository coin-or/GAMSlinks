# Copyright (C) 2008 GAMS Development and others 
# All Rights Reserved.
# This file is distributed under the Common Public License.
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
#
# GAMSIO_CODE          the GAMS code for the architecture we build on (LNX,SIG,...)
# GAMSIO_CIA           the GAMS CIA code for the architecture we use (CIA_LEX,CIA_WIN,...) 
# GAMSIO_IS_DARWIN     AM_CONDITIONAL that tells whether we build on a Darwin system
# GAMSIO_IS_UNIX       AM_CONDITIONAL that tells whether we build on a Unix-like system (= !Windows)
# GAMSIO_IS_WINDOWS    AM_CONDITIONAL that tells whether we build on a Windows system
#
# coin_has_gamsio      shell variable that tells whether the GAMS I/O library is there (value is yes or no) 

AC_DEFUN([AC_COIN_HAVE_GAMS], [

#check whether we have a GAMS system available

GAMS_PATH=UNAVAILABLE
AC_ARG_WITH([gamssystem],
  AC_HELP_STRING([--with-gamssystem],[specify directory of GAMS distribution]),
  AC_CHECK_FILE([$withval/gams],[GAMS_PATH=$withval],[GAMS_PATH=UNAVAILABLE]),
  [AC_PATH_PROG(gamspath, [gams], UNAVAILABLE,,)
   GAMS_PATH=${gamspath/%gams/}
  ])
AM_CONDITIONAL([COIN_HAS_GAMSSYSTEM], [test $GAMS_PATH != UNAVAILABLE])
AC_SUBST(GAMS_PATH)
if test $GAMS_PATH = UNAVAILABLE; then
  AC_MSG_NOTICE([no GAMS system found, tests will not work])
fi


#check on which architecture we are running

GAMSIO_CODE=unsupportedarchitecture

AC_ARG_WITH([gamsio-code],
  AC_HELP_STRING([--with-gamsio-code],[specify the build architecture to identify which build of the GAMS I/O library to use]),
  [GAMSIO_CODE=$withval],
  [case $build in
      x86_64-*-linux-*)
        GAMSIO_CODE=LEI
      ;;
      i?86-*-linux-*)
        GAMSIO_CODE=LX3
      ;;
      *-cygwin* | *-mingw*)
        GAMSIO_CODE=VIS
      ;;
      i?86-*-darwin*)
        GAMSIO_CODE=DII
      ;;
      i?86-*-solaris*)
        GAMSIO_CODE=SIG
      ;;
      *)
        AC_MSG_ERROR([Build type $build not supported by GAMS I/O libraries.])
  esac]
)

case $GAMSIO_CODE in
  LEI)
      gamsio_system=Linux
      GAMSIO_CIA=CIA_LEX
    ;;
  LX3)
      gamsio_system=Linux
      GAMSIO_CIA=CIA_LNX
    ;;
  VIS)
      gamsio_system=Windows
      GAMSIO_CIA=CIA_WIN
    ;;
  DII)
      gamsio_system=Darwin
      GAMSIO_CIA=CIA_DAR
    ;;
  SIG)
      gamsio_system=Solaris
      GAMSIO_CIA=CIA_SIS
    ;;
  *)
      AC_MSG_ERROR([GAMS I/O libraries with system code $GAMSIO_CODE not supported.])
esac

AC_SUBST(GAMSIO_CODE)
AC_SUBST(GAMSIO_CIA)
AM_CONDITIONAL([GAMSIO_IS_DARWIN], [test x"$gamsio_system" = xDarwin])
AM_CONDITIONAL([GAMSIO_IS_UNIX],   [test x"$gamsio_system" != xWindows])
AM_CONDITIONAL([GAMSIO_IS_WINDOWS],[test x"$gamsio_system" = xWindows])

# check whether we have the GAMS I/O libraries that we need

if test "$PACKAGE_NAME" = ThirdPartyGAMSIO; then
  gamsio_objdir=.
else
  gamsio_objdir=../ThirdParty/GAMSIO
fi
gamsio_srcdir=`$CYGPATH_W $abs_source_dir/$gamsio_objdir/$GAMSIO_CODE | sed -e sX\\\\\\\\X/Xg`
gamsio_objdir=`cd $gamsio_objdir; pwd`
gamsio_objdir=`$CYGPATH_W $gamsio_objdir`
AC_CHECK_FILE([$gamsio_srcdir/iolib.h],[coin_has_gamsio=yes],[
  coin_has_gamsio=no
	AC_MSG_WARN([no GAMS I/O libraries in ThirdParty/GAMSIO/$GAMSIO_CODE found. You can download them by calling $abs_source_dir/../ThirdParty/GAMSIO/get.$GAMSIO_CODE.])
])

])  #end of COIN_HAVE_GAMS


######################################################################
#                        COIN_USE_GAMS                               #
######################################################################

# This macro assumes that COIN_HAVE_GAMS was run before.
# It defines the following variables:
#
# COIN_HAS_GAMSIO      AM_CONDITIONAL that tells whether the GAMS I/O library is there and working
# COIN_HAS_GAMSIO      AC_SUBST that tells whether the GAMS I/O library is there and working (value is yes or no)
# COIN_HAS_GAMSIO      AC_DEFINE that is defined if the GAMS I/O library is there and working

# GAMSIO_CPPFLAGS      C/C++ flags for using the GAMS I/O header files (include path)
# GAMSIO_ADDLIBS       additional libraries needed to link with the GAMS I/O libraries
# GAMSIO_LIBS          linker flags when "old-style" (ciolib) GAMS I/O libraries are used
# SMAG_LIBS            linker flags when "new-style" (smag) GAMS I/O libraries are used
# GAMSIO_OBJDIR        directory where compiled code from GAMS I/O library can be found (usually ThirdParty/GAMSIO) 
#
# FORTRAN_NAMEMANGLING_CFLAG   compiler flags to define name mangling scheme as used in the GAMS I/O libs 

AC_DEFUN([AC_COIN_USE_GAMS], [

# check whether the Intel Fortran Compiler libraries are available
# these are required to link with a non-Intel compiler Intel-compiler-compiled GAMSIO libraries

IFORT_LIBS=
if test x"$gamsio_system" = xLinux || test x"$gamsio_system" = xDarwin; then
	case "$CXX" in
		icpc* | */icpc*)
			;;
		*)
		  AC_ARG_WITH([ifort-libdir],
        AC_HELP_STRING([--with-ifort-libdir],[specify directory of Intel Fortran libraries]),
        AC_CHECK_FILE([$withval/libifcore.so],[IFORT_LIBS="-L$withval"],
          AC_CHECK_FILE([$withval/libifcore.a],[IFORT_LIBS="-L$withval"],
            AC_MSG_WARN([Path for Intel Fortran Libraries given by --with-ifort-libdir but no libifcore.so or libifcore.a found. GAMS I/O libraries might fail to link.])        
        )),
        [if test x"$GAMS_PATH" != xUNAVAILABLE; then
          AC_CHECK_FILE([$GAMS_PATH/libifcore.so],[IFORT_LIBS="-L$GAMS_PATH"],)
         else
          AC_MSG_WARN([Path for Intel Fortran Libraries not given by --with-ifort-libdir and no GAMS system available. GAMS I/O libraries might fail to link.])
         fi
        ])
      IFORT_LIBS="$IFORT_LIBS -lifcore -limf -lirc"
	esac
fi

# setting up linker flags

case $GAMSIO_CODE in
  LX3)
      GAMSIO_LIBS="$gamsio_srcdir/iolib.a $gamsio_srcdir/nliolib.a $gamsio_srcdir/clicelib.a $gamsio_srcdir/gclib.a"
      SMAG_LIBS="$gamsio_srcdir/clicelib.a $gamsio_srcdir/libsmag.a $gamsio_srcdir/gclib.a $gamsio_srcdir/libg2d.a"
      GAMSIO_ADDLIBS="-ldl $IFORT_LIBS"
    ;;
  LEI)
      GAMSIO_LIBS="$gamsio_srcdir/iolib.a $gamsio_srcdir/nliolib.a $gamsio_srcdir/clicelib.a $gamsio_srcdir/gclib.a"
      SMAG_LIBS="$gamsio_srcdir/clicelib.a $gamsio_srcdir/libsmag.a $gamsio_srcdir/gclib.a $gamsio_srcdir/libg2d.a"
      GAMSIO_ADDLIBS="-ldl $IFORT_LIBS"
    ;;
  VIS)
      GAMSIO_LIBS="$gamsio_srcdir/iolib.lib $gamsio_srcdir/nliolib.lib $gamsio_srcdir/clicelib.lib $gamsio_srcdir/gclib.lib"
      SMAG_LIBS="$gamsio_srcdir/clicelib.lib $gamsio_srcdir/smag.lib $gamsio_srcdir/gclib.lib $gamsio_srcdir/g2d.lib"
      GAMSIO_ADDLIBS=""
    ;;
  DII)
      GAMSIO_LIBS="$gamsio_srcdir/iolib.a $gamsio_srcdir/nliolib.a $gamsio_srcdir/clicelib.a $gamsio_srcdir/gclib.a"
      SMAG_LIBS="$gamsio_srcdir/clicelib.a $gamsio_srcdir/libsmag.a $gamsio_srcdir/gclib.a $gamsio_srcdir/libg2d.a"
      GAMSIO_ADDLIBS="-ldl -lSystemStubs $IFORT_LIBS"
    ;;
  SIG)
      GAMSIO_LIBS="$gamsio_srcdir/iolib.a $gamsio_srcdir/nliolib.a $gamsio_srcdir/clicelib.a $gamsio_srcdir/gclib.a"
      SMAG_LIBS="$gamsio_srcdir/clicelib.a $gamsio_srcdir/libsmag.a $gamsio_srcdir/gclib.a $gamsio_srcdir/libg2d.a"
      GAMSIO_ADDLIBS=""
    ;;
esac

# check whether we can use the iolib

AC_MSG_CHECKING([whether GAMS I/O libraries at $gamsio_srcdir work])
GAMSIO_CPPFLAGS="-I$gamsio_srcdir"
LIBS="$GAMSIO_LIBS $GAMSIO_ADDLIBS $LIBS"
CPPFLAGS_save=$CPPFLAGS
CPPFLAGS="$GAMSIO_CPPFLAGS $CPPFLAGS"
AC_TRY_LINK([
#include <cstdio>
#include "iolib.h"], [gfinit()],
  [AC_MSG_RESULT([yes])
   LIBS=$LIBS_without_ADDLIBS
   CPPFLAGS=$CPPFLAGS_save],
  [AC_MSG_RESULT([no])
   AC_MSG_ERROR([GAMS I/O library at $gamsio_src does not work])])

# write out compiler, linker, Makefile... flags

AM_CONDITIONAL([COIN_HAS_GAMSIO],[test "$coin_has_gamsio" = yes])
AC_DEFINE([COIN_HAS_GAMSIO],[1],[If defined, the GAMS I/O Library is available.])
COIN_HAS_GAMSIO=yes
AC_SUBST(COIN_HAS_GAMSIO)
AC_SUBST(GAMSIO_CPPFLAGS)
AC_SUBST(GAMSIO_ADDLIBS)
AC_SUBST(GAMSIO_LIBS)
AC_SUBST(SMAG_LIBS)
GAMSIO_OBJDIR=$gamsio_objdir
AC_SUBST(GAMSIO_OBJDIR)

# define the fortran name mangling scheme as used in the GAMS I/O libs
# this is needed to use the g2dexports.h in SmagExtra.c 

case $ac_cv_f77_mangling in
  "lower case, no underscore"*)
    FORTRAN_NAMEMANGLING_CFLAG="-DFNAME_LCASE_NODECOR"
  ;;
  "lower case, underscore"*)
    FORTRAN_NAMEMANGLING_CFLAG="-DFNAME_LCASE_DECOR"
  ;;
  "upper case, no underscore"*)
    FORTRAN_NAMEMANGLING_CFLAG="-DFNAME_UCASE_NODECOR"
  ;;
  "upper case, underscore"*)
    FORTRAN_NAMEMANGLING_CFLAG="-DFNAME_UCASE_DECOR"
  ;;
  *)
    AC_MSG_ERROR("cannot handle name mangling scheme '$ac_cv_f77_mangling'. Compilation of SmagExtra.c will probably fail.")
  ;;
esac
AC_SUBST(FORTRAN_NAMEMANGLING_CFLAG)

]) # end of COIN_USE_GAMS
