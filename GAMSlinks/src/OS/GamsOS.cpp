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

#include <iostream>
#include <fstream>

#include "smag.h"
#include "Smag2OSiL.hpp"
#include "OSrL2Smag.hpp"
#include "GamsHandlerSmag.hpp"
#include "GamsOptions.hpp"

#include "OSiLWriter.h"
#include "OSrLWriter.h"
#include "OSErrorClass.h"
#include "OSSolverAgent.h"
#ifdef COIN_OS_SOLVER
#include "OSDefaultSolver.h"
#ifdef COIN_HAS_OSI
#include "OSCoinSolver.h"
#endif
#ifdef COIN_HAS_IPOPT
#include "OSIpoptSolver.h"
#endif
#endif

void localSolve(smagHandle_t prob, GamsOptions& opt, OSInstance* osinstance, std::string& osol);
void remoteSolve(smagHandle_t prob, GamsOptions& opt, OSInstance* osinstance, std::string& osol);
void processResult(smagHandle_t prob, GamsOptions& opt, string* osrl, OSResult* osresult);

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
  smagSetInf (prob, OSDBL_MAX);
  smagSetObjFlavor (prob, OBJ_FUNCTION);
  smagSetSqueezeFreeRows (prob, 1);	/* don't show me =n= rows */
  smagReadModel (prob);

#ifdef GAMS_BUILD
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/CoinOS (OS Library trunk)\nwritten by Jun Ma, Kipp Martin, ...\n");
#else
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/OS (OS Library trunk)\nwritten by Jun Ma, Kipp Martin, ...\n");
#endif
	smagStdOutputFlush(prob, SMAG_ALLMASK);
	
	GamsHandlerSmag gamshandler(prob);

#ifdef GAMS_BUILD
	GamsOptions opt(gamshandler, "coinos");
#else
	GamsOptions opt(gamshandler, "os");
#endif
	if (prob->gms.useopt)
		opt.readOptionsFile(prob->gms.optFileName);

	Smag2OSiL smagosil(prob);
	if (!smagosil.createOSInstance()) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Creation of OSInstance failed.\n");
		return EXIT_FAILURE;
	}
	
	if (opt.isDefined("writeosil")) {
		OSiLWriter osilwriter;
		char osilfilename[255];
		std::ofstream osilfile(opt.getString("writeosil", osilfilename));
		if (!osilfile.good()) {
			snprintf(buffer, 255, "Error opening file %s for writing of instance in OSiL.\n", osilfilename);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		} else {
			snprintf(buffer, 255, "Writing instance in OSiL to %s.\n", osilfilename);
			smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
			osilfile << osilwriter.writeOSiL(smagosil.osinstance);
		}
	}
	
	std::string osol;
	if (opt.isDefined("readosol")) {
		char osolfilename[255], buffer[512];
		std::ifstream osolfile(opt.getString("readosol", osolfilename));
		if (!osolfile.good()) {
			snprintf(buffer, 255, "Error opening file %s for reading solver options in OSoL.\n", osolfilename);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		} else {
			snprintf(buffer, 255, "Reading solver options in OSoL from %s.\n", osolfilename);
			smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
			std::getline(osolfile, osol, '\0');
		}		
	} else { // just give an "empty" xml file
		osol = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <osol xmlns=\"os.optimizationservices.org\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"os.optimizationservices.org http://www.optimizationservices.org/schemas/OSoL.xsd\"><other> </other></osol>";
	}

	if (opt.isDefined("service")) {
		remoteSolve(prob, opt, smagosil.osinstance, osol);
	} else {
		localSolve(prob, opt, smagosil.osinstance, osol);
	}
	
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);

  return EXIT_SUCCESS;
} // main

