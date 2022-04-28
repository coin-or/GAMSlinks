# Copyright (C) GAMS Development and others 2022
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AM_CPPFLAGS = \
  -I$(srcdir)/../utils

AM_LDFLAGS = $(LT_LDFLAGS)

lib_LTLIBRARIES = libGamsAmplSolver.la
libGamsAmplSolver_la_SOURCES = amplsolver.c convert_nl.c gamsnl.c \
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/palmcc.c
#libGamsAmplSolver_la_LIBADD =

CLEANFILES =
include ../utils/Makefile_gamsapi.inc
