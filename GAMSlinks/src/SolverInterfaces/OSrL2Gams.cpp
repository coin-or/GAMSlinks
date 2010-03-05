// Copyright (C) GAMS Development and others 2008-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#include "OSResult.h"
#include "OSrL2Gams.hpp"
#include "OSrLReader.h"
#include "OSErrorClass.h"
#include "CoinHelperFunctions.hpp"

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

#include "gmomcc.h"
#include "gevmcc.h"
/* value for not available/applicable */
#if GMOAPIVERSION >= 7
#define GMS_SV_NA     gmoValNA(gmo)
#else
#define GMS_SV_NA     2.0E300
#endif

static
void mygevLogStatPChar(gevRec* gev, const char* msg_) {
	int len = strlen(msg_);
	
	if (len < 250) {
		gevLogStat(gev, msg_);
		return;
	}
	
	char* msg = const_cast<char*>(msg_);
	
	char tmp = msg[0];
	for (int i = 0; i < len; i+=250) {
		msg[i] = tmp;
		if (i+250 < len) {
			tmp = msg[i+250];
			msg[i+250] = 0;
		}
		gevLogStat(gev, msg+i);
	}
}

OSrL2Gams::OSrL2Gams(gmoHandle_t gmo_)
: gmo(gmo_), gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL)
{ }

void OSrL2Gams::writeSolution(OSResult& osresult) {
	if (osresult.general == NULL) {
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gevLogStat(gev, "Error: OS result does not have header.");
		return;
	} else if (osresult.getGeneralStatusType() == "error") {
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gevLogStatPChar(gev, "Error: OS result reports error: ");
		mygevLogStatPChar(gev, osresult.getGeneralMessage().c_str());
		gevLogStat(gev, "");
		return;
	} else if (osresult.getGeneralStatusType() == "warning") {
		gevLogStatPChar(gev, "Warning: OS result reports warning: ");
		mygevLogStatPChar(gev, osresult.getGeneralMessage().c_str());
		gevLogStat(gev, "");
	}

	gmoSolveStatSet(gmo, SolveStat_Normal);

	if (osresult.getSolutionNumber() == 0) {
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
	} else if (osresult.getSolutionStatusType(0) == "unbounded") {
		gmoModelStatSet(gmo, ModelStat_Unbounded);
	} else if (osresult.getSolutionStatusType(0) == "globallyOptimal") {
		gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
	} else if (osresult.getSolutionStatusType(0) == "locallyOptimal") {
		gmoModelStatSet(gmo, ModelStat_OptimalLocal);
	} else if (osresult.getSolutionStatusType(0) == "optimal") {
		gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
	} else if (osresult.getSolutionStatusType(0) == "bestSoFar") {
		gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);	// or should we report integer solution if integer var.?
	} else if (osresult.getSolutionStatusType(0) == "feasible") {
		gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);	// or should we report integer solution if integer var.?
	} else if (osresult.getSolutionStatusType(0) == "infeasible") {
		gmoModelStatSet(gmo, ModelStat_InfeasibleGlobal);
	} else if (osresult.getSolutionStatusType(0) == "stoppedByLimit") {
		gmoSolveStatSet(gmo, SolveStat_Iteration); // just a guess
		gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
	} else if (osresult.getSolutionStatusType(0) == "unsure") {
		gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
	} else if (osresult.getSolutionStatusType(0) == "error") {
		gmoModelStatSet(gmo, ModelStat_ErrorUnknown);
	} else if (osresult.getSolutionStatusType(0) == "other") {
		gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
	} else {
		gmoModelStatSet(gmo, ModelStat_ErrorUnknown);
	}

	if (osresult.getVariableNumber() != gmoN(gmo)) {
		gevLogStat(gev, "Error: Number of variables in OS result does not match with gams model.");
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return;
	}
	if (osresult.getConstraintNumber() != gmoM(gmo)) {
		gevLogStat(gev, "Error: Number of constraints in OS result does not match with gams model.");
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return;
	}

	OptimizationSolution* sol = osresult.optimization->solution[0];

	int* colBasStat = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)Bstat_Super);
	int* colIndic   = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)Cstat_OK);
	double* colMarg = CoinCopyOfArray((double*)NULL, gmoN(gmo), GMS_SV_NA);
	double* colLev  = CoinCopyOfArray((double*)NULL, gmoN(gmo), GMS_SV_NA);

	int* rowBasStat = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)Bstat_Super);
	int* rowIndic   = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)Cstat_OK);
	double* rowLev  = CoinCopyOfArray((double*)NULL, gmoM(gmo), GMS_SV_NA);
	double* rowMarg = CoinCopyOfArray((double*)NULL, gmoM(gmo), GMS_SV_NA);
	//TODO
//	if (sol->constraints && sol->constraints->values) // set row levels, if available
//		for (std::vector<ConValue*>::iterator it(sol->constraints->values->con.begin());
//		it!=sol->constraints->values->con.end(); ++it)
//			rowLev[(*it)->idx]=(*it)->value;
	if (sol->constraints && sol->constraints->dualValues) // set row dual values, if available
		for (int i = 0; i < sol->constraints->dualValues->numberOfCon; ++i)
			rowMarg[sol->constraints->dualValues->con[i]->idx] = sol->constraints->dualValues->con[i]->value;
	if (sol->variables && sol->variables->values) // set var values, if available
		for (int i = 0; i < sol->variables->values->numberOfVar; ++i)
			colLev[sol->variables->values->var[i]->idx] = sol->variables->values->var[i]->value;
	if (sol->variables)
		for (int i=0; i<sol->variables->numberOfOtherVariableResults; ++i) {
			if (sol->variables->other[i]->name=="reduced costs") {
				for (int j = 0; j < sol->variables->other[i]->numberOfVar; ++j)
					colMarg[sol->variables->other[i]->var[j]->idx] = atof(sol->variables->other[i]->var[j]->value.c_str());
				break;
			}
		}
	gmoSetSolution8(gmo, colLev, colMarg, rowMarg, rowLev, colBasStat, colIndic, rowBasStat, rowIndic);

	delete[] rowLev;
	delete[] rowMarg;
	delete[] rowBasStat;
	delete[] rowIndic;
	delete[] colLev;
	delete[] colMarg;
	delete[] colBasStat;
	delete[] colIndic;

	if (sol->objectives && sol->objectives->values && sol->objectives->values->obj[0])
		gmoSetHeadnTail(gmo, HobjVal, sol->objectives->values->obj[0]->value);
}

void OSrL2Gams::writeSolution(std::string& osrl) {
	OSResult* osresult = NULL;
	OSrLReader osrl_reader;
	try {
		osresult = osrl_reader.readOSrL(osrl);
	} catch(const ErrorClass& error) {
		gevLogStat(gev, "Error parsing the OS result string:");
		mygevLogStatPChar(gev, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return;
	}
	if (osresult)
		writeSolution(*osresult);
}
