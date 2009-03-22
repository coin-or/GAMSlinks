// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GAMSlinksConfig.h"
#include "GamsCbc.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#include "CoinError.hpp"

extern "C" {
#include "gmocc.h"
}

int main(int argc, char** argv) {
	WindowsErrorPopupBlocker();
	
  if (argc == 1) {
  	fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n", argv[0]);
    return EXIT_FAILURE;
  }

  gmoHandle_t gmo;
  char msg[256];
  int rc;

  // initialize GMO
  if (!gmoCreateD(&gmo, "/home/stefan/work/coin/GAMSlinks-trunk/ThirdParty/GAMSIO/LEG", msg, sizeof(msg))) {
  	fprintf(stderr, "%s\n",msg);
    return EXIT_FAILURE;
  }

  gmoIdentSet(gmo, "CBClink object");
  
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
  
  GamsSolver* cbc = createNewGamsCbc();
  gmoLogStat(gmo, "");
	gmoLogStatPChar(gmo, cbc->getWelcomeMessage());
	
  bool ok = true;
	
  if (!cbc->readyAPI(gmo, NULL, NULL)) {
  	gmoLogStat(gmo, "There was an error in setting up CBC.\n");
  	gmoSolveStatSet(gmo, SolveStat_SystemErr);
  	gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
  	ok = false;
  }
  if (ok && !cbc->callSolver()) {
  	gmoLogStat(gmo, "There was an error in solving the model by CBC.\n");
  	gmoSolveStatSet(gmo, SolveStat_SystemErr);
  	gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
  	ok = false;
  }
	gmoUnloadSolutionGms(gmo);
	
	delete cbc;

// 	gmoLogStat(gmo, "CBC finished.");
  
  //close output channels
  gmoCloseGms(gmo);
  //free gmo handle
  gmoFree(&gmo);
  //unload gmo library
  gmoLibraryUnload();
	
	return EXIT_SUCCESS;
}
