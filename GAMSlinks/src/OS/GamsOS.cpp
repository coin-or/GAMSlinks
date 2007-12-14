// Copyright (C) GAMS Development 2007
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

#include "smag.h"
#include "Smag2OSiL.hpp"

#include "OSiLWriter.h"

int main (int argc, char* argv[]) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif
  smagHandle_t prob;

  if (argc < 2) {
  	fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n", argv[0]);
    return EXIT_FAILURE;
  }

  prob = smagInit (argv[1]);
  if (!prob) {
  	fprintf(stderr, "Error reading control file %s\nexiting ...\n", argv[1]);
		return EXIT_FAILURE;
  }

  char buffer[512];
  prob->logFlush = 1; // flush output more often to avoid funny look
  if (smagStdOutputStart(prob, SMAG_STATUS_OVERWRITE_IFDUMMY, buffer, sizeof(buffer)))
  	fprintf(stderr, "Warning: Error opening GAMS output files .. continuing anyhow\t%s\n", buffer);

  smagReadModelStats (prob);
  smagSetInf (prob, OSINFINITY);
  smagSetObjFlavor (prob, OBJ_FUNCTION);
  smagSetSqueezeFreeRows (prob, 1);	/* don't show me =n= rows */
  smagReadModel (prob);

#ifdef GAMS_BUILD
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/CoinOS (OS Library 1.0)\nwritten by Jun Ma, Kipp Martin, ...\n");
#else
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/OS (OS Library 1.0)\nwritten by Jun Ma, Kipp Martin, ...\n");
#endif
	smagStdOutputFlush(prob, SMAG_ALLMASK);

	Smag2OSiL smagosil(prob);
	if (!smagosil.createOSInstance()) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Creation of OSInstance failed.\n");
		return EXIT_FAILURE;
	}
	
	OSiLWriter osilwriter;
	smagStdOutputPrint(prob, SMAG_LOGMASK, "Writing the instance...\n");
	smagStdOutputPrint(prob, SMAG_LOGMASK, osilwriter.writeOSiL(smagosil.osinstance).c_str());
	smagStdOutputPrint(prob, SMAG_LOGMASK, "Done writing the instance.\n");

	
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);

  return EXIT_SUCCESS;
} // main
