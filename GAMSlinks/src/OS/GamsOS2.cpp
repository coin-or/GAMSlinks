// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOS2.hpp"

#include "OSiLWriter.h"
#include "OSrLWriter.h"
#include "OSErrorClass.h"
#include "OSSolverAgent.h"
#ifdef COIN_OS_SOLVER
#include "OSDefaultSolver.h"
#ifdef GMODEVELOP
#include "GMOSolver.hpp"
#endif
#ifdef COIN_HAS_OSI
#include "OSCoinSolver.h"
#endif
#ifdef COIN_HAS_IPOPT
#include "OSIpoptSolver.h"
#endif
#ifdef COIN_HAS_IPOPT
#include "OSBonminSolver.h"
#endif
#endif

#include "GMO2OSiL.hpp"
#include "OSrL2GMO.hpp"

#include <fstream>
#include <iostream>

GamsOS::GamsOS(gmoHandle_t gmo_)
: gmo(gmo_), gamshandler(gmo_),
#ifdef GAMS_BUILD
  gamsopt(gamshandler, "coinos")
#else
  gamsopt(gamshandler, "os")
#endif
{ }

bool GamsOS::execute() {
	char buffer[255];
	
	gmoMinfSet(gmo, -OSDBL_MAX);
	gmoPinfSet(gmo,  OSDBL_MAX);
	gmoObjReformSet(gmo, 1);
	gmoObjStyleSet(gmo, ObjType_Fun);
  gmoIndexBaseSet(gmo, 0);

	if (gmoOptFile(gmo)) {
		char optfilename[256];
		gmoGetOptFile(gmo, optfilename);
		gamsopt.readOptionsFile(optfilename);
	}

	GMO2OSiL gmoosil(gmo);
	if (!gmoosil.createOSInstance()) {
		gmoLogStat(gmo, "Creation of OSInstance failed.");
		return false;
	}
	
	if (gamsopt.isDefined("writeosil")) {
		OSiLWriter osilwriter;
		char osilfilename[255];
		std::ofstream osilfile(gamsopt.getString("writeosil", osilfilename));
		if (!osilfile.good()) {
			snprintf(buffer, sizeof(buffer), "Error opening file %s for writing of instance in OSiL.", osilfilename);
			buffer[sizeof(buffer)-1]=0;
			gmoLogStat(gmo, buffer);
		} else {
			snprintf(buffer, sizeof(buffer), "Writing instance in OSiL to %s.", osilfilename);
			buffer[sizeof(buffer)-1]=0;
			gmoLogStat(gmo, buffer);
			osilfile << osilwriter.writeOSiL(gmoosil.osinstance);
		}
	}
	
	std::string osol;
	if (gamsopt.isDefined("readosol")) {
		char osolfilename[255], buffer[512];
		std::ifstream osolfile(gamsopt.getString("readosol", osolfilename));
		if (!osolfile.good()) {
			snprintf(buffer, 255, "Error opening file %s for reading solver options in OSoL.", osolfilename);
			gmoLogStat(gmo, buffer);
		} else {
			snprintf(buffer, 255, "Reading solver options in OSoL from %s.", osolfilename);
			gmoLogStat(gmo, buffer);
			std::getline(osolfile, osol, '\0');
		}
	} else { // just give an "empty" xml file
		osol = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <osol xmlns=\"os.optimizationservices.org\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"os.optimizationservices.org http://www.optimizationservices.org/schemas/OSoL.xsd\"><other> </other></osol>";
	}

	bool success;
	
	if (gamsopt.isDefined("service")) {
		success = remoteSolve(gmoosil.osinstance, osol);
	} else {
		success = localSolve(gmoosil.osinstance, osol);
	}

	return success;
}


#ifdef COIN_OS_SOLVER
std::string GamsOS::getSolverName(bool isnonlinear, bool isdiscrete) {
	if (isnonlinear) { // (MI)NLP
		if (isdiscrete) { // MINLP
#ifdef COIN_HAS_BONMIN
			return "bonmin";
#endif
			gmoLogStat(gmo, "Error: No MINLP solver with OS interface available.");
		} else { // NLP
#ifdef COIN_HAS_IPOPT
			return "ipopt";
#endif
			gmoLogStat(gmo, "Error: No NLP solver with OS interface available.");
		}
	} else if (isdiscrete) { // MIP
#ifdef COIN_HAS_CBC
		return "cbc";
#endif
#ifdef COIN_HAS_CPX
		return "cplex";
#endif
#ifdef COIN_HAS_GLPK
		return "glpk";
#endif
#ifdef COIN_HAS_SYMPHONY
		return "symphony";
#endif
		gmoLogStat(gmo, "Error: No MIP solver with OS interface available.");
	} else { // LP
#ifdef COIN_HAS_CLP
		return "clp";
#endif
#ifdef COIN_HAS_CPX
		return "cplex";
#endif
#ifdef COIN_HAS_GLPK
		return "glpk";
#endif
#ifdef COIN_HAS_DYLP
		return "dylp";
#endif
#ifdef COIN_HAS_IPOPT
		return "ipopt";
#endif
#ifdef COIN_HAS_VOL
		return "vol";
#endif
		gmoLogStat(gmo, "Error: No LP solver with OS interface available.");
	}

	return "error";
}

