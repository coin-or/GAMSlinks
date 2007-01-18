# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

bin_PROGRAMS = GamsIpopt
GamsIpopt_SOURCES = GamsIpopt.cpp SMAGNLP.cpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`

GamsIpopt_LDADD = \
 $(IPOPTOBJDIR)/src/Interfaces/libipopt.la \
 $(SMAG_LIBS)

LIBS += `cat $(IPOPTOBJDIR)/src/Interfaces/ipopt_addlibs_cpp.txt`

GamsIpopt_LDFLAGS = $(ADDLIBS) -rpath $(libdir)
 
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_UNIX
install-exec-local:
	ln -sf $(DESTDIR)$(bindir)/GamsIpopt gmsip_ux.out
	zip gmsip_.zip gmsip_ux.out gmsip_us.run install/gmsip_un.dat
	$(install_sh_DATA) gmsip_.zip $(DESTDIR)$(bindir)/gmsip_.zip
	rm -f gmsip_ux.out
endif

if GAMSIO_IS_WINDOWS
install-exec-local:
	ln -sf $(DESTDIR)$(bindir)/GamsIpopt.exe gmsip_nx.exe
	zip gmsip_.zip gmsip_nx.exe gmsip_nt.cmd install/gmsip_nt.dat
	$(install_sh_DATA) gmsip_.zip $(DESTDIR)$(bindir)/gmsip_.zip
	rm -f gmsip_nx.exe
endif
	
uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsip_.zip

include ../../../BuildTools/Makemain.inc
