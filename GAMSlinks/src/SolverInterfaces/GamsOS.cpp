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

#include "GamsOSxL.hpp"

#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gmspal.h"  /* for audit line */
#endif

#include <string>
#include <fstream>
#include <iostream>

GamsOS::GamsOS()
: gmo(NULL), osinstance(NULL)
{
	strcpy(os_message, "COIN-OR Optimization Services (OS Library 2.1)\nwritten by H. Gassmann, J. Ma, and K. Martin\n");
}

GamsOS::~GamsOS() { }

int GamsOS::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	char buffer[256];
	
	gmo = gmo_;
	assert(gmo);

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1;

	gev = (gevRec*)gmoEnvironment(gmo);
	
#ifdef GAMS_BUILD
#include "coinlibdCL4svn.h"
	auditGetLine(buffer, sizeof(buffer));
	gevLogStat(gev, "");
	gevLogStat(gev, buffer);
	gevStatAudit(gev, buffer);
#endif
	
	gevLogStat(gev, "");
	gevLogStatPChar(gev, getWelcomeMessage());

	gmoMinfSet(gmo, -OSDBL_MAX);
	gmoPinfSet(gmo,  OSDBL_MAX);
	gmoObjReformSet(gmo, 1);
	gmoObjStyleSet(gmo, ObjType_Fun);
	gmoIndexBaseSet(gmo, 0);
	
	if (gmoGetVarTypeCnt(gmo, var_S1) || gmoGetVarTypeCnt(gmo, var_S2) || gmoGetVarTypeCnt(gmo, var_SC) || gmoGetVarTypeCnt(gmo, var_SI)) {
		gevLogStat(gev, "Error: Semicontinuous and semiinteger variables and special ordered sets not supported by OS.");
		gmoSolveStatSet(gmo, SolveStat_Capability);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		return 1;
  }
	
	if (gmoGetEquTypeCnt(gmo, equ_C) || gmoGetEquTypeCnt(gmo, equ_X)) {
		gevLogStat(gev, "Error: Conic constraints and external functions not supported by OS.");
		gmoSolveStatSet(gmo, SolveStat_Capability);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		return 1;
  }

	gamsopt.setGMO(gmo);
	if (opt) {
		gamsopt.setOpt(opt);
	} else {
		char buffer[1024];
		gmoNameOptFile(gmo, buffer);
#ifdef GAMS_BUILD
		gamsopt.readOptionsFile("os",  gmoOptFile(gmo) ? buffer : NULL);
#else
		gamsopt.readOptionsFile("myos", gmoOptFile(gmo) ? buffer : NULL);
#endif
	}

	GamsOSxL gmoosxl(gmo);
	try {
		if (!gmoosxl.createOSInstance()) {
			gevLogStat(gev, "Creation of OSInstance failed.");
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			return 1;
		}
	} catch(ErrorClass error) {
		gevLogStat(gev, "Error creating the instance. Error message:");
		gevLogStatPChar(gev, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return 1;
	}

	osinstance = gmoosxl.takeOverOSInstance();

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

	int retcode;

	if (gamsopt.isDefined("service"))
		retcode = !remoteSolve(osinstance, osol);
	else {
	  gevLogStat(gev, "\nNo Optimization Services server specified (option 'service').\nSkipping remote solve.");
    gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    gmoSolveStatSet(gmo, SolveStat_Normal);
    retcode = 0;
	}

	return retcode;
}

bool GamsOS::remoteSolve(OSInstance* osinstance, std::string& osol) {
	char buffer[512];

	if (gamsopt.isDefined("solver")) { // if a solver option was specified put that into osol
		gamsopt.getString("solver", buffer);
		std::string::size_type iStringpos = osol.find("</general");
		std::string solverInput;
		if (iStringpos == std::string::npos) {
			iStringpos = osol.find("</osol");
			assert(iStringpos != std::string::npos);
			solverInput = "<general><solverToInvoke>";
			solverInput += buffer;
			solverInput += "</solverToInvoke></general>";
		} else {
			solverInput = "<solverToInvoke>";
			solverInput += buffer;
			solverInput += "</solverToInvoke>";
		}
		osol.insert(iStringpos, solverInput);
//		std::clog << "OSoL: " << osol;
	}

	bool success = true;

	try {
		OSSolverAgent agent(gamsopt.getString("service", buffer));

#if 1
    std::string osrl = agent.solve(OSiLWriter().writeOSiL(osinstance), osol);
    if (!processResult(&osrl, NULL))
      return false;

#else
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
				ospl="<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
           "<ospl xmlns=\"os.optimizationservices.org\" "
           "xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:schemaLocation=\"os.optimizationservices.org http://www.optimizationservices.org/schemas/OSpL.xsd\">"
           "<processHeader>"
           "<request action=\"getAll\"/>"
           "</processHeader>"
           "<processData/>"
           "</ospl>";
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
				gevLogStatPChar(gev, ospl.c_str());
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
				gevLogStatPChar(gev, ospl.c_str());
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
#endif

	} catch(ErrorClass error) {
		gevLogStat(gev, "Error handling the OS service. Error message:");
		gevLogStatPChar(gev, error.errormsg.c_str());
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

	GamsOSxL osrl2gmo(gmo);
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
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	if (!gevGetReady(msg, sizeof(msg)))
		return 1;
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