bool GamsOS::localSolve(OSInstance* osinstance, std::string& osol) {
	std::string solvername;
	if (gamsopt.isDefined("solver")) {
		char buffer[128];
		if (gamsopt.getString("solver", buffer))
			solvername.assign(buffer);
	} else { // set default solver depending on problem type and what is available
		solvername = getSolverName(
				osinstance->getNumberOfNonlinearExpressions() || osinstance->getNumberOfQuadraticTerms(),
				osinstance->getNumberOfBinaryVariables() || osinstance->getNumberOfIntegerVariables());
		if (solvername.find("error") != std::string::npos) {
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_Capability);
	    return false;
		}
	}

	DefaultSolver* solver=NULL;
	if (solvername.find("ipopt") != std::string::npos) {
#ifdef COIN_HAS_IPOPT
		solver = new IpoptSolver();
#else
		gmoLogStat(gmo, "Error: Ipopt not available.");
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_Capability);
		return false;
#endif
	} else if (solvername.find("bonmin") != std::string::npos) {
#ifdef COIN_HAS_BONMIN
		solver = new BonminSolver();
#else
		gmoLogStat(gmo, "Error: Bonmin not available.");
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_Capability);
		return false;
#endif
#ifdef GMODEVELOP
	}	else if (solvername.find("gmo") != std::string::npos) {
		solver = new GMOSolver();
#endif
	} else {
#ifdef COIN_HAS_OSI
		solver = new CoinSolver();
#else
		gmoLogStat(gmo, "Error: CoinSolver not available.");
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_Capability);
		return false;
#endif		
	}

	solver->sSolverName = solvername;
	solver->osinstance = osinstance;
	solver->osol = osol;
	
	gmoLogStat(gmo, "Solving the instance...\n");
	try {
		solver->buildSolverInstance();
		solver->solve();
		gmoLogStat(gmo, "\nDone solving the instance.");		

		if (!processResult(&solver->osrl, solver->osresult))
			return false;

	} catch(ErrorClass error) {
		gmoLogStat(gmo, "Error solving the instance. Error message:");
		gmoLogStat(gmo, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return false;
	}
	
	delete solver;
	return true;
} // localSolve
#else

bool GamsOS::localSolve(OSInstance* osinstance, std::string& osol) {
	gmoLogStat(gmo, "Local solve of instances not supported. You need to rebuild GamsOS with the option --enable-os-solver.");
	gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
	gmoSolveStatSet(gmo, SolveStat_Capability);
	return false;
}

#endif

