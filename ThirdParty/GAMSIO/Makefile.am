# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = get.LX3 get.LEI get.DAR get.VIS

lib_LTLIBRARIES = libgamsoptions.la
libgamsoptions_la_SOURCES = $(GAMSIO_CODE)/optcc.c $(GAMSIO_CODE)/optco.cpp
libgamsoptions_la_LDFLAGS = 

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/$(GAMSIO_CODE)` -DCIA_$(GAMSIO_CODE2)

########################################################################
#                           Extra Targets                              #
########################################################################


.PHONY:

# tmpdir = tmpdir

# Stuff to clean up

DISTCLEANFILES =

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc
				
include ../../BuildTools/Makemain.inc
