// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsBonmin.cpp 699 2009-07-26 12:00:54Z stefan $
//
// Author: Stefan Vigerske

#include "GamsSolver.hpp"

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

#ifdef HAVE_CASSERT
#include <cassert>
#else
#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#error "don't have header file for assert"
#endif
#endif

#include "gmomcc.h"

GamsSolver::~GamsSolver() {
	if (need_unload_gmo) {
		assert(gmoLibraryLoaded());
		gmoLibraryUnload();
	}
}

int GamsSolver::getGmoReady() {
	if (gmoLibraryLoaded())
		return 0;
	
	char msg[256];
#ifndef GAMS_BUILD
  if (!gmoGetReadyD(GAMSIO_PATH, msg, sizeof(msg)))
#endif
  	if (!gmoGetReady(msg, sizeof(msg))) {
  		fprintf(stderr, "Error loading GMO library: %s\n",msg);
  		return 1;
  	}
  
  need_unload_gmo = true;
  
  return 0;
}
