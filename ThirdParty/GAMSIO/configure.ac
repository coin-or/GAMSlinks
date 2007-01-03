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
AC_CONFIG_SRCDIR(LX3/iolib.h)

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
#AC_COIN_PROG_CC

# Get the name of the C++ compiler and appropriate compiler options
#AC_COIN_PROG_CXX

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

#AC_COIN_HAS_PROJECT(GAMSlinks)
#AC_COIN_HAS_PROJECT(Cgl)
#AC_COIN_HAS_PROJECT(CoinUtils)
#AC_COIN_HAS_PROJECT(Osi)
# later: this one is optional, if SNOPT is there
#AC_COIN_HAS_PROJECT(Ipopt)
# later: this one is optional, if, e.g., CPLEX is there
#AC_COIN_HAS_PROJECT(Clp)

############################################################################
############################################################################
#                   Stuff that we need for C++ programs                    #
############################################################################
############################################################################

#AC_LANG_PUSH(C++)

################
# Header Files #
################


#AC_COIN_CHECK_CXX_CHEADER(assert)
#AC_COIN_CHECK_CXX_CHEADER(stdio)
#AC_COIN_CHECK_CXX_CHEADER(stdlib)
#AC_COIN_CHECK_CXX_CHEADER(math)
#AC_COIN_CHECK_CXX_CHEADER(ctype)
#AC_COIN_CHECK_CXX_CHEADER(stdarg)
#AC_COIN_CHECK_CXX_CHEADER(float)
#AC_COIN_CHECK_CXX_CHEADER(time)


############
# END PART #
############

#AC_LANG_POP(C++)

AC_CONFIG_FILES([Makefile])

#just to avoid an error when running autoheaders
AC_CONFIG_HEADER([config_gamsio.h])

AC_COIN_FINALIZE
