# Copyright (C) 2006-2008 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id: Makefile.am 635 2009-03-28 19:48:00Z stefan $
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = gmsip_nt.cmd gmsip_us.run install/gmsip_nt.dat install/gmsip_un.dat

bin_PROGRAMS = GamsIpopt

#SmagExtra needed because it defined stricmp that is currently required by bch.o
GamsIpopt_SOURCES = GamsIpopt.cpp SmagNLP.cpp ../GamsIO/SmagJournal.cpp ../GamsIO/SmagExtra.c

includecoindir = $(includedir)/coin
includecoin_HEADERS = \
  $(IPOPTSRCDIR)/src/Algorithm/IpIpoptCalculatedQuantities.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpIpoptNLP.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpNLPScaling.hpp \
  $(IPOPTSRCDIR)/src/LinAlg/IpScaledMatrix.hpp \
  $(IPOPTSRCDIR)/src/LinAlg/IpSymScaledMatrix.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpIpoptData.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpIteratesVector.hpp \
  $(IPOPTSRCDIR)/src/LinAlg/IpCompoundVector.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpTimingStatistics.hpp \
  $(IPOPTSRCDIR)/src/Interfaces/IpTNLPAdapter.hpp \
  $(IPOPTSRCDIR)/src/Algorithm/IpOrigIpoptNLP.hpp

#FORTRAN_NAMEMANGLING_CFLAG and GAMSIO_CODE required to compile SmagExtra.c properly
AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) $(FORTRAN_NAMEMANGLING_CFLAG) -D$(GAMSIO_CODE) \
 -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(srcdir)/../GamsIO` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/LinearSolverLoader` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`

#GamsGDX required for bch.o
GamsIpopt_LDADD = $(IPOPTOBJDIR)/src/Interfaces/libipopt.la $(GAMSIO_OBJDIR)/libGamsGDX.la ../GamsIO/libSmagStricmp.la

GamsIpopt_DEPENDENCIES = $(IPOPTOBJDIR)/src/Interfaces/libipopt.la $(SMAG_LIBS) ../GamsIO/libSmagStricmp.la

LIBS += `cat $(IPOPTOBJDIR)/src/Interfaces/ipopt_addlibs_cpp.txt $(GAMSLINKSOBJDIR)/smagio_addlibs.txt`

GamsIpopt_LDFLAGS = $(ADDLIBS) -rpath $(libdir)

DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

DISTCLEANFILES =

if GAMSIO_IS_UNIX
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsIpopt gmsip_ux.out
	zip gmsip_.zip gmsip_ux.out gmsip_us.run install/gmsip_un.dat
	$(install_sh_DATA) gmsip_.zip $(DESTDIR)$(bindir)/gmsip_.zip
	rm -f gmsip_ux.out gmsip_.zip
endif

if GAMSIO_IS_WINDOWS
install-exec-local: install-binPROGRAMS
	ln -sf $(DESTDIR)$(bindir)/GamsIpopt.exe gmsip_nx.exe
	zip gmsip_.zip gmsip_nx.exe gmsip_nt.cmd install/gmsip_nt.dat
	$(install_sh_DATA) gmsip_.zip $(DESTDIR)$(bindir)/gmsip_.zip
	rm -f gmsip_nx.exe gmsip_.zip
endif

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/gmsip_.zip

include ../../../BuildTools/Makemain.inc
