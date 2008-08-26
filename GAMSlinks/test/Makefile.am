# Copyright (C) GAMS Development and others 2008
# All Rights Reserved.
# This file is distributed under the Common Public License.
#
## $Id$
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

test:
	chmod u+x ./run_quality ./run_gmstest
	sh -c ./run_quality
	sh -c ./run_gmstest

unitTest: test

.PHONY: test unitTest

DISTCLEANFILES = 

include ../../BuildTools/Makemain.inc
