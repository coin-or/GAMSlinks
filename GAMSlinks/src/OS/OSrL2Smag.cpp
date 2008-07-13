// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#include "OSrL2Smag.hpp"
#include "OSrLReader.h"
#include "OSErrorClass.h"
#include "CoinHelperFunctions.hpp"

#include <cstring>

OSrL2Smag::OSrL2Smag(smagHandle_t smag_)
: smag(smag_)
{ }

OSrL2Smag::~OSrL2Smag() {
}

void OSrL2Smag::writeSolution(OSResult& osresult) {
	int solver_status, model_status;
	if (osresult.resultHeader == NULL) {
		solver_status=10; // error solver failure
		model_status=13; // error no solution
		smagReportSolBrief(smag, model_status, solver_status);
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error: OS result does not have header.\n");
		return;
	} else if (osresult.getGeneralStatusType()=="error") {
		solver_status=10; // error solver failure
		model_status=13; // error no solution
		smagReportSolBrief(smag, model_status, solver_status);
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error: OS result reports error: ");
		smagStdOutputPrint(smag, SMAG_ALLMASK, osresult.getGeneralStatusDescription().c_str());
		return;
	} else if (osresult.getGeneralStatusType()=="warning") {
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Warning: OS result reports warning: ");
		smagStdOutputPrint(smag, SMAG_ALLMASK, osresult.getGeneralStatusDescription().c_str());
	}

	solver_status=1; // normal completion

	if (osresult.getSolutionNumber()==0) {
		model_status=14; // no solution returned
	} else if (osresult.getSolutionStatusType(0)=="unbounded") {
		model_status=3;	// unbounded
	} else if (osresult.getSolutionStatusType(0)=="globallyOptimal") {
		model_status=1;	// optimal
	} else if (osresult.getSolutionStatusType(0)=="locallyOptimal") {
		model_status=2;	// locally optimal
	} else if (osresult.getSolutionStatusType(0)=="optimal") {
		model_status=2;	// locally optimal
	} else if (osresult.getSolutionStatusType(0)=="bestSoFar") {
		model_status=7;	// intermediate nonoptimal (or should we report integer solution if integer var.?)
	} else if (osresult.getSolutionStatusType(0)=="feasible") {
		model_status=3;	// intermediate nonoptimal
	} else if (osresult.getSolutionStatusType(0)=="infeasible") {
		model_status=4;	// infeasible
	} else if (osresult.getSolutionStatusType(0)=="stoppedByLimit") {
		solver_status=2; // interation interrupt (just a guess)
		model_status=6;	// intermediate infeasible
	} else if (osresult.getSolutionStatusType(0)=="unsure") {
		model_status=6;	// intermediate infeasible
	} else if (osresult.getSolutionStatusType(0)=="error") {
		model_status=12;	// error unknown
	} else if (osresult.getSolutionStatusType(0)=="other") {
		model_status=6;	// intermediate infeasible
	} else {
		model_status=12;	// error unknown
	}

	if (osresult.getVariableNumber()!=smagColCount(smag)) {
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error: Number of variables in OS result does not match with gams model.\n");
		smagStdOutputFlush(smag, SMAG_ALLMASK);
		smagReportSolBrief(smag, 13, 13);
		return;
	}
	if (osresult.getConstraintNumber()!=smagRowCount(smag)) {
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error: Number of constraints in OS result does not match with gams model.\n");
		smagStdOutputFlush(smag, SMAG_ALLMASK);
		smagReportSolBrief(smag, 13, 13);
		return;
	}

	OptimizationSolution* sol=osresult.resultData->optimization->solution[0];

	unsigned char* colBasStat=CoinCopyOfArray((unsigned char*)NULL, smagColCount(smag), (unsigned char)SMAG_BASSTAT_SUPERBASIC);
	unsigned char* colIndic=CoinCopyOfArray((unsigned char*)NULL, smagColCount(smag), (unsigned char)SMAG_RCINDIC_OK);
	double* colMarg=CoinCopyOfArray((double*)NULL, smagColCount(smag), SMAG_DBL_NA);
	double* colLev=CoinCopyOfArray((double*)NULL, smagColCount(smag), SMAG_DBL_NA);

	unsigned char* rowBasStat=CoinCopyOfArray((unsigned char*)NULL, smagRowCount(smag), (unsigned char)SMAG_BASSTAT_SUPERBASIC);
	unsigned char* rowIndic=CoinCopyOfArray((unsigned char*)NULL, smagRowCount(smag), (unsigned char)SMAG_RCINDIC_OK);
	double* rowLev=CoinCopyOfArray((double*)NULL, smagRowCount(smag), SMAG_DBL_NA);
	double* rowMarg=CoinCopyOfArray((double*)NULL, smagRowCount(smag), SMAG_DBL_NA);
	
	//TODO: add some checks that we do not write over the length of our arrays
	
	if (sol->constraints && sol->constraints->values) // set row levels, if available
		for (std::vector<ConValue*>::iterator it(sol->constraints->values->con.begin());
		it!=sol->constraints->values->con.end(); ++it) {
			rowLev[(*it)->idx]=(*it)->value;
		}
	if (sol->constraints && sol->constraints->dualValues) // set row dual values, if available
		for (std::vector<DualVarValue*>::iterator it(sol->constraints->dualValues->con.begin());
		it!=sol->constraints->dualValues->con.end(); ++it) {
			rowMarg[(*it)->idx]=(*it)->value; // what are it->lbValue and it->ubValue ?
		}
	if (sol->variables && sol->variables->values) // set var values, if available
		for (std::vector<VarValue*>::const_iterator it(sol->variables->values->var.begin());
		it!=sol->variables->values->var.end(); ++it) {
			colLev[(*it)->idx]=(*it)->value;
		}
	if (sol->variables)
		for (int i=0; i<sol->variables->numberOfOtherVariableResult; ++i) {
			if (sol->variables->other[i]->name=="reduced costs") {
				for (std::vector<OtherVarResult*>::const_iterator it(sol->variables->other[i]->var.begin());
				it!=sol->variables->other[i]->var.end(); ++it) {
					colMarg[(*it)->idx]=atof((*it)->value.c_str());
				}
				break;
			}
		}

	double objvalue=SMAG_DBL_NA;
	if (sol->objectives && sol->objectives->values && sol->objectives->values->obj[0])
		objvalue=sol->objectives->values->obj[0]->value;
	
	smagReportSolFull(smag, model_status, solver_status,
			SMAG_INT_NA /*iter used*/, SMAG_DBL_NA /*time used*/,			
  		objvalue /*obj func value*/,
  		SMAG_INT_NA /*dom errors*/,
  		rowLev, rowMarg, rowBasStat, rowIndic,
		  colLev, colMarg, colBasStat, colIndic);
	
	delete[] rowLev;
	delete[] rowMarg;
	delete[] rowBasStat;
	delete[] rowIndic;
	delete[] colLev;
	delete[] colMarg;
	delete[] colBasStat;
	delete[] colIndic;
}

