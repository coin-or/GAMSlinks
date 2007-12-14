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
#include "ErrorClass.h"
#include "DefaultSolver.h"
#ifdef COIN_HAS_OSI
#include "CoinSolver.h"
#endif
#ifdef COIN_HAS_IPOPT
#include "IpoptSolver.h"
#endif

string getSolverName(bool isnonlinear, bool isdiscrete, smagHandle_t prob);

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
	
	bool writeosil=false; // TODO: should be a parameter that also gives the filename
	if (writeosil) {
		OSiLWriter osilwriter;
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Writing the instance...\n");
		smagStdOutputPrint(prob, SMAG_LOGMASK, osilwriter.writeOSiL(smagosil.osinstance).c_str());
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Done writing the instance.\n");
	}

	string solvername;//="ipopt"; // TODO: should be set via parameter
	if (solvername=="") { // set default solver depending on problem type and what is available
		solvername=getSolverName(
				smagosil.osinstance->getNumberOfNonlinearExpressions() || smagosil.osinstance->getNumberOfQuadraticTerms(),
				smagosil.osinstance->getNumberOfBinaryVariables() || smagosil.osinstance->getNumberOfIntegerVariables(),
				prob);
	}
	
	//TODO: setup solver only for solve on local machine; otherwise check option "service" and do a remote solve
	
	DefaultSolver* solver=NULL;
#ifdef COIN_HAS_IPOPT
	// we need to keep a smartptr-lock on an IpoptSolver object, otherwise the ipoptsolver deletes itself after solve due to a "SmartPtr<TNLP> nlp = this" in IpoptSolver::solve()
	SmartPtr<IpoptSolver> tnlp;
#endif
	if (solvername.find("ipopt")!=std::string::npos) {
#ifdef COIN_HAS_IPOPT
		tnlp=new IpoptSolver();
		solver=GetRawPtr(tnlp);
#else
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Ipopt not available.\n");
    smagStdOutputFlush(prob, SMAG_ALLMASK);
	  smagReportSolBrief(prob, 13, 6);
    exit (EXIT_FAILURE);
#endif
	} else {
#ifdef COIN_HAS_OSI
		solver=new CoinSolver();
#else
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: CoinSolver not available.\n");
    smagStdOutputFlush(prob, SMAG_ALLMASK);
	  smagReportSolBrief(prob, 13, 6);
    exit (EXIT_FAILURE);
#endif		
	}
	
	solver->sSolverName = solvername;
	solver->osinstance=smagosil.osinstance;
	
	//TODO: setup options
	
	smagStdOutputPrint(prob, SMAG_ALLMASK, "Solving the instance...\n\n");
	try {
		solver->solve();
		smagStdOutputPrint(prob, SMAG_ALLMASK, "\nDone solving the instance.\n");		

		bool writeosrl=false; // TODO: should be a parameter and go into a file
		if (writeosrl)
			smagStdOutputPrint(prob, SMAG_LOGMASK, solver->osrl.c_str());
		
		//TODO: write gams solution file
	
	} catch(ErrorClass error) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error solving the instance. Error message:\n");
		smagStdOutputPrint(prob, SMAG_ALLMASK, error.errormsg.c_str());
	  smagReportSolBrief(prob, 13, 13);
	}
	
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);

  return EXIT_SUCCESS;
} // main

string getSolverName(bool isnonlinear, bool isdiscrete, smagHandle_t prob) {
	if (isnonlinear) { // (MI)NLP
		if (isdiscrete) { // MINLP
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: No MINLP solver with OS interface available.\n");
		} else { // NLP
#ifdef COIN_HAS_IPOPT
			return "ipopt";
#endif
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: No NLP solver with OS interface available.\n");
      smagStdOutputFlush(prob, SMAG_ALLMASK);
		  smagReportSolBrief(prob, 13, 6);
	    exit (EXIT_FAILURE);
		}
	} else if (isdiscrete) { // MIP
#ifdef COIN_HAS_CBC
		return "cbc";
#endif
#ifdef COIN_HAS_SYMPHONY
		return "symphony";
#endif
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: No MIP solver with OS interface available.\n");
    smagStdOutputFlush(prob, SMAG_ALLMASK);
	  smagReportSolBrief(prob, 13, 6);
    exit (EXIT_FAILURE);
	} else { // LP
#ifdef COIN_HAS_CLP
		return "clp";
#endif
#ifdef COIN_HAS_DYLP
		return "dylp";
#endif
#ifdef COIN_HAS_VOL
		return "vol";
#endif
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: No LP solver with OS interface available.\n");
    smagStdOutputFlush(prob, SMAG_ALLMASK);
	  smagReportSolBrief(prob, 13, 6);
    exit (EXIT_FAILURE);
	}

	return "error"; // should never reach this point of code
}
