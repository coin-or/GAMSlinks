## Process this file with autoconf to create configure. -*- autoconf -*-
# Copyright 2001  Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA
#
# Modified to comply with COIN-OR build system

AC_PREREQ(2.59)

AC_INIT([libltdl], [1.2], [bug-libtool@gnu.org])
AC_CONFIG_SRCDIR([ltdl.c])
AC_PREFIX_DEFAULT([`pwd`])

# Get the system type
AC_CANONICAL_BUILD

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(libltdl)

# Get the name of the C compiler and appropriate compiler options
coin_skip_warn_cflags=yes
AC_COIN_PROG_CC
AC_C_CONST
AC_C_INLINE

# Initialize automake and libtool
#AC_CONFIG_AUX_DIR([.])
AC_COIN_INIT_AUTO_TOOLS

## ------------------------------- ##
## Libltdl specific configuration. ##
## ------------------------------- ##

if test -z "$enable_ltdl_install$enable_ltdl_convenience"; then
  if test -f ${srcdir}/../ltmain.sh; then
    # if libltdl is libtoolized, it is assumed to be stand-alone and
    # installed unless the command line overrides it (tested above)
    enable_ltdl_install=yes
  else
    AC_MSG_WARN([*** The top-level configure must select either])
    AC_MSG_WARN([*** [A""C_LIBLTDL_INSTALLABLE] or [A""C_LIBLTDL_CONVENIENCE].])
    AC_MSG_ERROR([*** Maybe you want to --enable-ltdl-install?])
  fi
fi

LT_AC_PROG_SED
AC_LIBTOOL_SYS_DYNAMIC_LINKER
AC_LIB_LTDL

## -------- ##
## Outputs. ##
## -------- ##
AC_CONFIG_FILES([Makefile])
AM_CONFIG_HEADER([config.h:config-h.in])
AC_COIN_FINALIZE
