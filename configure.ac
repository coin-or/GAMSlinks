# Copyright (C) 2006-2009 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id: configure.ac 636 2009-03-29 15:39:47Z stefan $

# Author:  Stefan Vigerske

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([GamsUtils],[trunk],[https://projects.coin-or.org/GAMSlinks/newticket])

AC_COPYRIGHT([Copyright (C) 2009 GAMS Development and others
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

AC_COIN_CREATE_LIBTOOL

#############################################################################
#             Check which ThirdParty sources are available                  #
#############################################################################

AC_COIN_THIRDPARTY_SUBDIRS([GAMSIO],[ThirdParty/GAMSIO],[get.GAMSIO])

#############################################################################
#                  Check which subprojects are there                        #
#############################################################################

#we want SCIP to be build after CoinUtils and Clp in order to use them there 

AC_COIN_MAIN_SUBDIRS(GamsUtils)

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
