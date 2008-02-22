// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsCoinScip.cpp 337 2008-02-12 19:38:07Z stefan $
//
// Author: Stefan Vigerske

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

#ifdef HAVE_CSTDIO
#include <cstring>
#else
#ifdef HAVE_STDIO_H
#include <string.h>
#else
#error "don't have header file for stdio"
#endif
#endif

#include "smag.h"

extern "C" {
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/cons_linear.h"
}

// statistics and primal solution about mip or lp solve
typedef struct {
	int model_status;
	int solver_status;
	
	int iterations;
	int nodenum;
	double time;
	double objest;
	
	double optval;
	double* colval;
} SolveStatus;

struct SCIP_MessagehdlrData
{
	smagHandle_t smag;
};

void printWarningError(SCIP_MESSAGEHDLR* messagehdlr, FILE* file, const char* msg);
void printInfoDialog(SCIP_MESSAGEHDLR* messagehdlr, FILE* file, const char* msg);
void checkScipReturn(smagHandle_t prob, SCIP_RETCODE scipret);

SCIP_RETCODE setupMIP(smagHandle_t prob, SCIP* scip, SCIP_VAR**& vars);
SCIP_RETCODE setupMIPParameters(smagHandle_t prob, SCIP* scip);
SCIP_RETCODE checkMIPsolve(smagHandle_t prob, SCIP* scip, SCIP_VAR** vars, SolveStatus& solstatus);

SCIP_RETCODE setupLP(smagHandle_t prob, SCIP_LPI* lpi, double* colval);
SCIP_RETCODE setupLPParameters(smagHandle_t prob, SCIP_LPI* lpi);
SCIP_RETCODE checkLPsolve(smagHandle_t prob, SCIP_LPI* lpi, SolveStatus& solstatus);

SCIP_RETCODE writeSolution(smagHandle_t prob, SolveStatus& solstatus, SCIP_LPI* lpi);

