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
	chmod u+x ./run_tests
	sh -c ./run_tests

unitTest: test

.PHONY: test unitTest

DISTCLEANFILES = 

include ../../BuildTools/Makemain.inc
