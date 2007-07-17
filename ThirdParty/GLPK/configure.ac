# Copyright (C) 2007 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([ThirdPartyGlpk],[0.0],[http://projects.coin-or.org/BuildTools/newticket])

AC_COPYRIGHT([
Copyright 2007 Stefan Vigerske and others.
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(glpk/src/glplpx01.c)

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
AC_COIN_DEBUG_COMPILE(GLPK)

# Get the name of the C compiler
AC_COIN_PROG_CC

# Initialize automake
AC_COIN_INIT_AUTO_TOOLS

###########################################################################
#                            COIN_HAS_GLPK                                #
###########################################################################

# This macro checks for a library containing the GLPK library.  It
# checks if the user has provided an argument for the GLPK library,
# and if not, it checks whether the GLPK ThirdParty/GLPK directory has
# been configured.  It adds to ADDLIBS any flags required to link with
# an externally provided GLPK.  It defines the makefile conditional
# and preprocessor macro COIN_HAS_GLPK, if GLPK is available, and it
# defines the makefile conditional COIN_BUILD_GLPK, if GLPK is
# compiled within COIN.

AC_DEFUN([AC_COIN_HAS_GLPK],
[
if test "$PACKAGE_NAME" = ThirdPartyGlpk; then
  coin_glpkobjdir=../GLPK
else
  coin_glpkobjdir=../ThirdParty/GLPK
fi
coin_glpksrcdir=$abs_source_dir/$coin_glpkobjdir

MAKEOKFILE=.MakeOk

AC_ARG_WITH([glpk-dir],
            AC_HELP_STRING([--with-glpk-dir],
                           [specify directory where GLPK is installed]),
            [use_glpk="$withval"], [use_glpk=no])

if test "$use_glpk" != "no"; then
  if test -d $use_glpk; then :; else
    AC_MSG_ERROR([User provided GLPK directory $use_glpk does not exist.])
  fi
	glpk_dir=`cd $use_glpk; pwd`

  case "$CC" in
    cl* | */cl* | CL* | */CL*)
         libe=lib ;;
      *) libe=a ;;
  esac

  # Check if headers are there
  AC_COIN_CHECK_FILE([$glpk_dir/include/glpk.h],
                     [],
                     [AC_MSG_ERROR([I cannot find headers for GLPK])])
                
  GLPK_INCFLAGS="-I\`\$(CYGPATH_W) $glpk_dir/include\`"
  GLPK_LIB="$glpk_dir/lib/libglpk.$libe"
  # Check if GLPK actually works
  save_LIBS="$LIBS"
  LIBS="$GLPK_LIB $LIBS"
  AC_LANG_PUSH(C)
  AC_TRY_LINK([void glp_lpx_simplex();],[glp_lpx_simplex()],[],
   [AC_TRY_LINK([void _glp_lpx_simplex();],[_glp_lpx_simplex()],[],
     AC_MSG_ERROR([User provided GLPK library doesn't work]))
   ])
  AC_LANG_POP(C)
  LIBS="$save_LIBS"

else
  use_glpk=BUILD

  # Check if the GLPK' ThirdParty project has been configured
  if test "$PACKAGE_NAME" != ThirdPartyGlpk; then
    if test -r $coin_glpkobjdir/.MakeOk; then
      use_glpk=BUILD
    else
      use_glpk=
    fi
  fi

  GLPK_INCFLAGS="-I\`\$(CYGPATH_W) $coin_glpksrcdir/glpk/include\`"
  GLPK_LIB="../../../ThirdParty/GLPK/libcoinglpk.la"
fi

AC_SUBST(GLPK_INCFLAGS)
AC_SUBST(GLPK_LIB)

AM_CONDITIONAL([COIN_HAS_GLPK],[test x"$use_glpk" != x])
#AM_CONDITIONAL([COIN_BUILD_GLPK],[test "$use_glpk" = BUILD])

AC_MSG_CHECKING([whether GLPK is available])

if test x"$use_glpk" = x || test "$use_glpk" = no; then
  coin_has_glpk=no
else
  coin_has_glpk=yes
  AC_DEFINE([COIN_HAS_GLPK],[1],
            [If defined, the GLPK Library is available.])
fi
AC_MSG_RESULT([$coin_has_glpk])

]) # AC_COIN_HAS_GLPK

## Check if GLPK is already provided by user
AC_COIN_HAS_GLPK

rm -f $MAKEOKFILE
if test $coin_has_glpk = yes && test "$use_glpk" != BUILD; then
  AC_MSG_NOTICE([The GLPK sources don't need to be compiled])
  MAKEOKFILE=
else
  touch $MAKEOKFILE
fi
AC_SUBST(MAKEOKFILE)
AM_CONDITIONAL(SKIPMAKE, test -z "$MAKEOKFILE")

GLPKDIR=glpk
AC_SUBST(GLPKDIR)

AC_CONFIG_FILES([Makefile])

AC_COIN_FINALIZE