int main (int argc, const char *argv[]) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif
  smagHandle_t prob;

	if (argc==1) {
		fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n",  argv[0]);
		exit(EXIT_FAILURE);
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

  sprintf(buffer, "\nGAMS/%s (SCIP library %g.%d)\nwritten by Tobias Achterberg, Timo Berthold, Thorsten Koch, Alexander Martin, Kati Wolter\n\n", 
#ifdef GAMS_BUILD
  "CoinSCIP"
#else
  "SCIP"
#endif
  , SCIPversion(), SCIPsubversion());
	smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
	smagStdOutputFlush(prob, SMAG_ALLMASK);
 
  smagReadModelStats (prob);
  smagSetObjFlavor (prob, OBJ_FUNCTION);
  smagSetSqueezeFreeRows (prob, 1);	/* don't show me =n= rows */

  if (prob->gms.nsos1 || prob->gms.nsos2) {
  	smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Special ordered sets (SOS) not supported by SCIP.\n");
  	smagStdOutputFlush(prob, SMAG_ALLMASK);
		smagReportSolBrief(prob, 13, 6); // error; capability problems
		exit(EXIT_FAILURE);
  }
  if (prob->gms.nsemi || prob->gms.nsemii) {
  	smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Semicontinuous and semiinteger variables not supported by SCIP.\n");
  	smagStdOutputFlush(prob, SMAG_ALLMASK);
		smagReportSolBrief(prob, 13, 6); // error; capability problems  	
		exit(EXIT_FAILURE);
  }
  
  SCIP_RETCODE scipret;
 
  SolveStatus solstatus;
  solstatus.colval=NULL;
  
  if (prob->gms.nbin || prob->gms.numint) { // if discrete var, do SCIP
    SCIP_MESSAGEHDLR* messagehdlr=NULL;
    SCIP_MESSAGEHDLRDATA messagehdlrdata;
    messagehdlrdata.smag=prob;
    
    scipret = SCIPcreateMessagehdlr(&messagehdlr, TRUE, printWarningError, printWarningError, printInfoDialog, printInfoDialog, &messagehdlrdata);
    checkScipReturn(prob, scipret);
  	scipret = SCIPsetMessagehdlr(messagehdlr);
  	checkScipReturn(prob, scipret);

  	// have to put this here already, because we need the value for infinity
  	SCIP* scip = NULL;
  	// initialize SCIP
  	scipret = SCIPcreate(&scip);
  	checkScipReturn(prob, scipret);

  	smagSetInf (prob, SCIPinfinity(scip));
  	smagReadModel (prob);

  	/* include default SCIP plugins */
  	scipret = SCIPincludeDefaultPlugins(scip);
  	checkScipReturn(prob, scipret);

  	SCIP_VAR** mip_vars=NULL;

  	scipret = setupMIP(prob, scip, mip_vars);
  	checkScipReturn(prob, scipret);
  	
  	scipret = setupMIPParameters(prob, scip);
  	checkScipReturn(prob, scipret);

//  	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nStarting MIP solve...\n");
//  	smagStdOutputFlush(prob, SMAG_LOGMASK);

  	scipret = SCIPsolve(scip);
  	checkScipReturn(prob, scipret);
  	//  SCIP_CALL( SCIPprintStatistics(scip, NULL) );

  	scipret = checkMIPsolve(prob, scip, mip_vars, solstatus);
  	checkScipReturn(prob, scipret);

  	delete[] mip_vars;

    scipret = SCIPfree(&scip);
  	checkScipReturn(prob, scipret);

  	scipret = SCIPfreeMessagehdlr(&messagehdlr);
  	checkScipReturn(prob, scipret);
  }
  
	SCIP_LPI* lpi=NULL;
	double lpsolve_starttime=smagGetCPUTime(prob);
  if ((!prob->gms.nbin && !prob->gms.numint) || solstatus.colval) { // if we have an LP or got a mip feasible point, solve (fixed) LP
  	scipret = SCIPlpiCreate(&lpi, "gamsproblem", smagMinim(prob)==-1 ? SCIP_OBJSEN_MAXIMIZE : SCIP_OBJSEN_MINIMIZE);
  	checkScipReturn(prob, scipret);

  	if (!prob->gms.nbin && !prob->gms.numint) {
  		smagSetInf (prob, SCIPlpiInfinity(lpi));
  		smagReadModel (prob);
  	}
  	
  	scipret = setupLP(prob, lpi, solstatus.colval);
  	checkScipReturn(prob, scipret);

  	if (!prob->gms.nbin && !prob->gms.numint) {
  		scipret = setupLPParameters(prob, lpi);
    	checkScipReturn(prob, scipret);
    	smagStdOutputPrint(prob, SMAG_LOGMASK, "Starting LP solve...\n");
  	} else {
    	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolving LP with fixed discrete variables...\n");
  	}
  	smagStdOutputFlush(prob, SMAG_LOGMASK);

  	scipret = SCIPlpiSolveDual(lpi);
  	checkScipReturn(prob, scipret);
  }

	// if problem was an LP, get gams status and statistics
	if (!prob->gms.nbin && !prob->gms.numint) {
		solstatus.time=smagGetCPUTime(prob)-lpsolve_starttime;
		scipret = checkLPsolve(prob, lpi, solstatus);
  	checkScipReturn(prob, scipret);
	}
  
  scipret = writeSolution(prob, solstatus, lpi);
	checkScipReturn(prob, scipret);

  if (lpi) {
  	scipret = SCIPlpiFree(&lpi);
  	checkScipReturn(prob, scipret);
  }
  
  delete[] solstatus.colval;
  
  BMScheckEmptyMemory();
  
	smagStdOutputPrint(prob, SMAG_ALLMASK, "GAMS/SCIP finished.\n");
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);
	smagCloseLog(prob);

  return EXIT_SUCCESS;
}

