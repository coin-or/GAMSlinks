# Copyright (C) 2006-2008 GAMS Development and others
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

AC_INIT([ThirdPartyGAMSIO],[0.0],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) 2006-2008 GAMS Development and others.
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(get.GAMSIO)

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

#turn off warnings and pedantic mode since it seem to make trouble with dlsym in gdxwrap.c:
#gcc 4.2.1 error: ISO C forbids conversion of object pointer to function pointer type
coin_skip_warn_cflags=yes

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(GAMSIO)

# We need egrep and ln -s somewhere
#AC_PROG_EGREP
#AC_PROG_LN_S

# Get the name of the C compiler and appropriate compiler options
AC_COIN_PROG_CC

# Even though we do not need to compile Fortran code here, we want to know the name of Fortran compiler,
# so we can decide between LEI and LEG and LX3 and LNX, respectively. 
AC_COIN_FIND_F77

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

################
# GAMSIO stuff #
################

AC_COIN_HAVE_GAMS

############
# END PART #
############

AC_CONFIG_FILES([Makefile])

AC_COIN_FINALIZE
