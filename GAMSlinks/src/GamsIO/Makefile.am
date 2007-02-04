# Copyright (C) GAMS Development 2006
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske
# Template from: Andreas Waechter

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

lib_LTLIBRARIES = libGamsIO.la
libGamsIO_la_SOURCES = ../GAMSlinksConfig.h \
  GamsModel.cpp GamsModel.hpp GamsFinalize.cpp \
  GamsMessageHandler.cpp GamsMessageHandler.hpp
#libGamsModel_la_LDFLAGS
libGamsIO_la_LIBADD = $(GAMSLINKSOBJDIR)/../ThirdParty/GAMSIO/libgamsoptions.la

include_HEADERS = ../../inc/config_gamslinks.h GamsModel.hpp GamsMessageHandler.hpp GamsFinalize.hpp

AM_CPPFLAGS = $(GAMSIO_CPPFLAGS) -I`$(CYGPATH_W) $(srcdir)/..` \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(OSISRCDIR)/src` \
 -I`$(CYGPATH_W) $(OSIOBJDIR)/inc`
 
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                  Installation of the addlibs file                    #
########################################################################

addlibsfile = gamsio_addlibs.txt

install-exec-local:
	$(install_sh_DATA) $(addlibsfile) $(DESTDIR)$(libdir)/$(addlibsfile)

uninstall-local:
	rm -f $(DESTDIR)$(libdir)/$(addlibsfile)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include ../../../BuildTools/Makemain.inc
