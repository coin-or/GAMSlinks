# Copyright (C) 2009-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

noinst_LTLIBRARIES = libGamsUtils.la
libGamsUtils_la_SOURCES = \
  GamsHelper.c \
  GamsLicensing.c \
  GamsSolveTrace.c \
  optcc.c \
  gmomcc.c \
  gevmcc.c \
  gclgms.c \
  gdxcc.c \
  palmcc.c
libGamsUtils_la_LDFLAGS =

optcc.c : $(GAMS_PATH)/apifiles/C/api/optcc.c
	@cp $< $@

gmomcc.c : $(GAMS_PATH)/apifiles/C/api/gmomcc.c
	@cp $< $@

gevmcc.c : $(GAMS_PATH)/apifiles/C/api/gevmcc.c
	@cp $< $@

gclgms.c : $(GAMS_PATH)/apifiles/C/api/gclgms.c
	@cp $< $@

gdxcc.c : $(GAMS_PATH)/apifiles/C/api/gdxcc.c
	@cp $< $@

palmcc.c : $(GAMS_PATH)/apifiles/C/api/palmcc.c
	@cp $< $@

CLEANFILES = optcc.c gmomcc.c gevmcc.c gclgms.c gdxcc.c palmcc.c

AM_CPPFLAGS = -I$(srcdir)/.. -I"$(GAMS_PATH)/apifiles/C/api" $(GAMSLIBCFLAGS)
