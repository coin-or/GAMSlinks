// Copyright (C) GAMS Development and others 2008-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOS.hpp"
#include "GamsOS.h"

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

#include "Gams2OSiL.hpp"
#include "OSrL2Gams.hpp"

#include "gmomcc.h"
#include "gevmcc.h"

#include <string>
#include <fstream>
#include <iostream>

GamsOS::GamsOS()
: gmo(NULL), osinstance(NULL)
{
#ifdef GAMS_BUILD
	strcpy(os_message, "GAMS/OS (OS Library 2.0)\nwritten by H. Gassmann, J. Ma, and K. Martin\n");
#else
	strcpy(os_message, "GAMS/OSD (OS Library 2.0)\nwritten by H. Gassmann, J. Ma, and K. Martin\n");
#endif
}

GamsOS::~GamsOS() { }

int GamsOS::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	gmo = gmo_;
	assert(gmo);
	char buffer[255];

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1;

	gev = (gevRec*)gmoEnvironment(gmo);

	gmoMinfSet(gmo, -OSDBL_MAX);
	gmoPinfSet(gmo,  OSDBL_MAX);
	gmoObjReformSet(gmo, 1);
	gmoObjStyleSet(gmo, ObjType_Fun);
	gmoIndexBaseSet(gmo, 0);

	gamsopt.setGMO(gmo);
	if (opt) {
		gamsopt.setOpt(opt);
	} else {
		char buffer[1024];
		gmoNameOptFile(gmo, buffer);
#ifdef GAMS_BUILD
		gamsopt.readOptionsFile("os",  gmoOptFile(gmo) ? buffer : NULL);
#else
		gamsopt.readOptionsFile("osd", gmoOptFile(gmo) ? buffer : NULL);
#endif
	}

	Gams2OSiL gmoosil(gmo);
	try {
		if (!gmoosil.createOSInstance()) {
			gevLogStat(gev, "Creation of OSInstance failed.");
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			return 1;
		}
	} catch(ErrorClass error) {
		gevLogStat(gev, "Error creating the instance. Error message:");
		gevLogStat(gev, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return 1;
	}

	osinstance = gmoosil.takeOverOSInstance();

	if (gamsopt.isDefined("writeosil")) {
		OSiLWriter osilwriter;
		char osilfilename[255];
		std::ofstream osilfile(gamsopt.getString("writeosil", osilfilename));
		if (!osilfile.good()) {
			snprintf(buffer, sizeof(buffer), "Error opening file %s for writing of instance in OSiL.", osilfilename);
			buffer[sizeof(buffer)-1]=0;
			gevLogStat(gev, buffer);
		} else {
			snprintf(buffer, sizeof(buffer), "Writing instance in OSiL to %s.", osilfilename);
			buffer[sizeof(buffer)-1]=0;
			gevLogStat(gev, buffer);
			osilfile << osilwriter.writeOSiL(osinstance); //TODO fix
		}
	}

	return 0;
}

int GamsOS::callSolver() {
   assert(osinstance != NULL);
   
	std::string osol;
	if (gamsopt.isDefined("readosol")) {
		char osolfilename[255], buffer[512];
		std::ifstream osolfile(gamsopt.getString("readosol", osolfilename));
		if (!osolfile.good()) {
			snprintf(buffer, 512, "Error opening file %s for reading solver options in OSoL.", osolfilename);
			gevLogStat(gev, buffer);
		} else {
			snprintf(buffer, 512, "Reading solver options in OSoL from %s.", osolfilename);
			gevLogStat(gev, buffer);
			std::getline(osolfile, osol, '\0');
		}
	} else { // just give an "empty" xml file
		osol = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <osol xmlns=\"os.optimizationservices.org\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"os.optimizationservices.org http://www.optimizationservices.org/schemas/OSoL.xsd\"></osol>";
	}

	bool fail;

	if (gamsopt.isDefined("service"))
		fail = !remoteSolve(osinstance, osol);
	else
		fail = !localSolve(osinstance, osol);

	return fail;
}

