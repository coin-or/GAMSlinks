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
		gevLogStat(gev, osresult.getGeneralMessage().c_str());
		return;
	} else if (osresult.getGeneralStatusType() == "warning") {
		gevLogStatPChar(gev, "Warning: OS result reports warning: ");
		gevLogStat(gev, osresult.getGeneralMessage().c_str());
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

#define SMAG_DBL_NA -1E20  // there is a gmoNAinteger, but no gmoNAdouble
	int* colBasStat = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)Bstat_Super);
	int* colIndic   = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)Cstat_OK);
	double* colMarg = CoinCopyOfArray((double*)NULL, gmoN(gmo), SMAG_DBL_NA);
	double* colLev  = CoinCopyOfArray((double*)NULL, gmoN(gmo), SMAG_DBL_NA);

	int* rowBasStat = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)Bstat_Super);
	int* rowIndic   = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)Cstat_OK);
	double* rowLev  = CoinCopyOfArray((double*)NULL, gmoM(gmo), SMAG_DBL_NA);
	double* rowMarg = CoinCopyOfArray((double*)NULL, gmoM(gmo), SMAG_DBL_NA);

	//TODO
//	if (sol->constraints && sol->constraints->values) // set row levels, if available
//		for (std::vector<ConValue*>::iterator it(sol->constraints->values->con.begin());
//		it!=sol->constraints->values->con.end(); ++it)
//			rowLev[(*it)->idx]=(*it)->value;
	if (sol->constraints && sol->constraints->dualValues) // set row dual values, if available
		for (std::vector<DualVarValue*>::iterator it(sol->constraints->dualValues->con.begin());
		it!=sol->constraints->dualValues->con.end(); ++it)
			rowMarg[(*it)->idx]=(*it)->value; // what are it->lbValue and it->ubValue ?
	if (sol->variables && sol->variables->values) // set var values, if available
		for (std::vector<VarValue*>::const_iterator it(sol->variables->values->var.begin());
		it!=sol->variables->values->var.end(); ++it)
			colLev[(*it)->idx]=(*it)->value;
	if (sol->variables)
		for (int i=0; i<sol->variables->numberOfOtherVariableResults; ++i) {
			if (sol->variables->other[i]->name=="reduced costs") {
				for (std::vector<OtherVarResult*>::const_iterator it(sol->variables->other[i]->var.begin());
				it!=sol->variables->other[i]->var.end(); ++it)
					colMarg[(*it)->idx]=atof((*it)->value.c_str());
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
		gevLogStat(gev, error.errormsg.c_str());
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		return;
	}
	if (osresult)
		writeSolution(*osresult);
}
