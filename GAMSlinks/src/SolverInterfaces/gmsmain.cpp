// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef SOLVERNAME
#error "SOLVERNAME undefined"
#endif
#ifndef CREATEFUNCNAME
#error "CREATEFUNCNAME undefined"
#endif

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"
#include <ltdl.h>

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include "gmomcc.h"
#include "gevmcc.h"

int main(int argc, char** argv) {
#ifdef HAVE_WINDOWS_H
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
  LTDL_SET_PRELOADED_SYMBOLS();
  
  if (argc == 1) {
    fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n", argv[0]);
    return EXIT_FAILURE;
  }

  gmoHandle_t gmo;
  gevHandle_t gev;
  char msg[256];
  int rc;

  // initialize GMO:
  if (!gmoCreate(&gmo, msg, sizeof(msg))) {
    fprintf(stderr, "%s\n",msg);
    return EXIT_FAILURE;
  }

  if (!gevCreate(&gev, msg, sizeof(msg))) {
    fprintf(stderr, "%s\n",msg);
    return EXIT_FAILURE;
  }

  gmoIdentSet(gmo, SOLVERNAME "link object");
  
  // load control file
  if ((rc = gevInitEnvironmentLegacy(gev, argv[1]))) {
    fprintf(stderr, "Could not load control file: %s Rc = %d\n", argv[1], rc);
    gmoFree(&gmo);
    gevFree(&gev);
    return EXIT_FAILURE;
  }

  if ((rc = gmoRegisterEnvironment(gmo, gev, msg))) {
    gevLogStat(gev, "Could not register environment.");
    gmoFree(&gmo);
    gevFree(&gev);
    return EXIT_FAILURE;
  }

  // setup GAMS output channels
//  if ((rc = gmoOpenGms(gmo))) {
//  	fprintf(stderr, "Could not open GAMS environment. Rc = %d\n", rc);
//    gmoFree(&gmo);
//  	return EXIT_FAILURE;
//  }

  if ((rc = gmoLoadDataLegacy(gmo, msg))) {
    gevLogStat(gev, "Could not load model data.");
//    gmoCloseGms(gmo);
    gmoFree(&gmo);
    gevFree(&gev);
    return EXIT_FAILURE;
  }
  
  if ((rc = lt_dlinit())) {
    gevLogStat(gev, "Could not initialize dynamic library loader.");
//    gmoCloseGms(gmo);
    gmoFree(&gmo);
    gevFree(&gev);
    return EXIT_FAILURE;
  }
  
  lt_dlhandle coinlib = lt_dlopenext("libGamsCoin");
  if (!coinlib) {
    gevLogStat(gev, "Could not load GamsCoin library.");
    gevLogStat(gev, lt_dlerror());
//  	gmoCloseGms(gmo);
    gmoFree(&gmo);
    gevFree(&gev);
    return EXIT_FAILURE;
  }
  
  createNewGamsSolver_t* createsolver = (createNewGamsSolver_t*) lt_dlsym(coinlib, CREATEFUNCNAME);
  if (!createsolver) {
    gevLogStat(gev, "Could not load " CREATEFUNCNAME " symbol from GamsCoin library.");
    gevLogStat(gev, lt_dlerror());
//  	gmoCloseGms(gmo);
    gmoFree(&gmo);
    gevFree(&gev);
    lt_dlclose(coinlib);
    return EXIT_FAILURE;
  }
  
  GamsSolver* solver = (*createsolver)();
  gevLogStat(gev, "");
  gevLogStatPChar(gev, solver->getWelcomeMessage());

  bool ok = true;

  if (solver->readyAPI(gmo, NULL) != 0) {
    gevLogStat(gev, "There was an error in setting up " SOLVERNAME ".\n");
    ok = false;
  }
  if (ok && solver->callSolver() != 0) {
    gevLogStat(gev, "There was an error in solving the model.\n");
    ok = false;
  }
  gmoUnloadSolutionLegacy(gmo);

  delete solver;

// 	gevLogStat(gev, SOLVERNAME " finished.");
  
  //close output channels
//  gmoCloseGms(gmo);
  gmoFree(&gmo);
  gevFree(&gev);
  gmoLibraryUnload();
  gevLibraryUnload();

  lt_dlclose(coinlib);
  lt_dlexit();

  return EXIT_SUCCESS;
}