#ifdef COIN_OS_SOLVER
std::string GamsOS::getSolverName(bool isnonlinear, bool isdiscrete) {
	if (isnonlinear) { // (MI)NLP
		if (isdiscrete) { // MINLP
#ifdef COIN_HAS_BONMIN
			return "bonmin";
#endif
			gevLogStat(gev, "Error: No MINLP solver with OS interface available.");
		} else { // NLP
#ifdef COIN_HAS_IPOPT
			return "ipopt";
#endif
			gevLogStat(gev, "Error: No NLP solver with OS interface available.");
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
		gevLogStat(gev, "Error: No MIP solver with OS interface available.");
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
		gevLogStat(gev, "Error: No LP solver with OS interface available.");
	}

	return "error";
}

bool GamsOS::localSolve(OSInstance* osinstance, std::string& osol) {
	std::string solvername;
	if (gamsopt.isDefined("solver")) {
		char buffer[128];
		if (gamsopt.getString("solver", buffer))
			solvername.assign(buffer);
		if (solvername == "none") {
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoSolveStatSet(gmo, SolveStat_Normal);
	    return true;
		}
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
	try {
		if (solvername.find("ipopt") != std::string::npos) {
#ifdef COIN_HAS_IPOPT
			solver = new IpoptSolver();
#else
			gevLogStat(gev, "Error: Ipopt not available.");
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_Capability);
			return false;
#endif
		} else if (solvername.find("bonmin") != std::string::npos) {
#ifdef COIN_HAS_BONMIN
			solver = new BonminSolver();
#else
			gevLogStat(gev, "Error: Bonmin not available.");
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
			gevLogStat(gev, "Error: CoinSolver not available.");
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_Capability);
			return false;
#endif
		}
	} catch (ErrorClass error) {
		gevLogStat(gev, "Error creating the OS solver interface. Error message:");
		gevLogStat(gev, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return 1;
	}

	solver->sSolverName = solvername;
	solver->osinstance = osinstance;
	solver->osol = osol;

	gevLogStat(gev, "Solving the instance...\n");
	try {
		solver->buildSolverInstance();
		solver->solve();
		gevLogStat(gev, "\nDone solving the instance.");

		if (!processResult(&solver->osrl, solver->osresult))
			return false;

	} catch(ErrorClass error) {
		gevLogStat(gev, "Error solving the instance. Error message:");
		gevLogStat(gev, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return false;
	}

	delete solver;
	return true;
}
#else

bool GamsOS::localSolve(OSInstance* osinstance, std::string& osol) {
	gevLogStat(gev, "Local solve of instances not supported. You need to rebuild GamsOS with the option --enable-os-solver.");
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
			gevLogStat(gev, "Error reading value of parameter service_method");
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
			gevLog(gev, buffer);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoSolveStatSet(gmo, SolveStat_Normal);

		} else if (strncmp(buffer, "knock", 5) ==0 ) {
			std::string ospl;
			if (gamsopt.isDefined("readospl")) {
				char osplfilename[255];
				std::ifstream osplfile(gamsopt.getString("readospl", osplfilename));
				if (!osplfile.good()) {
					snprintf(buffer, 255, "Error opening OSpL file %s for reading knock request.", osplfilename);
					gevLogStat(gev, buffer);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gmoSolveStatSet(gmo, SolveStat_SystemErr);
					return false;
				} else {
					snprintf(buffer, 255, "Reading knock request from OSpL file %s.", osplfilename);
					gevLog(gev, buffer);
					std::getline(osplfile, ospl, '\0');
				}
			} else {
				gevLogStat(gev, "Error: To use OS service method 'knock', a ospl file need to be given (use option 'readospl').");
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
					gevLogStat(gev, buffer);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gmoSolveStatSet(gmo, SolveStat_SystemErr);
					return false;
				} else {
					snprintf(buffer, 255, "Writing knock result in OSpL to %s.", osplfilename);
					gevLog(gev, buffer);
					osplfile << ospl;
				}
			} else {
				gevLogStat(gev, "Answer from knock:");
				gevLogStat(gev, ospl.c_str());
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
					gevLogStat(gev, buffer);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gmoSolveStatSet(gmo, SolveStat_SystemErr);
					return false;
				} else {
					snprintf(buffer, 255, "Writing kill result in OSpL to %s.", osplfilename);
					gevLog(gev, buffer);
					osplfile << ospl;
				}
			} else {
				gevLogStat(gev, "Answer from kill:");
				gevLogStat(gev, ospl.c_str());
				gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
				gmoSolveStatSet(gmo, SolveStat_Normal);
			}

		} else if (strncmp(buffer, "send", 4)==0) {
			if (agent.send(OSiLWriter().writeOSiL(osinstance), osol)) {
				gevLogStat(gev, "Problem instance successfully send to OS service.");
				gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
				gmoSolveStatSet(gmo, SolveStat_Normal);
			} else {
				gevLogStat(gev, "There was an error sending the problem instance to the OS service.");
				gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
				gmoSolveStatSet(gmo, SolveStat_SystemErr);
			}

		} else if (strncmp(buffer, "retrieve", 8)==0) {
			std::string osrl = agent.retrieve(osol);
			success = processResult(&osrl, NULL);

		} else {
			char buffer2[512];
			snprintf(buffer2, 512, "Error: OS service method %s not known.", buffer);
			gevLogStat(gev, buffer2);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			return false;
		}
	} catch(ErrorClass error) {
		gevLogStat(gev, "Error handling the OS service. Error message:");
		gevLogStat(gev, error.errormsg.c_str());
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
			gevLogStat(gev, buffer);
		} else {
			snprintf(buffer, 255, "Writing result in OSrL to %s.", osrlfilename);
			gevLogStat(gev, buffer);
			if (osrl)
				osrlfile << *osrl;
			else
				osrlfile << OSrLWriter().writeOSrL(osresult);
		}
	}

	OSrL2Gams osrl2gmo(gmo);
	if (osresult)
		osrl2gmo.writeSolution(*osresult);
	else
		osrl2gmo.writeSolution(*osrl);

	return true;
}

DllExport GamsOS* STDCALL createNewGamsOS() {
	return new GamsOS();
}

DllExport int STDCALL os_CallSolver(os_Rec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsOS*)Cptr)->callSolver();
}

DllExport int STDCALL os_ModifyProblem(os_Rec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsOS*)Cptr)->modifyProblem();
}

DllExport int STDCALL os_HaveModifyProblem(os_Rec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsOS*)Cptr)->haveModifyProblem();
}

DllExport int STDCALL os_ReadyAPI(os_Rec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr) {
	gevHandle_t Eptr;
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	if (!gevGetReady(msg, sizeof(msg)))
		return 1;
	Eptr = (gevHandle_t) gmoEnvironment(Gptr);
	gevLogStatPChar(Eptr, ((GamsOS*)Cptr)->getWelcomeMessage());
	return ((GamsOS*)Cptr)->readyAPI(Gptr, Optr);
}

DllExport void STDCALL os_Free(os_Rec_t **Cptr) {
	assert(Cptr != NULL);
	delete (GamsOS*)*Cptr;
	*Cptr = NULL;
}

DllExport void STDCALL os_Create(os_Rec_t **Cptr, char *msgBuf, int msgBufLen) {
	assert(Cptr != NULL);
	*Cptr = (os_Rec_t*) new GamsOS();
	if (msgBufLen && msgBuf)
		msgBuf[0] = 0;
}