bool GamsOS::remoteSolve(OSInstance* osinstance, std::string& osol) {
	char buffer[512];
	
	if (gamsopt.isDefined("solver")) { // if a solver option was specified put that into osol
		std::string::size_type iStringpos = osol.find("</osol");
		std::string solverInput = "<other name=\"os_solver\">";
		gamsopt.getString("solver", buffer);
		solverInput += buffer;
		solverInput += "</other>";
		osol.insert(iStringpos, solverInput);
//		std::clog << "OSoL: " << osol;
	}
	
	bool success = true;

	try {
		OSSolverAgent agent(gamsopt.getString("service", buffer));

		if (!gamsopt.getString("service_method", buffer)) {
			gmoLogStat(gmo, "Error reading value of parameter service_method");
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			return false;
		}

		if (strncmp(buffer, "solve", 5) == 0) {
			std::string osrl = agent.solve(OSiLWriter().writeOSiL(osinstance), osol);
			if (!processResult(&osrl, NULL))
				return false;

		} else if (strncmp(buffer, "getJobID", 8) == 0) {
			std::string jobid = agent.getJobID(osol);
			sprintf(buffer, "OS Job ID: %s", jobid.c_str());
			gmoLog(gmo, buffer);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoSolveStatSet(gmo, SolveStat_Normal);

		} else if (strncmp(buffer, "knock", 5) ==0 ) {
			std::string ospl;
			if (gamsopt.isDefined("readospl")) {
				char osplfilename[255];
				std::ifstream osplfile(gamsopt.getString("readospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening OSpL file %s for reading knock request.", osplfilename);
					gmoLogStat(gmo, buffer);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gmoSolveStatSet(gmo, SolveStat_SystemErr);
					return false;
				} else {
					snprintf(buffer, 255, "Reading knock request from OSpL file %s.", osplfilename);
					gmoLog(gmo, buffer);
					std::getline(osplfile, ospl, '\0');
				}
			} else {
				gmoLogStat(gmo, "Error: To use OS service method 'knock', a ospl file need to be given (use option 'readospl').");
				gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
				gmoSolveStatSet(gmo, SolveStat_SystemErr);
				return false;
			}
			ospl = agent.knock(ospl, osol);

			if (gamsopt.isDefined("writeospl")) {
				char osplfilename[255];
				std::ofstream osplfile(gamsopt.getString("writeospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening file %s for writing knock result in OSpL.", osplfilename);
					gmoLogStat(gmo, buffer);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gmoSolveStatSet(gmo, SolveStat_SystemErr);
					return false;
				} else {
					snprintf(buffer, 255, "Writing knock result in OSpL to %s.", osplfilename);
					gmoLog(gmo, buffer);
					osplfile << ospl;
				}
			} else {
				gmoLogStat(gmo, "Answer from knock:");
				gmoLogStat(gmo, ospl.c_str());
				gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
				gmoSolveStatSet(gmo, SolveStat_SystemErr);
			}

		} else if (strncmp(buffer, "kill", 4) == 0) {
			std::string ospl = agent.kill(osol);

			if (gamsopt.isDefined("writeospl")) {
				char osplfilename[255];
				std::ofstream osplfile(gamsopt.getString("writeospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening file %s for writing kill result in OSpL.", osplfilename);
					gmoLogStat(gmo, buffer);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gmoSolveStatSet(gmo, SolveStat_SystemErr);
					return false;
				} else {
					snprintf(buffer, 255, "Writing kill result in OSpL to %s.", osplfilename);
					gmoLog(gmo, buffer);
					osplfile << ospl;
				}
			} else {
				gmoLogStat(gmo, "Answer from kill:");
				gmoLogStat(gmo, ospl.c_str());
				gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
				gmoSolveStatSet(gmo, SolveStat_Normal);
			}

		} else if (strncmp(buffer, "send", 4)==0) {
			if (agent.send(OSiLWriter().writeOSiL(osinstance), osol)) {
				gmoLogStat(gmo, "Problem instance successfully send to OS service.");
				gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
				gmoSolveStatSet(gmo, SolveStat_Normal);
			} else {
				gmoLogStat(gmo, "There was an error sending the problem instance to the OS service.");
				gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
				gmoSolveStatSet(gmo, SolveStat_SystemErr);
			}

		} else if (strncmp(buffer, "retrieve", 8)==0) {
			std::string osrl = agent.retrieve(osol);
			success = processResult(&osrl, NULL);

		} else {
			char buffer2[512];
			snprintf(buffer2, 512, "Error: OS service method %s not known.", buffer);
			gmoLogStat(gmo, buffer2);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			return false;
		}
	} catch(ErrorClass error) {
		gmoLogStat(gmo, "Error handling the OS service. Error message:");
		gmoLogStat(gmo, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return false;
	}
	
	return success;
}

bool GamsOS::processResult(std::string* osrl, OSResult* osresult) {
	assert(osrl || osresult);
	if (gamsopt.isDefined("writeosrl")) {
		char osrlfilename[255], buffer[512];
		std::ofstream osrlfile(gamsopt.getString("writeosrl", osrlfilename));
		if (!osrlfile.good()) {
			snprintf(buffer, 255, "Error opening file %s for writing optimization results in OSrL.", osrlfilename);
			gmoLogStat(gmo, buffer);
		} else {
			snprintf(buffer, 255, "Writing result in OSrL to %s.", osrlfilename);
			gmoLogStat(gmo, buffer);
			if (osrl)
				osrlfile << *osrl;
			else
				osrlfile << OSrLWriter().writeOSrL(osresult);
		}
	}
	

	OSrL2GMO osrl2gmo(gmo);
	if (osresult)
		osrl2gmo.writeSolution(*osresult);
	else
		osrl2gmo.writeSolution(*osrl);

	return true;
}