#ifdef COIN_OS_SOLVER
std::string getSolverName(bool isnonlinear, bool isdiscrete, smagHandle_t prob) {
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

void localSolve(smagHandle_t prob, GamsOptions& opt, OSInstance* osinstance, std::string& osol) {
	std::string solvername;
	if (opt.isDefined("solver")) {
		char buffer[128];
		if (opt.getString("solver", buffer))
			solvername.assign(buffer);
	} else { // set default solver depending on problem type and what is available
		solvername=getSolverName(
				osinstance->getNumberOfNonlinearExpressions() || osinstance->getNumberOfQuadraticTerms(),
				osinstance->getNumberOfBinaryVariables() || osinstance->getNumberOfIntegerVariables(),
				prob);
	}

	DefaultSolver* solver=NULL;
	if (solvername.find("ipopt")!=std::string::npos) {
#ifdef COIN_HAS_IPOPT
		solver=new IpoptSolver();
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
	solver->osinstance = osinstance;
	solver->osol = osol;
	
	smagStdOutputPrint(prob, SMAG_ALLMASK, "Solving the instance...\n\n");
	try {
		solver->solve();
		smagStdOutputPrint(prob, SMAG_ALLMASK, "\nDone solving the instance.\n");		

		processResult(prob, opt, &solver->osrl, solver->osresult);

	} catch(ErrorClass error) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error solving the instance. Error message:\n");
		smagStdOutputPrint(prob, SMAG_ALLMASK, error.errormsg.c_str());
		smagReportSolBrief(prob, 13, 13);
	}
	
	delete solver;
} // localSolve
#else

void localSolve(smagHandle_t prob, GamsOptions& opt, OSInstance* osinstance, std::string& osol) {
	smagStdOutputPrint(prob, SMAG_ALLMASK, "Local solve of instances not supported. You need to rebuild GamsOS with the option --enable-os-solver.\n");
	smagReportSolBrief(prob, 13, 6);
}

#endif

void remoteSolve(smagHandle_t prob, GamsOptions& opt, OSInstance* osinstance, std::string& osol) {
	char buffer[512];
	
	if (opt.isDefined("solver")) { // if a solver option was specified put that into osol
		string::size_type iStringpos = osol.find("</osol");
		std::string solverInput = "<other name=\"os_solver\">";
		opt.getString("solver", buffer);
		solverInput += buffer;
		solverInput += "</other>";
		osol.insert(iStringpos, solverInput);
		std::clog << "OSoL: " << osol;
	}

	try {
		OSSolverAgent agent(opt.getString("service", buffer));

		if (!opt.getString("service_method", buffer)) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Error reading value of parameter service_method\n");
			smagReportSolBrief(prob, 13, 13);
			return;
		}

		if (strncmp(buffer, "solve", 5)==0) {
			std::string osrl = agent.solve(OSiLWriter().writeOSiL(osinstance), osol);
			processResult(prob, opt, &osrl, NULL);

		} else if (strncmp(buffer, "getJobID", 8)==0) {
			std::string jobid = agent.getJobID(osol);
			sprintf(buffer, "OS Job ID: %s\n", jobid.c_str());
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagReportSolBrief(prob, 14, 1); // no solution returned; normal completion

		} else if (strncmp(buffer, "knock", 5)==0) {
			string ospl;
			if (opt.isDefined("readospl")) {
				char osplfilename[255];
				std::ifstream osplfile(opt.getString("readospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening OSpL file %s for reading knock request.\n", osplfilename);
					smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
					smagReportSolBrief(prob, 13, 13);
					return;
				} else {
					snprintf(buffer, 255, "Reading knock request from OSpL file %s.\n", osplfilename);
					smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
					std::getline(osplfile, ospl, '\0');
				}		
			} else {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Error: To use OS service method 'knock', a ospl file need to be given (use option 'readospl').\n");
				smagReportSolBrief(prob, 13, 13);
				return;
			}
			ospl=agent.knock(ospl, osol);

			if (opt.isDefined("writeospl")) {
				char osplfilename[255];
				std::ofstream osplfile(opt.getString("writeospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening file %s for writing knock result in OSpL.\n", osplfilename);
					smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
					smagReportSolBrief(prob, 13, 13);
					return;
				} else {
					snprintf(buffer, 255, "Writing knock result in OSpL to %s.\n", osplfilename);
					smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
					osplfile << ospl;
				}
			} else {
				smagStdOutputPrint(prob, SMAG_ALLMASK, "Answer from knock:\n");
				smagStdOutputPrint(prob, SMAG_ALLMASK, ospl.c_str());
				smagReportSolBrief(prob, 14, 1); // no solution returned; normal completion
			}

		} else if (strncmp(buffer, "kill", 4)==0) {
			std::string ospl=agent.kill(osol);

			if (opt.isDefined("writeospl")) {
				char osplfilename[255];
				std::ofstream osplfile(opt.getString("writeospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening file %s for writing kill result in OSpL.\n", osplfilename);
					smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
					smagReportSolBrief(prob, 13, 13);
					return;
				} else {
					snprintf(buffer, 255, "Writing kill result in OSpL to %s.\n", osplfilename);
					smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
					osplfile << ospl;
				}
			} else {
				smagStdOutputPrint(prob, SMAG_ALLMASK, "Answer from kill:\n");
				smagStdOutputPrint(prob, SMAG_ALLMASK, ospl.c_str());
				smagReportSolBrief(prob, 14, 1); // no solution returned; normal completion
			}

		} else if (strncmp(buffer, "send", 4)==0) {
			bool success=agent.send(OSiLWriter().writeOSiL(osinstance), osol);
			if (success) {
				smagStdOutputPrint(prob, SMAG_ALLMASK, "Problem instance successfully send to OS service.\n");
				smagReportSolBrief(prob, 14, 1); // no solution returned; normal completion
			} else {
				smagStdOutputPrint(prob, SMAG_ALLMASK, "There was an error sending the problem instance to the OS service.\n");
				smagReportSolBrief(prob, 14, 13); // no solution returned; error system failure			
			}

		} else if (strncmp(buffer, "retrieve", 8)==0) {
			std::string osrl = agent.retrieve(osol);
			processResult(prob, opt, &osrl, NULL);

		} else {
			char buffer2[512];
			snprintf(buffer2, 512, "Error: OS service method %s not known.\n", buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer2);
			smagReportSolBrief(prob, 13, 13);
			return;
		}
	} catch(ErrorClass error) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error handling the OS service. Error message:\n");
		smagStdOutputPrint(prob, SMAG_ALLMASK, error.errormsg.c_str());
		smagReportSolBrief(prob, 13, 13);
	}
}

void processResult(smagHandle_t prob, GamsOptions& opt, string* osrl, OSResult* osresult) {
	assert(osrl || osresult);
	if (opt.isDefined("writeosrl")) {
		char osrlfilename[255], buffer[512];
		std::ofstream osrlfile(opt.getString("writeosrl", osrlfilename));
		if (!osrlfile.good()) {
			snprintf(buffer, 255, "Error opening file %s for writing optimization results in OSrL.\n", osrlfilename);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		} else {
			snprintf(buffer, 255, "Writing result in OSrL to %s.\n", osrlfilename);
			smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
			if (osrl)
				osrlfile << *osrl;
			else
				osrlfile << OSrLWriter().writeOSrL(osresult);
		}
	}

	OSrL2Smag osrl2smag(prob);
	if (osresult)
		osrl2smag.writeSolution(*osresult);
	else
		osrl2smag.writeSolution(*osrl);
}
