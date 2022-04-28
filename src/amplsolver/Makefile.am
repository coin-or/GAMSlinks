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
  ../utils/gmomcc.c ../utils/gevmcc.c ../utils/cfgmcc.c ../utils/optcc.c
libGamsAmplSolver_la_LIBADD = $(GAMSAMPLSOLVER_LFLAGS)

CLEANFILES =
include ../utils/Makefile_gamsapi.inc

noinst_PROGRAMS = optamplsolver
optamplsolver_SOURCES = optamplsolver.cpp ../utils/GamsOptionsSpecWriter.cpp

pkgdata_DATA = optamplsolver.def
doc_DATA = optamplsolver.md
optamplsolver.def optamplsolver.md : optamplsolver$(EXEEXT)
	./optamplsolver$(EXEEXT)
