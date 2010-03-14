# Copyright (C) GAMS Development 2007-2009
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmsos_nt.cmd gmsos_us.run install/gmsos_nt.dat install/gmsos_un.dat

bin_PROGRAMS = GamsOS

GamsOS_SOURCES      = GamsOS.cpp Smag2OSiL.cpp OSrL2Smag.cpp
GamsOS_LDADD        = ../GamsIO/libSmagIO.la ../GamsIO/libSmagStricmp.la
GamsOS_DEPENDENCIES = ../GamsIO/libSmagIO.la ../GamsIO/libSmagStricmp.la

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSCommonInterfaces` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSUtils` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSAgent` \
 -I`$(CYGPATH_W) $(OSSRCDIR)/src/OSParsers` \
 -I`$(CYGPATH_W) $(OSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CPPADSRCDIR)`

GamsOS_LDADD += \
 $(OSOBJDIR)/src/OSCommonInterfaces/libOSCommonInterfaces.la \
 $(OSOBJDIR)/src/OSParsers/libOSParsers.la \
 $(OSOBJDIR)/src/OSUtils/libOSUtils.la \
 $(OSOBJDIR)/src/OSAgent/libOSAgent.la \
 $(COINUTILSOBJDIR)/src/libCoinUtils.la
 
GamsOS_DEPENDENCIES += ../GamsIO/libSmagIO.la $(SMAG_LIBS)

LIBS += `cat $(OSOBJDIR)/os_addlibs.txt`
LIBS += `cat $(COINUTILSOBJDIR)/coinutils_addlibs.txt`

#TODO: this is actually only needed for GamsOS, but not for GamsOS2
#unfortunately adding this to LDFLAGS does not give the right order of linker arguments in a debug build
LIBS += `cat $(GAMSLINKSOBJDIR)/smagio_addlibs.txt`
GamsOS_LDFLAGS   = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsOS gmsos_ux.out
	zip gmsos_.zip gmsos_ux.out gmsos_us.run optos.def install/gmsos_un.dat
	$(install_sh_DATA) gmsos_.zip $(DESTDIR)$(bindir)/gmsos_.zip
	rm -f gmsos_ux.out gmsos_.zip
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsOS.exe gmsos_nx.exe
	zip gmsos_.zip gmsos_nx.exe gmsos_nt.cmd optos.def install/gmsos_nt.dat
	$(install_sh_DATA) gmsos_.zip $(DESTDIR)$(bindir)/gmsos_.zip
	rm -f gmsos_nx.exe gmsos_.zip
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsos_.zip

include ../../../BuildTools/Makemain.inc
