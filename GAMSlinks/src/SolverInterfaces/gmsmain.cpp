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
  char msg[256];
  int rc;

  // initialize GMO:
  // first try path where GAMS I/O libraries were during compilation (the gmo library there should be the correct version)
  // if that fails, try using some global search path, so it should take the one from the gams installation (hope it is update enough) 
  if (!gmoCreateD(&gmo, GAMSIO_PATH, msg, sizeof(msg))) {
  	if (!gmoCreate(&gmo, msg, sizeof(msg))) {
  		fprintf(stderr, "%s\n",msg);
  		return EXIT_FAILURE;
  	}
  }

  gmoIdentSet(gmo, SOLVERNAME "link object");
  
  // load control file
  if ((rc = gmoLoadInfoGms(gmo, argv[1]))) {
  	fprintf(stderr, "Could not load control file: %s Rc = %d\n", argv[1], rc);
    gmoFree(&gmo);
  	return EXIT_FAILURE;
  }
  
  // setup GAMS output channels
  if ((rc = gmoOpenGms(gmo))) {
  	fprintf(stderr, "Could not open GAMS environment. Rc = %d\n", rc);
    gmoFree(&gmo);
  	return EXIT_FAILURE;
  }

  if ((rc = gmoLoadDataGms(gmo))) {
  	gmoLogStat(gmo, "Could not load model data.");
    gmoCloseGms(gmo);
    gmoFree(&gmo);
  	return EXIT_FAILURE;
  }
  
  if ((rc = lt_dlinit())) {
  	gmoLogStat(gmo, "Could not initialize dynamic library loader.");
    gmoCloseGms(gmo);
    gmoFree(&gmo);
  	return EXIT_FAILURE;
  }
  
  lt_dlhandle coinlib = lt_dlopenext("libGamsCoin");
  if (!coinlib) {
  	gmoLogStat(gmo, "Could not load GamsCoin library.");
  	gmoLogStat(gmo, lt_dlerror());
  	gmoCloseGms(gmo);
  	gmoFree(&gmo);
  	return EXIT_FAILURE;  	
  }
  
  createNewGamsSolver_t* createsolver = (createNewGamsSolver_t*) lt_dlsym(coinlib, CREATEFUNCNAME);
  if (!createsolver) {
  	gmoLogStat(gmo, "Could not load " CREATEFUNCNAME " symbol from GamsCoin library.");
  	gmoLogStat(gmo, lt_dlerror());
  	gmoCloseGms(gmo);
  	gmoFree(&gmo);
  	lt_dlclose(coinlib);
  	return EXIT_FAILURE;  	
  }
  
  GamsSolver* solver = (*createsolver)();
  gmoLogStat(gmo, "");
	gmoLogStatPChar(gmo, solver->getWelcomeMessage());
	
  bool ok = true;
	
  if (solver->readyAPI(gmo, NULL, NULL) != 0) {
  	gmoLogStat(gmo, "There was an error in setting up " SOLVERNAME ".\n");
//  	gmoSolveStatSet(gmo, SolveStat_SystemErr);
//  	gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
  	ok = false;
  }
  if (ok && solver->callSolver() != 0) {
  	gmoLogStat(gmo, "There was an error in solving the model.\n");
//  	gmoSolveStatSet(gmo, SolveStat_SystemErr);
//  	gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
  	ok = false;
  }
	gmoUnloadSolutionGms(gmo);
	
	delete solver;

// 	gmoLogStat(gmo, SOLVERNAME " finished.");
  
  //close output channels
  gmoCloseGms(gmo);
  //free gmo handle
  gmoFree(&gmo);
  //unload gmo library
  gmoLibraryUnload();

	lt_dlclose(coinlib);
	lt_dlexit();

	return EXIT_SUCCESS;
}
