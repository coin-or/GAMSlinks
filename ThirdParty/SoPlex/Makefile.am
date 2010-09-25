# Copyright (C) 2010 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id: Makefile.am 928 2010-09-14 12:18:54Z stefan $

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

if COIN_BUILD_SOPLEX
  noinst_LTLIBRARIES = libcoinsoplex.la
else
  noinst_LTLIBRARIES =
endif

AM_CPPFLAGS = -I`$(CYGPATH_W) $(SOPLEXINCDIR)`

libcoinsoplex_la_SOURCES = \
  @SOPLEXDIR@/src/changesoplex.cpp \
  @SOPLEXDIR@/src/didxset.cpp \
  @SOPLEXDIR@/src/dsvector.cpp \
  @SOPLEXDIR@/src/dvector.cpp \
  @SOPLEXDIR@/src/enter.cpp \
  @SOPLEXDIR@/src/factor.cpp \
  @SOPLEXDIR@/src/forest.cpp \
  @SOPLEXDIR@/src/idxset.cpp \
  @SOPLEXDIR@/src/leave.cpp \
  @SOPLEXDIR@/src/lpcolset.cpp \
  @SOPLEXDIR@/src/lprowset.cpp \
  @SOPLEXDIR@/src/lprow.cpp \
  @SOPLEXDIR@/src/message.cpp \
  @SOPLEXDIR@/src/mpsinput.cpp \
  @SOPLEXDIR@/src/nameset.cpp \
  @SOPLEXDIR@/src/slufactor.cpp \
  @SOPLEXDIR@/src/solve.cpp \
  @SOPLEXDIR@/src/soplex.cpp \
  @SOPLEXDIR@/src/spxbasis.cpp \
  @SOPLEXDIR@/src/spxbounds.cpp \
  @SOPLEXDIR@/src/spxchangebasis.cpp \
  @SOPLEXDIR@/src/spxequilisc.cpp \
  @SOPLEXDIR@/src/spxdantzigpr.cpp \
  @SOPLEXDIR@/src/spxdefaultrt.cpp \
  @SOPLEXDIR@/src/spxdefines.cpp \
  @SOPLEXDIR@/src/spxdesc.cpp \
  @SOPLEXDIR@/src/spxdevexpr.cpp \
  @SOPLEXDIR@/src/spxfastrt.cpp \
  @SOPLEXDIR@/src/spxfileio.cpp \
  @SOPLEXDIR@/src/spxgeometsc.cpp \
  @SOPLEXDIR@/src/spxharrisrt.cpp \
  @SOPLEXDIR@/src/spxhybridpr.cpp \
  @SOPLEXDIR@/src/spxid.cpp \
  @SOPLEXDIR@/src/spxio.cpp \
  @SOPLEXDIR@/src/spxlp.cpp \
  @SOPLEXDIR@/src/spxlpfread.cpp \
  @SOPLEXDIR@/src/spxmainsm.cpp \
  @SOPLEXDIR@/src/spxmpsread.cpp \
  @SOPLEXDIR@/src/spxmpswrite.cpp \
  @SOPLEXDIR@/src/spxlpfwrite.cpp \
  @SOPLEXDIR@/src/spxout.cpp \
  @SOPLEXDIR@/src/spxparmultpr.cpp \
  @SOPLEXDIR@/src/spxquality.cpp \
  @SOPLEXDIR@/src/spxscaler.cpp \
  @SOPLEXDIR@/src/spxshift.cpp \
  @SOPLEXDIR@/src/spxsolver.cpp \
  @SOPLEXDIR@/src/spxsolve.cpp \
  @SOPLEXDIR@/src/spxstarter.cpp \
  @SOPLEXDIR@/src/spxsteeppr.cpp \
  @SOPLEXDIR@/src/spxsumst.cpp \
  @SOPLEXDIR@/src/spxvecs.cpp \
  @SOPLEXDIR@/src/spxvectorst.cpp \
  @SOPLEXDIR@/src/spxweightpr.cpp \
  @SOPLEXDIR@/src/spxweightst.cpp \
  @SOPLEXDIR@/src/spxwritestate.cpp \
  @SOPLEXDIR@/src/ssvector.cpp \
  @SOPLEXDIR@/src/svector.cpp \
  @SOPLEXDIR@/src/svset.cpp \
  @SOPLEXDIR@/src/timer.cpp \
  @SOPLEXDIR@/src/tracemethod.cpp \
  @SOPLEXDIR@/src/unitvector.cpp \
  @SOPLEXDIR@/src/update.cpp \
  @SOPLEXDIR@/src/updatevector.cpp \
  @SOPLEXDIR@/src/vector.cpp \
  @SOPLEXDIR@/src/vsolve.cpp \
  @SOPLEXDIR@/src/gzstream.cpp

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)`

libcoinsoplex_la_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES = $(MAKEOKFILE)

include ../../BuildTools/Makemain.inc
