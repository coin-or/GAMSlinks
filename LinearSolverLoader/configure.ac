# Copyright (C) 2008
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

AC_INIT([LinearSolverLoader],[0.0],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) 2008
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/HSLLoader.h)

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

#turn off warnings and pedantic mode since it seem to make trouble with dlsym in LibraryHandler.c:
#gcc 4.2.1 error: ISO C forbids conversion of object pointer to function pointer type
coin_skip_warn_cflags=yes

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(LinearSolverLoader)

# We need egrep and ln -s somewhere
AC_PROG_EGREP
AC_PROG_LN_S

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
#AC_COIN_PROG_CXX

# Get the C++ runtime libraries in case we want to link with a C or
# Fortran compiler
#AC_COIN_CXXLIBS

# Check if we have a Fortran compiler; if we do, acquire the appropriate
# options. This has to be done before we call AC_COIN_INIT_AUTO_TOOLS, else
# it will helpfully set FFLAGS to an incorrect value.
AC_COIN_FIND_F77
if test "$F77" != unavailable ; then
  AC_COIN_PROG_F77
  AC_COIN_F77_WRAPPERS
fi

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

#############################################################################
# Check for which linear solver we should provide a dynamic library loader  #
#############################################################################

AC_ARG_ENABLE([hslloader],
  AC_HELP_STRING([--enable-hslloader], [enables loader for dynamic library containing HSL routines (no)]),
  [hsl=yes],
  [hsl=no])

if test "$hsl" = yes ; then
  AC_ARG_ENABLE([ma27],
    AC_HELP_STRING([--enable-ma27], [enables load of MA27 from HSL library (no)]),
    AC_DEFINE([COIN_ENABLE_MA27LOADER], [1], [If defined, MA27 is load from a dynamic library]),
  )
  AC_ARG_ENABLE([mc19],
    AC_HELP_STRING([--enable-mc19], [enables load of MC19 from HSL library (no)]),
    AC_DEFINE([COIN_ENABLE_MC19LOADER], [1], [If defined, MC19 is load from a dynamic library]),
  )
  AC_ARG_ENABLE([ma57],
    AC_HELP_STRING([--enable-ma57], [enables load of MA57 from HSL library (no)]),
    AC_DEFINE([COIN_ENABLE_MA57LOADER], [1], [If defined, MA57 is load from a dynamic library]),
  )
  AC_ARG_ENABLE([ma28],
    AC_HELP_STRING([--enable-ma28], [enables load of MA28 from HSL library (no)]),
    AC_DEFINE([COIN_ENABLE_MA28LOADER], [1], [If defined, MA28 is load from a dynamic library]),
  )
fi

AM_CONDITIONAL([COIN_ENABLE_HSLLOADER], [test hsl=yes])


# mumps

# paradiso

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])

AC_CONFIG_HEADER([inc/config_linearsolverloader.h])

AC_COIN_FINALIZE
