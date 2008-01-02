# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = get.LX3 get.LEI get.DII get.VIS get.SIG FILES INSTALL.GAMSIO LICENSE

noinst_LTLIBRARIES = libGamsOptions.la libGamsGDX.la

nodist_libGamsOptions_la_SOURCES = $(GAMSIO_CODE)/optcc.c
libGamsOptions_la_LDFLAGS = 

nodist_libGamsGDX_la_SOURCES = $(GAMSIO_CODE)/gdxcc.c
libGamsGDX_la_LDFLAGS =

noinst_PROGRAMS = gdx2gams
gdx2gams_SOURCES = gdx2gams.c
gdx2gams_LDFLAGS = gdxcc.lo
gdx2gams_DEPENDENCIES = gdxcc.lo

if GAMSIO_IS_UNIX
gdx2gams_LDFLAGS += -ldl
endif

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/$(GAMSIO_CODE)` -D$(CIA_CODE)

########################################################################
#                           Extra Targets                              #
########################################################################


.PHONY:

# tmpdir = tmpdir

# Stuff to clean up

DISTCLEANFILES =

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc
				
include ../../BuildTools/Makemain.inc
