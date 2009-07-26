# Copyright (C) 2006-2009 GAMS Development and others 
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = get.GAMSIO INSTALL.GAMSIO LICENSE

if GAMSIO_PRESENT
noinst_LTLIBRARIES = libGamsOptions.la libGamsGDX.la libGamsJoat.la
endif

nodist_libGamsOptions_la_SOURCES = $(GAMSIO_CODE)/optcc.c
libGamsOptions_la_LDFLAGS =

nodist_libGamsGDX_la_SOURCES = $(GAMSIO_CODE)/gdxcc.c
libGamsGDX_la_LDFLAGS =

nodist_libGamsJoat_la_SOURCES = $(GAMSIO_CODE)/gmomcc.c $(GAMSIO_CODE)/dctmcc.c
libGamsJoat_la_LDFLAGS =

if GAMSIO_PRESENT
noinst_PROGRAMS = gdx2gams
endif
gdx2gams_SOURCES = gdx2gams.c
gdx2gams_LDFLAGS = gdxcc.lo
gdx2gams_DEPENDENCIES = gdxcc.lo

if GAMSIO_IS_UNIX
gdx2gams_LDFLAGS += -ldl
endif

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/$(GAMSIO_CODE)` -D$(GAMSIO_CIA)

########################################################################
#                           Extra Targets                              #
########################################################################


.PHONY:

# tmpdir = tmpdir

# Stuff to clean up

DISTCLEANFILES =

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc
				
include ../../BuildTools/Makemain.inc
