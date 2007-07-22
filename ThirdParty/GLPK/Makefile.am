# Copyright (C) 2007 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = get.GLPK

if SKIPMAKE
  noinst_LTLIBRARIES =
else
  noinst_LTLIBRARIES = libcoinglpk.la
endif

GLPKDIR = @GLPKDIR@

nodist_libcoinglpk_la_SOURCES = \
$(GLPKDIR)/src/glpapi01.c \
$(GLPKDIR)/src/glpapi02.c \
$(GLPKDIR)/src/glpapi03.c \
$(GLPKDIR)/src/glpapi04.c \
$(GLPKDIR)/src/glpapi05.c \
$(GLPKDIR)/src/glpapi06.c \
$(GLPKDIR)/src/glpapi07.c \
$(GLPKDIR)/src/glpapi08.c \
$(GLPKDIR)/src/glpapi09.c \
$(GLPKDIR)/src/glpapi10.c \
$(GLPKDIR)/src/glpavl.c \
$(GLPKDIR)/src/glpbfd.c \
$(GLPKDIR)/src/glpbfx.c \
$(GLPKDIR)/src/glpdmp.c \
$(GLPKDIR)/src/glpfhv.c \
$(GLPKDIR)/src/glpgmp.c \
$(GLPKDIR)/src/glphbm.c \
$(GLPKDIR)/src/glpiet.c \
$(GLPKDIR)/src/glpios01.c \
$(GLPKDIR)/src/glpios02.c \
$(GLPKDIR)/src/glpios03.c \
$(GLPKDIR)/src/glpipm.c \
$(GLPKDIR)/src/glpipp01.c \
$(GLPKDIR)/src/glpipp02.c \
$(GLPKDIR)/src/glplib01.c \
$(GLPKDIR)/src/glplib02.c \
$(GLPKDIR)/src/glplib03.c \
$(GLPKDIR)/src/glplib04.c \
$(GLPKDIR)/src/glplib05.c \
$(GLPKDIR)/src/glplib06.c \
$(GLPKDIR)/src/glplib07.c \
$(GLPKDIR)/src/glplib08.c \
$(GLPKDIR)/src/glplib09.c \
$(GLPKDIR)/src/glplpf.c \
$(GLPKDIR)/src/glplpp01.c \
$(GLPKDIR)/src/glplpp02.c \
$(GLPKDIR)/src/glplpx01.c \
$(GLPKDIR)/src/glplpx02.c \
$(GLPKDIR)/src/glplpx03.c \
$(GLPKDIR)/src/glplpx04.c \
$(GLPKDIR)/src/glplpx05.c \
$(GLPKDIR)/src/glplpx06.c \
$(GLPKDIR)/src/glplpx07.c \
$(GLPKDIR)/src/glplpx08.c \
$(GLPKDIR)/src/glplpx09.c \
$(GLPKDIR)/src/glplpx10.c \
$(GLPKDIR)/src/glplpx11.c \
$(GLPKDIR)/src/glplpx12.c \
$(GLPKDIR)/src/glplpx13.c \
$(GLPKDIR)/src/glplpx14.c \
$(GLPKDIR)/src/glplpx15.c \
$(GLPKDIR)/src/glplpx16.c \
$(GLPKDIR)/src/glplpx17.c \
$(GLPKDIR)/src/glplpx18.c \
$(GLPKDIR)/src/glplpx19.c \
$(GLPKDIR)/src/glpluf.c \
$(GLPKDIR)/src/glplux.c \
$(GLPKDIR)/src/glpmat.c \
$(GLPKDIR)/src/glpmip01.c \
$(GLPKDIR)/src/glpmip02.c \
$(GLPKDIR)/src/glpqmd.c \
$(GLPKDIR)/src/glprng.c \
$(GLPKDIR)/src/glpscf.c \
$(GLPKDIR)/src/glpscs.c \
$(GLPKDIR)/src/glpspx01.c \
$(GLPKDIR)/src/glpspx02.c \
$(GLPKDIR)/src/glpssx01.c \
$(GLPKDIR)/src/glpssx02.c

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)`

AM_CPPFLAGS = -I`$(CYGPATH_W) $(srcdir)/$(GLPKDIR)/include`

libcoinglpk_la_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES = $(MAKEOKFILE)

include ../../BuildTools/Makemain.inc
