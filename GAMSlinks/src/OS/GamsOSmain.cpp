// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Stefan Vigerske

#include "GAMSlinksConfig.h"

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

#include "CoinError.hpp"

extern "C" {
#include "gmocc.h"
}
#include "GamsOS2.hpp"

int main (int argc, char* argv[]) {
	WindowsErrorPopupBlocker();

  if (argc == 1) {
  	fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n", argv[0]);
    return EXIT_FAILURE;
  }

  gmoHandle_t gmo;
  char msg[256];
  int rc;

  // initialize GMO
  if (!gmoCreate(&gmo, msg, sizeof(msg))) {
  	fprintf(stderr, "%s\n",msg);
    return EXIT_FAILURE;
  }

  gmoIdentSet(gmo, "OSlink object");
  
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

#ifdef GAMS_BUILD
	gmoLogStat(gmo, "\nGAMS/CoinOS (OS Library 1.1)\nwritten by Jun Ma, Kipp Martin, ...\n");
#else
	gmoLogStat(gmo, "\nGAMS/OS (OS Library 1.1)\nwritten by Jun Ma, Kipp Martin, ...\n");
#endif
  
  if ((rc = gmoLoadDataGms(gmo))) {
  	gmoLogStat(gmo, "Could not load model data.");
    gmoCloseGms(gmo);
    gmoFree(&gmo);
  	return EXIT_FAILURE;
  }
  
  GamsOS os(gmo);
  bool success = os.execute();
	gmoUnloadSolutionGms(gmo);

  if (success) {
  	gmoLogStat(gmo, "GAMS/OS finished.");
  } else {
  	gmoLogStat(gmo, "GAMS/OS finished with an error.");
  }
  
  //close output channels
  gmoCloseGms(gmo);
  //free gmo handle
  gmoFree(&gmo);
  //unload gmo library
  gmoLibraryUnload();

  return EXIT_SUCCESS;
}

