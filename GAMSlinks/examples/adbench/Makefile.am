# Copyright (C) GAMS Development and others 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

bin_PROGRAMS = adbench
adbench_SOURCES = adbench.cpp
# if we have OS, then we compile also the Smag2OSiL interface and link with libSmagIO later
# if we do not have OS, then we do not link with libSmagIO, so we need compile in SmagExtra here
if COIN_HAS_OS
adbench_SOURCES += ../../src/OS/Smag2OSiL.cpp
else
adbench_SOURCES += ../../src/GamsIO/SmagExtra.c
endif

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) $(FORTRAN_NAMEMANGLING_CFLAG) \
  -I`$(CYGPATH_W) $(srcdir)/../../src` \
  -I`$(CYGPATH_W) $(srcdir)/../../src/GamsIO`

adbench_LDADD = $(GAMSIO_OBJDIR)/libGamsGDX.la ../../src/GamsIO/libSmagStricmp.la
adbench_DEPENDENCIES = $(GAMSIO_OBJDIR)/libGamsGDX.la ../../src/GamsIO/libSmagStricmp.la $(SMAG_LIBS)

LIBS += `cat $(GAMSLINKSOBJDIR)/smagio_addlibs.txt`

if COIN_HAS_OS
AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(srcdir)/../../src/OS` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSAgent` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSParsers` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)`

adbench_LDADD += ../../src/GamsIO/libSmagIO.la \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
adbench_DEPENDENCIES += ../../src/GamsIO/libSmagIO.la \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la

LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

endif

adbench_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/adbench gmsad_ux.out
	zip gmsad_.zip gmsad_ux.out gmsad_us.run install/gmsad_un.dat
	$(install_sh_DATA) gmsad_.zip $(DESTDIR)$(bindir)/gmsad_.zip
	rm -f gmsad_ux.out gmsad_.zip
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/adbench.exe gmsad_nx.exe
	zip gmsad_.zip gmsad_nx.exe gmsad_nt.cmd install/gmsad_nt.dat
	$(install_sh_DATA) gmsad_.zip $(DESTDIR)$(bindir)/gmsad_.zip
	rm -f gmsad_nx.exe gmsad_.zip
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsad_.zip

DISTCLEANFILES = 

include ../../../BuildTools/Makemain.inc