void OSrL2Smag::writeSolution(std::string& osrl) {
	OSResult* osresult=NULL;
	OSrLReader osrl_reader;
	try {
		osresult=osrl_reader.readOSrL(osrl);
	} catch(const ErrorClass& error) {
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error parsing the OS result string:\n");
		smagStdOutputPrint(smag, SMAG_ALLMASK, error.errormsg.c_str());
		smagStdOutputFlush(smag, SMAG_ALLMASK);
		smagReportSolBrief(smag, 13, 13);
		return;
	}
	if (osresult)
		writeSolution(*osresult);
}


// SOLVER STATUS CODE  	DESCRIPTION
// 1 	Normal Completion
// 2 	Iteration Interrupt
// 3 	Resource Interrupt
// 4 	Terminated by Solver
// 5 	Evaluation Error Limit
// 6 	Capability Problems
// 7 	Licensing Problems
// 8 	User Interrupt
// 9 	Error Setup Failure
// 10 	Error Solver Failure
// 11 	Error Internal Solver Error
// 12 	Solve Processing Skipped
// 13 	Error System Failure
// MODEL STATUS CODE  	DESCRIPTION
// 1 	Optimal
// 2 	Locally Optimal
// 3 	Unbounded
// 4 	Infeasible
// 5 	Locally Infeasible
// 6 	Intermediate Infeasible
// 7 	Intermediate Nonoptimal
// 8 	Integer Solution
// 9 	Intermediate Non-Integer
// 10 	Integer Infeasible
// 11 	Licensing Problems - No Solution
// 12 	Error Unknown
// 13 	Error No Solution
// 14 	No Solution Returned
// 15 	Solved Unique
// 16 	Solved
// 17 	Solved Singular
// 18 	Unbounded - No Solution
// 19 	Infeasible - No Solution
