# Copyright (C) 2006-2009 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

# Author:  Stefan Vigerske

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([GAMSlinks],[0.4],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) 2006-2008 GAMS Development and others
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

AC_REVISION(0.9)

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(configure.ac)

# Where should everything be installed by default?  Here, we want it
# to be installed directly in 'bin', 'lib', 'include' subdirectories
# of the directory where configure is run.  The default would be
# /usr/local.
AC_PREFIX_DEFAULT([`pwd`])

#############################################################################
#              Create the libtool script for the subdirectories             #
#############################################################################

AC_LIBTOOL_DLOPEN
AC_LIBLTDL_CONVENIENCE
AC_COIN_CREATE_LIBTOOL

#############################################################################
#             Check which ThirdParty sources are available                  #
#############################################################################

#AC_COIN_THIRDPARTY_SUBDIRS([GAMSIO],[ThirdParty/GAMSIO],[get.GAMSIO])
AC_COIN_THIRDPARTY_SUBDIRS([Glpk],[ThirdParty/Glpk],[glpk/src/glplpx01.c])
AC_COIN_THIRDPARTY_SUBDIRS([Blas],[ThirdParty/Blas],[daxpy.f])
AC_COIN_THIRDPARTY_SUBDIRS([Lapack],[ThirdParty/Lapack],[LAPACK/SRC/dlarf.f])
AC_COIN_THIRDPARTY_SUBDIRS([HSL],[ThirdParty/HSL],[configure.ac])
AC_COIN_THIRDPARTY_SUBDIRS([Mumps],[ThirdParty/Mumps],[MUMPS/src/dmumps_part1.F])

#############################################################################
#                  Check which subprojects are there                        #
#############################################################################

#we want SCIP to be build after CoinUtils, Clp, Ipopt, and CppAD in order to use them there 

AC_COIN_MAIN_SUBDIRS(libltdl)
AC_COIN_MAIN_SUBDIRS(CoinUtils)
AC_COIN_MAIN_SUBDIRS(Clp)
AC_COIN_MAIN_SUBDIRS(Ipopt)
AC_COIN_MAIN_SUBDIRS(cppad)
AC_COIN_THIRDPARTY_SUBDIRS([Scip],[ThirdParty/SCIP],[scip/src/scip/scip.h])
AC_COIN_MAIN_SUBDIRS(DyLP)
AC_COIN_MAIN_SUBDIRS(Vol)
AC_COIN_MAIN_SUBDIRS(Cgl)
AC_COIN_MAIN_SUBDIRS(Osi)
AC_COIN_MAIN_SUBDIRS(SYMPHONY)
AC_COIN_MAIN_SUBDIRS(Cbc)
AC_COIN_MAIN_SUBDIRS(Bonmin)
AC_COIN_MAIN_SUBDIRS(Couenne)
AC_COIN_MAIN_SUBDIRS(OS)
AC_COIN_MAIN_SUBDIRS(GAMSlinks)

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