SCIP_RETCODE setupMIP(smagHandle_t prob, SCIP* scip, SCIP_VAR**& vars) {
	SCIP_CALL( SCIPcreateProb(scip, "gamsmodel", NULL, NULL, NULL, NULL, NULL, NULL) );
	
	vars=new SCIP_VAR*[smagColCount(prob)];
	
	double minprior,maxprior;
	if (prob->gms.priots && smagColCount(prob)>0) { // compute range of given priorities
		minprior=prob->colPriority[0];
		maxprior=prob->colPriority[0];
		for (int i=0; i<smagColCount(prob); ++i)
			if (prob->colPriority[i]<minprior) minprior=prob->colPriority[i];
			else if (prob->colPriority[i]>maxprior) maxprior=prob->colPriority[i];
	}

	smagObjGradRec_t* og = prob->objGrad;
	for (int i=0; i<smagColCount(prob); ++i) {
		SCIP_VARTYPE vartype;
		switch (prob->colType[i]) {
			case SMAG_VAR_CONT:
				vartype=SCIP_VARTYPE_CONTINUOUS;
				break;
			case SMAG_VAR_BINARY:
				vartype=SCIP_VARTYPE_BINARY;
				break;
			case SMAG_VAR_INTEGER:
				vartype=SCIP_VARTYPE_INTEGER;
				break;
			default : {
				smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: SOS, semicontinuous, and semiinteger variables not supported.\n"); 			
	      smagStdOutputFlush(prob, SMAG_ALLMASK);
				return SCIP_READERROR;
			}
		}
		double obj_coeff=0.;
		if (og && og->j==i) {
			obj_coeff=og->dfdj;
			og=og->next;
		}
		//TODO: variable names
		SCIP_CALL( SCIPcreateVar(scip, vars+i, NULL /*varname*/, prob->colLB[i], prob->colUB[i], obj_coeff, vartype, TRUE, FALSE, NULL, NULL, NULL, NULL) );
		SCIP_CALL( SCIPaddVar(scip, vars[i]) );

		if (prob->gms.priots && minprior<maxprior) {
			// in GAMS: higher priorities are given by smaller .prior values
			// in SCIP: variables with higher branch priority are always preferred to variables with lower priority in selection of branching variable
			// thus, we scale the values from GAMS to lie between 0 (lowest prior) and 1000 (highest prior) 
			int branchpriority = (int)(1000./(maxprior-minprior)*(maxprior-prob->colPriority[i]));		
			SCIP_CALL( SCIPchgVarBranchPriority(scip, vars[i], branchpriority) );
		}
	}
	
	// overestimate on max. nr. of nonzero in a row (prob->gms.maxcol does not seem to work)
	SCIP_VAR** con_vars=new SCIP_VAR*[smagColCount(prob)];
	SCIP_Real* con_coef=new SCIP_Real[smagColCount(prob)];
	for (int i=0; i<smagRowCount(prob); ++i) {
		double lb,ub;
		switch (prob->rowType[i]) {
			case SMAG_EQU_EQ:
				lb = ub = prob->rowRHS[i];
				break;
			case SMAG_EQU_LT:
				lb = -prob->inf;
				ub =  prob->rowRHS[i];
				break;
			case SMAG_EQU_GT:
				lb = prob->rowRHS[i];
				ub = prob->inf;
				break;
			default:
				smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Unknown SMAG row type. Exiting ...\n");
				smagStdOutputFlush(prob, SMAG_ALLMASK);
				return SCIP_READERROR;
		}

		int ncoef=0;
    for (smagConGradRec_t* cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next, ++ncoef) {
    	con_coef[ncoef]=cGrad->dcdj;
    	con_vars[ncoef]=vars[cGrad->j];
    }
		
    SCIP_CONS* con;

		//TODO: constraint names
		SCIP_CALL( SCIPcreateConsLinear(scip, &con, "gamscon" /*conname*/, ncoef, con_vars, con_coef, lb, ub,
				TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE) );
		
		SCIP_CALL( SCIPaddCons(scip, con) );
		
//		SCIP_CALL( SCIPprintCons(scip, con, NULL) );

		SCIP_CALL( SCIPreleaseCons(scip, &con) );

	}

	if (smagMinim(prob)==-1) {
		SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );
	}
	
	delete[] con_vars;
	delete[] con_coef;
	
	return SCIP_OKAY;
}

SCIP_RETCODE setupMIPParameters(smagHandle_t prob, SCIP* scip) {
	SCIP_CALL( SCIPsetLongintParam(scip, "limits/nodes", prob->gms.nodlim ? prob->gms.nodlim : prob->gms.itnlim) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/time", prob->gms.reslim) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/gap", prob->gms.optcr) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/absgap", prob->gms.optca) );
	
	//TODO: cutoff
	
  if (prob->gms.useopt)
  	SCIP_CALL( SCIPreadParams(scip, prob->gms.optFileName) );

	return SCIP_OKAY;
}

SCIP_RETCODE checkMIPsolve(smagHandle_t prob, SCIP* scip, SCIP_VAR** vars, SolveStatus& solstatus) {
//	SCIP_CALL( SCIPprintStatus(scip, NULL) );

	SCIP_STATUS status = SCIPgetStatus(scip);
	int nrsol = SCIPgetNSols(scip);

	solstatus.model_status=13;
	solstatus.solver_status=13;

//	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolving MIP finished.\nSolution status: ");

	switch(status) {
		default:
		case SCIP_STATUS_UNKNOWN: // the solving status is not yet known
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Unknown.\n");
			break;
		case SCIP_STATUS_USERINTERRUPT: // the user interrupted the solving process (by pressing Ctrl-C)
			solstatus.solver_status = 8; // user interrupt
			solstatus.model_status = nrsol ? 8 : 6; // integer solution or intermediate infeasible
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "User interrupt.\n");
			break;
		case SCIP_STATUS_NODELIMIT: // the solving process was interrupted because the node limit was reached
		case SCIP_STATUS_STALLNODELIMIT: // the solving process was interrupted because the node limit was reached
			solstatus.solver_status = 2; // lets call it iteration interrupt
			solstatus.model_status = nrsol ? 8 : 6; // integer solution or intermediate infeasible
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Node limit exceeded.\n");
			break;
		case SCIP_STATUS_TIMELIMIT: // the solving process was interrupted because the time limit was reached
			solstatus.solver_status = 3; // resource interrupt
			solstatus.model_status = nrsol ? 8 : 6; // integer solution or intermediate infeasible
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Time limit exceeded.\n");
			break;
		case SCIP_STATUS_MEMLIMIT: // the solving process was interrupted because the memory limit was reached
			solstatus.solver_status = 3; // resource interrupt
			solstatus.model_status = nrsol ? 8 : 6; // integer solution or intermediate infeasible
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Memory limit exceeded.\n");
			break;
		case SCIP_STATUS_GAPLIMIT: // the solving process was interrupted because the gap limit was reached
			solstatus.solver_status = 1; // normal completion
			solstatus.model_status = nrsol ? ((SCIPgetGap(scip)>0) ? 8 : 1) : 6;
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Optimality gap below tolerance.\n");
			break;
		case SCIP_STATUS_SOLLIMIT: // the solving process was interrupted because the solution limit was reached
		case SCIP_STATUS_BESTSOLLIMIT: // the solving process was interrupted because the solution improvement limit was reached
			solstatus.solver_status = 3; // resource interrupt
			solstatus.model_status = nrsol ? 8 : 6;
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Solutions limit exceeded.\n");
			break;
		case SCIP_STATUS_OPTIMAL: // the problem was solved to optimality, an optimal solution is available
			solstatus.solver_status = 1; // normal completion
			solstatus.model_status = 1; // optimal
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem solved to optimality.\n");
			break;
		case SCIP_STATUS_INFEASIBLE: // the problem was proven to be infeasible
			solstatus.solver_status = 1; // normal completion
			solstatus.model_status = 19; // infeasible - no sol
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem is infeasible.\n");
			break;
		case SCIP_STATUS_UNBOUNDED: // the problem was proven to be unbounded
			solstatus.solver_status = 1; // normal completion
			solstatus.model_status = nrsol ? 3 : 18; // unbounded or unbounded - no sol
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem in unbounded.\n");
			break;
		case SCIP_STATUS_INFORUNBD: // the problem was proven to be either infeasible or unbounded
			solstatus.solver_status = 1; // normal completion
			solstatus.model_status = 14; // no solution
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem is either infeasible or unbounded.\n");
			break;
	}

	solstatus.objest=SCIPgetDualbound(scip);
	solstatus.nodenum=SCIPgetNNodes(scip);
	solstatus.iterations=solstatus.nodenum;
	solstatus.time=SCIPgetTotalTime(scip);

	if (nrsol) {
//  	SCIP_CALL( SCIPprintBestSol(scip, NULL, FALSE) );
		SCIP_SOL* sol=SCIPgetBestSol(scip);
		solstatus.colval = new double[smagColCount(prob)];
//		printf("nr vars in SCIP: %d\n", SCIPgetNVars(scip));
		assert(SCIPgetNVars(scip)==smagColCount(prob));
		for (int i=0; i<smagColCount(prob); ++i) {
//			printf("value variable %d is %g\n", i, SCIPgetSolVal(scip, sol, vars[i]));
			solstatus.colval[i]=SCIPgetSolVal(scip, sol, vars[i]);
		}
		solstatus.optval=SCIPgetPrimalbound(scip);
//		char buffer[1024];
//		snprintf(buffer, 1024, "Objective function value of best feasible point: %g\nBest possible: %g\nAbsolute gap: %g\nRelative gap: %g\n", solstatus.optval, solstatus.objest, fabs(solstatus.optval-solstatus.objest), SCIPgetGap(scip));
//		smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
	} else {
		solstatus.optval=prob->gms.valna;
	}
//	smagStdOutputFlush(prob, SMAG_LOGMASK);
	
	return SCIP_OKAY;
}


SCIP_RETCODE setupLP(smagHandle_t prob, SCIP_LPI* lpi, double* colval) {
	int ncols=smagColCount(prob);
	
	double* objcoef=new double[ncols];
	memset(objcoef, 0, ncols*sizeof(double));
	for (smagObjGradRec_t* og = prob->objGrad; og; og=og->next)
		objcoef[og->j]=og->dfdj;
	
	int* beg=new int[ncols+1]; // dummy as workaround of scip bug
	memset(beg, 0, ncols*sizeof(int));
	
	double* collb=NULL;
	double* colub=NULL;
	if (colval) {
		collb=new double[ncols];
		colub=new double[ncols];
		memcpy(collb, prob->colLB, ncols*sizeof(double));
		memcpy(colub, prob->colUB, ncols*sizeof(double));
		for (int i=0; i<ncols; ++i)
			if (prob->colType[i]!=SMAG_VAR_CONT)
				collb[i]=colub[i]=colval[i];
	}
	
	SCIPlpiAddCols(lpi, smagColCount(prob), objcoef, collb ? collb : prob->colLB, colub ? colub : prob->colUB, NULL, 0, beg, NULL, NULL);
	
	delete[] objcoef;
	delete[] beg;
	delete[] collb;
	delete[] colub;
	
	double* values=new double[smagNZCount(prob)];
	int* rowstarts=new int[smagRowCount(prob)+1];
	int* colindexes=new int[smagNZCount(prob)];
	int j = 0;
  for (int i = 0;  i < smagRowCount(prob);  ++i) {
  	rowstarts[i]=j;
    for (smagConGradRec_t* cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next, ++j) {
    	values[j]=cGrad->dcdj;
    	colindexes[j]=cGrad->j;
    }
  }
  assert(j==smagNZCount(prob));
  rowstarts[smagRowCount(prob)]=j;
	
  SCIPlpiAddRows(lpi, smagRowCount(prob), prob->rowLB, prob->rowUB, NULL, smagNZCount(prob), rowstarts, colindexes, values);
  
  delete[] values;
  delete[] rowstarts;
  delete[] colindexes;
  
  if (ncols==0) {
  	double zero=0.;
  	int zeroint=0;
  	SCIPlpiAddCols(lpi, 1, &zero, &zero, &zero, NULL, 0, &zeroint, NULL, NULL);
  }
  
  //TODO: redirect output
	
	return SCIP_OKAY;
}

SCIP_RETCODE setupLPParameters(smagHandle_t prob, SCIP_LPI* lpi) {
	SCIPlpiSetIntpar(lpi, SCIP_LPPAR_LPINFO, TRUE);

	SCIPlpiSetIntpar(lpi, SCIP_LPPAR_LPITLIM, prob->gms.itnlim);
	SCIPlpiSetRealpar(lpi, SCIP_LPPAR_LPTILIM, prob->gms.reslim);

	// TODO: read parameter file ?

	return SCIP_OKAY;
}

SCIP_RETCODE checkLPsolve(smagHandle_t prob, SCIP_LPI* lpi, SolveStatus& solstatus) {
	SCIP_Bool primalfeasible=SCIPlpiIsPrimalFeasible(lpi);
	
	// workaround bug in scip
	primalfeasible = !primalfeasible;
	
	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolution status: ");
	
	if (SCIPlpiIsPrimalUnbounded(lpi)) {
		solstatus.solver_status = 1; // normal completion
		solstatus.model_status = primalfeasible ? 3 : 18; // unbounded with or without solution	
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem is unbounded.\n");
	} else if (SCIPlpiIsPrimalInfeasible(lpi)) {
		solstatus.solver_status = 1; // normal completion
		solstatus.model_status = 4; // infeasible
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem is infeasible.\n");
	} else if (SCIPlpiIsOptimal(lpi)) {
		solstatus.solver_status = 1; // normal completion
		solstatus.model_status = 1; // optimal
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem solved to optimality.\n");
	} else if (SCIPlpiIsIterlimExc(lpi)) {
		solstatus.solver_status = 2; // iteration interrupt
		solstatus.model_status = primalfeasible ? 7 : 6; // intermediate nonopt. or infeas. 
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Iteration limit exceeded.\n");
	} else if (SCIPlpiIsTimelimExc(lpi)) {
		solstatus.solver_status = 3; // resource interrupt
		solstatus.model_status = primalfeasible ? 7 : 6; // intermediate nonopt. or infeas.
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Time limit exceeded.\n");
	} else if (SCIPlpiIsObjlimExc(lpi)) {
		solstatus.solver_status = 4; // ???, terminated by solver
		solstatus.model_status = primalfeasible ? 7 : 6; // intermediate nonopt. or infeas.
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Objective limit exceeded.\n");
	} else { // don't have idea about status
		solstatus.solver_status = 13;
		solstatus.model_status = primalfeasible ? 7 : 6; // intermediate nonopt. or infeas.
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Status unknown.\n");
		if (smagColCount(prob)==0) { // claim that an empty problem was solved to optimality
			solstatus.solver_status = 1;
			solstatus.model_status = 1;
		}
	}
	
	SCIPlpiGetIterations(lpi, &solstatus.iterations);
	solstatus.nodenum=0;

	if (primalfeasible) {
		SCIP_CALL( SCIPlpiGetObjval(lpi, &solstatus.optval) );
		solstatus.objest=solstatus.optval;
		
		char buffer[256];
		snprintf(buffer, 256, "Found feasible point with objective function value %g.\n", solstatus.optval);
		smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
	}
	
	smagStdOutputFlush(prob, SMAG_LOGMASK);
	
	return SCIP_OKAY;
}

SCIP_RETCODE writeSolution(smagHandle_t prob, SolveStatus& solstatus, SCIP_LPI* lpi) {
	if (!lpi && !solstatus.colval) { // no LP solved and no MIP feasible solution
	  smagReportSolStats(prob, solstatus.model_status, solstatus.solver_status,
	  		solstatus.nodenum, solstatus.time, prob->gms.valna, 0);
	  return SCIP_OKAY;		
	}
		
	double* rowlev = new double[smagRowCount(prob)];
	double* rowmarg = new double[smagRowCount(prob)];
	unsigned char* rowbasstat = new unsigned char[smagRowCount(prob)];
	unsigned char* rowindic = new unsigned char[smagRowCount(prob)];
	double* collev = NULL;
	double* colmarg = new double[smagColCount(prob)];
	unsigned char* colbasstat = new unsigned char[smagColCount(prob)];
	unsigned char* colindic = new unsigned char[smagColCount(prob)];
	
	// workaround bug in scip: negate return of SCIPlpiIsPrimalFeasible
	if (lpi && !SCIPlpiIsPrimalFeasible(lpi)) { // read solution from lpi object
		
		collev = new double[smagColCount(prob)];
		SCIPlpiGetSol(lpi, &solstatus.optval, collev, rowmarg, rowlev, colmarg);
		
		int* cstat=new int[smagColCount(prob)];
		int* rstat=new int[smagRowCount(prob)];
		SCIPlpiGetBase(lpi, cstat, rstat);
		
		for (int i=0; i<smagColCount(prob); ++i) {
			if (prob->colType[i]!=SMAG_VAR_CONT)
				colbasstat[i] = SMAG_BASSTAT_SUPERBASIC;
			else switch(cstat[i]) {
				case SCIP_BASESTAT_LOWER: colbasstat[i] = SMAG_BASSTAT_NBLOWER; break;
				case SCIP_BASESTAT_UPPER: colbasstat[i] = SMAG_BASSTAT_NBUPPER; break;
				case SCIP_BASESTAT_BASIC: colbasstat[i] = SMAG_BASSTAT_BASIC; break;
				case SCIP_BASESTAT_ZERO:  
				default:                  colbasstat[i] = SMAG_BASSTAT_SUPERBASIC; break;
			}			
			colindic[i]=SMAG_RCINDIC_OK;
		}
		
		for (int i=0; i<smagRowCount(prob); ++i) {
			switch(rstat[i]) {
				case SCIP_BASESTAT_LOWER: rowbasstat[i] = SMAG_BASSTAT_NBLOWER; break;
				case SCIP_BASESTAT_UPPER: rowbasstat[i] = SMAG_BASSTAT_NBUPPER; break;
				case SCIP_BASESTAT_BASIC: rowbasstat[i] = SMAG_BASSTAT_BASIC; break;
				case SCIP_BASESTAT_ZERO:  
				default:                  rowbasstat[i] = SMAG_BASSTAT_SUPERBASIC; break;
			}			
			rowindic[i]=SMAG_RCINDIC_OK;
		}
		
		delete[] cstat;
		delete[] rstat;		
		
	} else { // do not have full LP solution in lpi
  	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolving fixed LP failed. Only primal solution values will be available.\n");
		memset(rowmarg, 0, smagRowCount(prob)*sizeof(double));
		for (int i=0; i<smagRowCount(prob); ++i) {
			rowbasstat[i]=SMAG_BASSTAT_SUPERBASIC;
			rowindic[i]=SMAG_RCINDIC_OK;
		}

		if (!solstatus.colval) {
  		memset(rowlev, 0, smagRowCount(prob)*sizeof(double));
			collev=new double[smagColCount(prob)];
			memset(collev, 0, smagColCount(prob)*sizeof(double));
  	} else {
  		smagEvalConFunc(prob, solstatus.colval, rowlev);
  	}
		
		for (int i=0; i<smagColCount(prob); ++i) {
			colbasstat[i]=SMAG_BASSTAT_SUPERBASIC;
			colindic[i]=SMAG_RCINDIC_OK;
		}
	}
	
	double objoffset = prob->gms.grhs[prob->gms.slplro-1] * prob->gObjFactor;
	solstatus.optval -= objoffset;
	if (solstatus.objest != prob->gms.valna)
		solstatus.objest -= objoffset;
	
  smagSetObjEst(prob, solstatus.objest);
  smagSetNodUsd(prob, solstatus.nodenum);
  smagReportSolFull (prob, solstatus.model_status, solstatus.solver_status,
		     solstatus.iterations, solstatus.time, solstatus.optval, 0,
		     rowlev, rowmarg, rowbasstat, rowindic,
		     collev ? collev : solstatus.colval, colmarg, colbasstat, colindic);

  delete[] rowlev;
  delete[] rowmarg;
  delete[] rowbasstat;
  delete[] rowindic;
  delete[] collev;
  delete[] colmarg;
  delete[] colbasstat;
  delete[] colindic;
  
  return SCIP_OKAY;
}

void printWarningError(SCIP_MESSAGEHDLR* messagehdlr, FILE* file, const char* msg) {
	smagStdOutputPrint(messagehdlr->messagehdlrdata->smag, SMAG_ALLMASK, msg);
	smagStdOutputFlush(messagehdlr->messagehdlrdata->smag, SMAG_ALLMASK);
}

void printInfoDialog(SCIP_MESSAGEHDLR* messagehdlr, FILE* file, const char* msg) {
	smagStdOutputPrint(messagehdlr->messagehdlrdata->smag, SMAG_LOGMASK, msg);
}

void checkScipReturn(smagHandle_t prob, SCIP_RETCODE scipret) {
	if (scipret==SCIP_OKAY)
		return;

	char buffer[256];
	snprintf(buffer, 256, "Error %d in call of SCIP function\n", scipret);
	smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
	smagStdOutputFlush(prob, SMAG_ALLMASK);

  smagReportSolBrief(prob, 13, 13);
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);
	smagCloseLog(prob);

  exit(EXIT_FAILURE);
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
