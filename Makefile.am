# Copyright (C) GAMS Development 2007-2011
# All Rights Reserved.
# This file is distributed under the Eclipse Public License.
#
# Author: Stefan Vigerske

SUBDIRS = src

if ALWAYS_FALSE
  SUBDIRS += test
endif

doc_DATA = README.md LICENSE AUTHORS

test: all
	cd test; $(MAKE) test

doc:
	cd doc && doxygen

.PHONY: test unitTest doc

# Stuff to clean up
clean-doc :
	cd doc && rm -rf html *.log

clean-local: clean-doc
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi
