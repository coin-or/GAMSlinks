# Copyright (C) 2008-2011 GAMS Development and others
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

test:
	sh -c ./run_quality
	sh -c ./run_gmstest

unitTest: test

clean-local:
	rm -rf gmstest quality

.PHONY: test unitTest

DISTCLEANFILES = 

include ../BuildTools/Makemain.inc
