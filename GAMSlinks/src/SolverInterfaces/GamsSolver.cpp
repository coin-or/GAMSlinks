// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
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
#include "gevmcc.h"

#ifdef GAMS_BUILD
#include "gmspal.h"
#include "gevlice.h"
#endif

GamsSolver::~GamsSolver() {
	if (need_unload_gmo) {
		assert(gmoLibraryLoaded());
		gmoLibraryUnload();
	}
	if (need_unload_gev) {
		assert(gevLibraryLoaded());
		gevLibraryUnload();
	}
}

int GamsSolver::getGmoReady() {
	if (gmoLibraryLoaded())
		return 0;

	char msg[256];
//#ifndef GAMS_BUILD
//  if (!gmoGetReadyD(GAMSIO_PATH, msg, sizeof(msg)))
//#endif
  	if (!gmoGetReady(msg, sizeof(msg))) {
  		fprintf(stderr, "Error loading GMO library: %s\n",msg);
  		return 1;
  	}

  need_unload_gmo = true;

  return 0;
}

int GamsSolver::getGevReady() {
	if (gevLibraryLoaded())
		return 0;

	char msg[256];
//#ifndef GAMS_BUILD
//  if (!gevGetReadyD(GAMSIO_PATH, msg, sizeof(msg)))
//#endif
  	if (!gevGetReady(msg, sizeof(msg))) {
  		fprintf(stderr, "Error loading GEV library: %s\n",msg);
  		return 1;
  	}

  need_unload_gev = true;

  return 0;
}

bool GamsSolver::checkLicense(struct gmoRec* gmo) {
#ifdef GAMS_BUILD
  gevRec* gev = (gevRec*)gmoEnvironment(gmo);
#define GEVPTR gev
#include "cmagic2.h"
  if (licenseCheck(gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo))) {
    char msg[256];
    gevLogStat(gev, "The license check failed:\n");
    while (licenseGetMessage(msg, sizeof(msg)))
      gevLogStat(gev, msg);
    return false;
  }
#undef GEVPTR
#else
  return true;
#endif
}

bool GamsSolver::registerGamsCplexLicense(struct gmoRec* gmo) {
#if defined(COIN_HAS_CPX) && defined(GAMS_BUILD)
  int rc, cp_l=0, cp_m=0, cp_q=0, cp_p=0;
  CPlicenseInit_t initType;
  gevRec* gev = (gevRec*)gmoEnvironment(gmo);

  /* Cplex license setup */
  rc = gevcplexlice(gev,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo), gmoNDisc(gmo), 0, &initType, &cp_l, &cp_m, &cp_q, &cp_p);
  if (rc || (0==rc && ((0==cp_m && gmoNDisc(gmo)) || (0==cp_q && gmoNLNZ(gmo)))))
    return false;
  return true;
#else
  return true;
#endif
}
