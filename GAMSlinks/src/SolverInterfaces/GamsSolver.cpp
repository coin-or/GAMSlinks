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
#endif

#include "GamsCompatibility.h"

#ifdef HAVE_GOTO_SETNUMTHREADS
extern "C" void goto_set_num_threads(int);
#endif

#ifdef HAVE_MKL_SETNUMTHREADS
extern "C" void MKL_Domain_Set_Num_Threads(int, int);
//extern "C" void MKL_Set_Num_Threads(int);
#endif

void GamsSolver::setNumThreadsBlas(struct gevRec* gev, int nthreads) {
#ifdef HAVE_GOTO_SETNUMTHREADS
	if (gev != NULL && nthreads > 1) {
		char msg[100];
		sprintf(msg, "Limit number of threads in GotoBLAS to %d.\n", nthreads);
		gevLogPChar(gev, msg);
	}
  goto_set_num_threads(nthreads);
#endif
#ifdef HAVE_MKL_SETNUMTHREADS
	if (gev != NULL && nthreads > 1) {
		char msg[100];
		sprintf(msg, "Limit number of threads in MKL BLAS to %d.\n", nthreads);
		gevLogPChar(gev, msg);
	}
  MKL_Domain_Set_Num_Threads(nthreads, 1); // 1 = Blas
#endif
}

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
  return true;
#undef GEVPTR
#else
  return true;
#endif
}

bool GamsSolver::registerGamsCplexLicense(struct gmoRec* gmo) {
#if defined(COIN_HAS_CPX) && defined(GAMS_BUILD)
  gevRec* gev = (gevRec*)gmoEnvironment(gmo);

/* bad bad bad */ 
#undef SUB_FR
#define GEVPTR gev
#define SUB_OC
#include "cmagic2.h"
  if (licenseCheck(gmoM(gmo), gmoN(gmo), gmoNZ(gmo), gmoNLNZ(gmo), gmoNDisc(gmo))) {
    // Check for CP and CL license
    if (licenseCheckSubSys(2, "CPCL")) {
      gevLogStat(gev,"***");
      gevLogStat(gev,"*** LICENSE ERROR:");
      gevLogStat(gev,"*** See http://www.gams.com/osicplex/ for OsiCplex licensing information.");
      gevLogStat(gev,"***");
      return false;
    } else {
      int isLicMIP=0, isLicCP=1;
      licenseQueryOption("CPLEX","MIP", &isLicMIP);
      licenseQueryOption("CPLEX","GMSLICE", &isLicCP);
      if (0==isLicMIP && 1==isLicCP && gmoNDisc(gmo)) {
        gevLogStat(gev,"*** MIP option not licensed. Can solve continuous models only.");
        return false;
      }
    }
  }
  return true;
#else
  return true;
#endif
}
