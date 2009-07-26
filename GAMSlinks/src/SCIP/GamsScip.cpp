// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
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

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

#ifdef HAVE_CERRNO
#include <cerrno>
#else
#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
#error "don't have header file for errno"
#endif
#endif

#ifdef GAMS_BUILD
#include "gmspal.h"
#endif
#include "smag.h"
#include "GamsDictionary.hpp"
#include "GamsHandlerSmag.hpp"
#include "GamsMessageHandler.hpp"
extern "C" {
#include "gdxcc.h"
}

#include "CoinError.hpp"
#include "ClpSimplex.hpp" // for passing in message handler

extern "C" {
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/cons_linear.h"
#include "scip/cons_sos1.h"
#include "scip/cons_sos2.h"
}

#include "ScipBCH.hpp"

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

SCIP_RETCODE runSCIP(smagHandle_t prob);

SCIP_RETCODE setupMIP(smagHandle_t prob, GamsHandler& gamshandler, GamsDictionary& dict, SCIP* scip, SCIP_VAR**& vars);
SCIP_RETCODE setupMIPParameters(smagHandle_t prob, SCIP* scip);
SCIP_RETCODE setupMIPStart(smagHandle_t prob, SCIP* scip, SCIP_VAR**& vars);
SCIP_RETCODE checkMIPsolve(smagHandle_t prob, SCIP* scip, SCIP_VAR** vars, SolveStatus& solstatus);

SCIP_RETCODE setupLP(smagHandle_t prob, SCIP_LPI* lpi, double* colval);
SCIP_RETCODE setupLPParameters(smagHandle_t prob, SCIP_LPI* lpi);
SCIP_RETCODE checkLPsolve(smagHandle_t prob, SCIP_LPI* lpi, SolveStatus& solstatus);

SCIP_RETCODE writeSolution(smagHandle_t prob, SolveStatus& solstatus, SCIP_LPI* lpi, SCIP_Bool solve_final);

int main (int argc, const char *argv[]) {
	WindowsErrorPopupBlocker();
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

  sprintf(buffer, "\nSCIP version %d.%d.%d [LP solver: %s]\n%s\n\n", SCIPmajorVersion(), SCIPminorVersion(), SCIPtechVersion(), SCIPlpiGetSolverName(), SCIP_COPYRIGHT);
	smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
	smagStdOutputFlush(prob, SMAG_ALLMASK);
 
  smagReadModelStats (prob);

#ifdef GAMS_BUILD
#define SKIPCHECK
#include "cmagic2.h"
  {
    int isDemo, isAcademic;
    char msg[256];

    licenseRegisterGAMS(1, prob->gms.lice1);
    licenseRegisterGAMS(2, prob->gms.lice2);
    licenseRegisterGAMS(3, prob->gms.lice3);
    licenseRegisterGAMS(4, prob->gms.lice4);
    licenseRegisterGAMS(5, prob->gms.lice5);

    if (licenseCheck (prob->gms.nrows, prob->gms.ncols, prob->gms.nnz, prob->gms.nlnz, prob->gms.ndisc)) {
      if (!licenseQueryOption("GAMS","ACADEMIC", &isAcademic))
        isAcademic = 0;
      if (!isAcademic) {
        while (licenseGetMessage(msg, sizeof(msg)))
          smagStdOutputPrintLn(prob, SMAG_ALLMASK, msg);
        smagStdOutputPrint(prob, SMAG_ALLMASK, "\n*** Use of SCIP is limited to academic users.\n*** Please contact koch@zib.de to arrange for a license.\n");
        smagReportSolBrief(prob, 11, 7); // license error
        exit(EXIT_FAILURE);
      }
    }
  }
#else
/*  if (prob->gms.nrows>300 || prob->gms.ncols>300 || prob->gms.nnz>2000 || prob->gms.ndisc>50) {
    smagStdOutputPrint(prob, SMAG_ALLMASK, "\n*** Use of CoinScip limited to academic users.\n");
    smagReportSolBrief(prob, 11, 7); // license error
    exit(EXIT_FAILURE);
  }
*/
#endif
  
  smagSetObjFlavor (prob, OBJ_FUNCTION);

  if (prob->gms.nsemi || prob->gms.nsemii) {
  	smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Semicontinuous and semiinteger variables not supported by SCIP.\n");
  	smagStdOutputFlush(prob, SMAG_ALLMASK);
		smagReportSolBrief(prob, 14, 6); // no solution; capability problems
		return EXIT_SUCCESS;
  }
  
  SCIP_RETCODE scipret = runSCIP(prob);
  
	if (scipret != SCIP_OKAY) {
		snprintf(buffer, 512, "Error %d in call of SCIP function\n", scipret);
		smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		smagStdOutputFlush(prob, SMAG_ALLMASK);

		smagReportSolBrief(prob, 13, 13);
	}
  
	smagStdOutputPrint(prob, SMAG_LOGMASK, "GAMS/SCIP finished.\n");
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);
	smagCloseLog(prob);

  return EXIT_SUCCESS;
}


SCIP_RETCODE runSCIP(smagHandle_t prob) {
	SCIP_Bool solvelp = TRUE;
	SCIP_Bool solvefinal = TRUE;

  SolveStatus solstatus;
  solstatus.colval=NULL;

  GamsHandlerSmag gamshandler(prob);
	GamsMessageHandler messagehandler(gamshandler);
	messagehandler.setLogLevel(0);
  
  bool islp = !prob->gms.nbin && !prob->gms.numint && !prob->gms.nsos1 && !prob->gms.nsos2;
  
  if (!islp) { // if not just LP, do SCIP
  	GamsDictionary dict(gamshandler);
  	
  	SCIP_MESSAGEHDLR* messagehdlr=NULL;
    SCIP_MESSAGEHDLRDATA messagehdlrdata;
    messagehdlrdata.smag=prob;
    
    SCIP_CALL( SCIPcreateMessagehdlr(&messagehdlr, TRUE, printWarningError, printWarningError, printInfoDialog, printInfoDialog, &messagehdlrdata) );
  	SCIP_CALL( SCIPsetMessagehdlr(messagehdlr) );

  	// have to put this here already, because we need the value for infinity
  	SCIP* scip = NULL;
  	// initialize SCIP
  	SCIP_CALL( SCIPcreate(&scip) );

  	smagSetSqueezeFreeRows(prob, 1);	/* don't show me =n= rows */
  	smagSetInf(prob, SCIPinfinity(scip));
  	smagReadModel(prob);

  	/* include default SCIP plugins, documentation says it needs to come after BCH setup because of its display column;
  	 * on the other hand the plugins should be included (and their parameters registered) before the option file read */
  	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

  	SCIP_CALL( setupMIPParameters(prob, scip) );
  	
		SCIP_CALL( SCIPgetBoolParam(scip, "gams/solvefinal", &solvefinal) );

  	char* dobch;
  	SCIP_CALL( SCIPgetStringParam(scip, "gams/usercutcall", &dobch) );
  	if (!*dobch)
    	SCIP_CALL( SCIPgetStringParam(scip, "gams/userheurcall", &dobch) );
//  	if (!*dobch)
//    	SCIP_CALL( SCIPgetStringParam(scip, "gams/userincbcall", &dobch) );
  	if (!*dobch)
    	SCIP_CALL( SCIPgetStringParam(scip, "gams/userincbicall", &dobch) );

  	SCIP_VAR** mip_vars=NULL;

  	GamsBCH* bch=NULL;
  	void* bchdata=NULL;
  	gdxHandle_t gdxhandle=NULL;
  	if (*dobch) {
  		char buffer[512];
  		if (!gdxCreate(&gdxhandle, buffer, sizeof(buffer))) {
  			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
  			smagStdOutputFlush(prob, SMAG_ALLMASK);
  			return SCIP_ERROR;
  		}
  		SCIP_CALL( BCHsetup(scip, &mip_vars, prob, gamshandler, dict, bch, bchdata) );
  	}

  	SCIP_CALL( setupMIP(prob, gamshandler, dict, scip, mip_vars) );
  	
  	SCIP_Bool mipstart;
  	SCIP_CALL( SCIPgetBoolParam(scip, "gams/mipstart", &mipstart) );
  	if (mipstart)
  		SCIP_CALL( setupMIPStart(prob, scip, mip_vars) );

  	if (strncmp(SCIPlpiGetSolverName(), "Clp", 3) == 0)
  	{
  		SCIP_LPI* lpi;
  		SCIPgetLPI(scip, &lpi);
  		if (lpi)
  		{
  			((ClpSimplex*)SCIPlpiGetSolverPointer(lpi))->passInMessageHandler(&messagehandler);
  		}
  	}

//  	SCIP_CALL( SCIPprintOrigProblem(scip, NULL, "mps", TRUE) );
  	
  	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nStarting MIP solve...\n");
  	smagStdOutputFlush(prob, SMAG_LOGMASK);

  	SCIP_CALL( SCIPsolve(scip) );
  	
  	SCIP_CALL( checkMIPsolve(prob, scip, mip_vars, solstatus) );
  	
  	delete[] mip_vars;

    SCIP_CALL( SCIPfree(&scip) );
  	SCIP_CALL( SCIPfreeMessagehdlr(&messagehdlr) );
  	
  	// we only solve the fixed MIP if we have a MIP feasible point and the user allows us
  	solvelp = solstatus.colval && solvefinal;
  	
  	SCIP_CALL( BCHcleanup(prob, bch, bchdata) );
  	if (gdxhandle) {
  		gdxClose(gdxhandle);
  		gdxFree(&gdxhandle);
  		gdxLibraryUnload();
  	}
  }

	SCIP_LPI* lpi=NULL;
	double lpsolve_starttime=smagGetCPUTime(prob);
  if (solvelp) { // if we have an LP or got a mip feasible point and user did not disable fixed LP solve, solve (fixed) LP
  	SCIP_CALL( SCIPlpiCreate(&lpi, "gamsproblem", smagMinim(prob)==-1 ? SCIP_OBJSEN_MAXIMIZE : SCIP_OBJSEN_MINIMIZE) );
  	if (strncmp(SCIPlpiGetSolverName(), "Clp", 3) == 0)
  	{
  		((ClpSimplex*)SCIPlpiGetSolverPointer(lpi))->passInMessageHandler(&messagehandler);
  	}

  	if (islp) {
  		// here we allow =n= rows in order to get the lp11 test passed 
  		smagSetInf (prob, SCIPlpiInfinity(lpi));
  		smagReadModel (prob);
  	}
  	
  	SCIP_CALL( setupLP(prob, lpi, solstatus.colval) );

  	if (islp) {
  		SCIP_CALL( setupLPParameters(prob, lpi) );
  		messagehandler.setLogLevel(1); // SCIP has set Clp's loglevel to 2, but then it might print to stdout
    	smagStdOutputPrint(prob, SMAG_LOGMASK, "Starting LP solve...\n");
  	} else {
    	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolving LP with fixed discrete variables...\n");
  	}
  	smagStdOutputFlush(prob, SMAG_LOGMASK);

  	SCIP_RETCODE lpreturn = SCIPlpiSolveDual(lpi);
  	if (!islp && lpreturn == SCIP_LPERROR) // catch hard lp failures in final lp solve
  	 	SCIP_CALL( SCIPlpiFree(&lpi) );
  	else
  		SCIP_CALL( lpreturn );
  }

	// if problem was an LP, get gams status and statistics
	if (islp) {
		solstatus.time = smagGetCPUTime(prob)-lpsolve_starttime;
		SCIP_CALL( checkLPsolve(prob, lpi, solstatus) );
	}
  
  SCIP_CALL( writeSolution(prob, solstatus, lpi, solvefinal) );

  if (lpi)
  	SCIP_CALL( SCIPlpiFree(&lpi) );
  
  delete[] solstatus.colval;
  
  return SCIP_OKAY;
}

SCIP_RETCODE setupMIP(smagHandle_t prob, GamsHandler& gamshandler, GamsDictionary& dict, SCIP* scip, SCIP_VAR**& vars) {
	SCIP_CALL( SCIPcreateProb(scip, "gamsmodel", NULL, NULL, NULL, NULL, NULL, NULL) );
	
	if (!dict.haveNames()) {
		SCIP_Bool read_dict=FALSE;
		SCIP_CALL( SCIPgetBoolParam(scip, "gams/names", &read_dict) );
		if (read_dict)
			dict.readDictionary();
	}
	
	char buffer[256];
	
	vars=new SCIP_VAR*[smagColCount(prob)];
	
	double minprior=0;
	double maxprior=0;
	if (prob->gms.priots && smagColCount(prob)>0) { // compute range of given priorities
		minprior=prob->colPriority[0];
		maxprior=prob->colPriority[0];
		for (int i=0; i<smagColCount(prob); ++i) {
			if (prob->colType[i] == SMAG_VAR_CONT)
				continue;
			if (prob->colPriority[i]<minprior) minprior=prob->colPriority[i];
			else if (prob->colPriority[i]>maxprior) maxprior=prob->colPriority[i];
		}
	}

	if (prob->niceObjRow && prob->gms.grhs[prob->gms.slplro-1]) {
		sprintf(buffer, "Note: Constant %g in objective function is ignored during SCIP run.\n", prob->gms.grhs[prob->gms.slplro-1] * prob->gObjFactor);
		smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
	}

	smagObjGradRec_t* og = prob->objGrad;
	for (int i=0; i<smagColCount(prob); ++i) {
		SCIP_VARTYPE vartype;
		switch (prob->colType[i]) {
			case SMAG_VAR_CONT:
			case SMAG_VAR_SOS1:
			case SMAG_VAR_SOS2:
				vartype=SCIP_VARTYPE_CONTINUOUS;
				break;
			case SMAG_VAR_BINARY:
				vartype=SCIP_VARTYPE_BINARY;
				break;
			case SMAG_VAR_INTEGER:
				vartype=SCIP_VARTYPE_INTEGER;
				break;
			default : {
				smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: semicontinuous and semiinteger variables not supported.\n");
	      smagStdOutputFlush(prob, SMAG_ALLMASK);
				return SCIP_READERROR;
			}
		}
		double obj_coeff=0.;
		if (og && og->j==i) {
			obj_coeff=og->dfdj;
			og=og->next;
		}
		char* varname=NULL;
		if (dict.haveNames())
			varname=dict.getColName(i, buffer, 256);
		SCIP_CALL( SCIPcreateVar(scip, vars+i, varname, prob->colLB[i], prob->colUB[i], obj_coeff, vartype, TRUE, FALSE, NULL, NULL, NULL, NULL) );
		SCIP_CALL( SCIPaddVar(scip, vars[i]) );
		
		if (prob->gms.priots && minprior<maxprior && prob->colType[i] != SMAG_VAR_CONT) {
			// in GAMS: higher priorities are given by smaller .prior values
			// in SCIP: variables with higher branch priority are always preferred to variables with lower priority in selection of branching variable
			// thus, we scale the values from GAMS to lie between 0 (lowest prior) and 1000 (highest prior) 
			int branchpriority = (int)(1000./(maxprior-minprior)*(maxprior-prob->colPriority[i]));
			SCIP_CALL( SCIPchgVarBranchPriority(scip, vars[i], branchpriority) );
		}
	}
	
	int maxrowlen=0;
	for (int i=0; i<smagRowCount(prob); ++i)
		if (prob->rowLen[i]>maxrowlen)
			maxrowlen = prob->rowLen[i];
	
	SCIP_VAR** con_vars=new SCIP_VAR*[maxrowlen];
	SCIP_Real* con_coef=new SCIP_Real[maxrowlen];
  SCIP_CONS* con;
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
		
		const char* conname=NULL;
		if (dict.haveNames())
			conname=dict.getRowName(i, buffer, 256);
		if (!conname) {
			sprintf(buffer, "con%d", i);
			conname=buffer;
		}
		SCIP_CALL( SCIPcreateConsLinear(scip, &con, conname, ncoef, con_vars, con_coef, lb, ub,
				TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE) );
		
		SCIP_CALL( SCIPaddCons(scip, con) );
		
//		SCIP_CALL( SCIPprintCons(scip, con, NULL) );

		SCIP_CALL( SCIPreleaseCons(scip, &con) );
	}
	
	if (prob->gms.nosos1 || prob->gms.nosos2)
	{
		SCIP_VAR** sos = new SCIP_VAR*[MAX(prob->gms.nsos1, prob->gms.nsos2)];
		for (int i = 0; i < prob->gms.nosos1 + prob->gms.nosos2; ++i)
		{
			int n = 0;
			int sostype = 0;
			for (int j = 0; j < smagColCount(prob); ++j)
			{
				if (prob->colType[j] != SMAG_VAR_SOS1 && prob->colType[j] != SMAG_VAR_SOS2)
					continue;
				if (prob->colSOS[j] == i+1)
				{
					assert(n < MAX(prob->gms.nsos1, prob->gms.nsos2));
					sos[n] = vars[j];
					sostype = prob->colType[j] == SMAG_VAR_SOS1 ? 1 : 2;
					n++;
				}
			}
			sprintf(buffer, "sos%d", i);
			if (sostype == 1)
			{
				SCIP_CALL( SCIPcreateConsSOS1(scip, &con, buffer, n, sos, NULL, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
			}
			else if (sostype == 2)
			{
				SCIP_CALL( SCIPcreateConsSOS2(scip, &con, buffer, n, sos, NULL, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
			}
			else
			{
				SCIPerrorMessage("SOS type %d not supported.\n", sostype);
				return SCIP_ERROR;
			}
			SCIP_CALL( SCIPaddCons(scip, con) );
			//		SCIP_CALL( SCIPprintCons(scip, con, NULL) );
			SCIP_CALL( SCIPreleaseCons(scip, &con) );
		}
		delete[] sos;
	}

	if (smagMinim(prob)==-1)
		SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );
	
	delete[] con_vars;
	delete[] con_coef;
	
	return SCIP_OKAY;
}

SCIP_RETCODE setupMIPParameters(smagHandle_t prob, SCIP* scip) {
	char buffer[512];
	if (prob->gms.optca >= prob->inf) {
		prob->gms.optca=0.999*prob->inf;
		snprintf(buffer, 512, "Value for optca greater or equal than value for infinity. Reduced to %g.\n", prob->gms.optca);
		smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
	}
	
	SCIP_CALL( SCIPsetLongintParam(scip, "limits/nodes", prob->gms.nodlim ? prob->gms.nodlim : prob->gms.itnlim) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/time", prob->gms.reslim) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/gap", prob->gms.optcr) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/absgap", prob->gms.optca) );
	SCIP_CALL( SCIPsetIntParam(scip, "display/width", 80) );
	
	SCIPchgFeastol(scip, 1e-7);
	SCIPchgDualfeastol(scip, 1e-7);
	
	//TODO: cutoff (does not seem to be supported by SCIP yet)

	SCIP_CALL( SCIPaddBoolParam(scip, "gams/names", "whether the gams dictionary should be read and col/row names be given to scip", NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/solvefinal", "whether the problem should be solved with fixed discrete variables to get dual values", NULL, FALSE, TRUE, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/mipstart", "whether to try initial point as first primal solution", NULL, FALSE, TRUE, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/print_statistics", "whether to print statistics on a MIP solve", NULL, FALSE, FALSE, NULL, NULL) );
	
	SCIP_CALL( BCHaddParam(scip) );

  if (prob->gms.useopt) {
  	SCIP_RETCODE ret = SCIPreadParams(scip, prob->gms.optFileName);
  	if (ret != SCIP_OKAY ) {
  		snprintf(buffer, 512, "WARNING: Reading of optionfile %s failed with error %d ! We continue.\n", prob->gms.optFileName, ret);
  		smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
  	} else {
  		snprintf(buffer, 512, "Optionfile %s successfully read.\n", prob->gms.optFileName);
  		smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
  	}
  }

	return SCIP_OKAY;
}

SCIP_RETCODE setupMIPStart(smagHandle_t prob, SCIP* scip, SCIP_VAR**& vars) {
	SCIP_CALL( SCIPtransformProb(scip) );
	
	SCIP_SOL* sol;
	SCIP_CALL( SCIPcreateOrigSol(scip, &sol, NULL) );
	
	SCIP_CALL( SCIPsetSolVals(scip, sol, smagColCount(prob), vars, prob->colLev) );
	
	SCIP_Bool stored;
	SCIP_CALL( SCIPtrySolFree(scip, &sol, TRUE, TRUE, TRUE, &stored) );
	
	if (stored)
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Feasible initial solution used to initialize primal bound.\n");
/*	else
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Initial solution not feasible.");
*/		
	
	return SCIP_OKAY;
}

SCIP_RETCODE checkMIPsolve(smagHandle_t prob, SCIP* scip, SCIP_VAR** vars, SolveStatus& solstatus) {
	SCIP_Bool printstat;
	SCIP_CALL( SCIPgetBoolParam(scip, "gams/print_statistics", &printstat) );
	if (printstat) {
		smagStdOutputPrint(prob, SMAG_LOGMASK, "\n");
		SCIP_CALL( SCIPprintStatistics(scip, NULL) );
	}

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
			solstatus.model_status = nrsol ? 8 : 14; // integer solution or no solution
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "User interrupt.\n");
			break;
		case SCIP_STATUS_NODELIMIT: // the solving process was interrupted because the node limit was reached
		case SCIP_STATUS_STALLNODELIMIT: // the solving process was interrupted because the node limit was reached
			solstatus.solver_status = 2; // lets call it iteration interrupt
			solstatus.model_status = nrsol ? 8 : 14; // integer solution or no solution
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Node limit exceeded.\n");
			break;
		case SCIP_STATUS_TIMELIMIT: // the solving process was interrupted because the time limit was reached
			solstatus.solver_status = 3; // resource interrupt
			solstatus.model_status = nrsol ? 8 : 14; // integer solution or no solution
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Time limit exceeded.\n");
			break;
		case SCIP_STATUS_MEMLIMIT: // the solving process was interrupted because the memory limit was reached
			solstatus.solver_status = 3; // resource interrupt
			solstatus.model_status = nrsol ? 8 : 14; // integer solution or no solution
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Memory limit exceeded.\n");
			break;
		case SCIP_STATUS_GAPLIMIT: // the solving process was interrupted because the gap limit was reached
			solstatus.solver_status = 1; // normal completion
			solstatus.model_status = nrsol ? ((SCIPgetGap(scip)>0) ? 8 : 1) : 14;
//			smagStdOutputPrint(prob, SMAG_LOGMASK, "Optimality gap below tolerance.\n");
			break;
		case SCIP_STATUS_SOLLIMIT: // the solving process was interrupted because the solution limit was reached
		case SCIP_STATUS_BESTSOLLIMIT: // the solving process was interrupted because the solution improvement limit was reached
			solstatus.solver_status = 3; // resource interrupt
			solstatus.model_status = nrsol ? 8 : 14;
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
	solstatus.nodenum=(int)SCIPgetNNodes(scip);
	solstatus.iterations=solstatus.nodenum;
	solstatus.time=SCIPgetTotalTime(scip);

	if (nrsol) {
//  	SCIP_CALL( SCIPprintBestSol(scip, NULL, FALSE) );
		SCIP_SOL* sol=SCIPgetBestSol(scip);
		solstatus.colval = new double[smagColCount(prob)];
//		printf("nr vars in SCIP: %d\t in SMAG: %d\n", SCIPgetNVars(scip), smagColCount(prob));
//		assert(SCIPgetNVars(scip)==smagColCount(prob));
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
	
	SCIPlpiAddCols(lpi, smagColCount(prob), objcoef, collb ? collb : prob->colLB, colub ? colub : prob->colUB, NULL, 0, NULL, NULL, NULL);
	
	delete[] objcoef;
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
  
  if (ncols==0) { // if no columns, add a "fake" one to prevent Clp from crashing
  	double zero=0.;
  	int zeroint=0;
  	SCIPlpiAddCols(lpi, 1, &zero, &zero, &zero, NULL, 0, &zeroint, NULL, NULL);
  }
  
	return SCIP_OKAY;
}

SCIP_RETCODE setupLPParameters(smagHandle_t prob, SCIP_LPI* lpi) {
	SCIPlpiSetIntpar(lpi, SCIP_LPPAR_LPINFO, TRUE);
	SCIPlpiSetIntpar(lpi, SCIP_LPPAR_LPITLIM, prob->gms.itnlim);
	SCIPlpiSetRealpar(lpi, SCIP_LPPAR_LPTILIM, prob->gms.reslim);

	return SCIP_OKAY;
}

SCIP_RETCODE checkLPsolve(smagHandle_t prob, SCIP_LPI* lpi, SolveStatus& solstatus) {
	SCIP_Bool primalfeasible=SCIPlpiIsPrimalFeasible(lpi);

	smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolution status: ");
	
	if (SCIPlpiIsPrimalUnbounded(lpi)) {
		solstatus.solver_status = 1; // normal completion
		solstatus.model_status = primalfeasible ? 3 : 18; // unbounded with or without solution	
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Problem is unbounded.\n");
	} else if (SCIPlpiIsPrimalInfeasible(lpi)) {
		solstatus.solver_status = 1; // normal completion
		solstatus.model_status = 19; // infeasible - no solution
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
		if ((smagColCount(prob)==0 || smagRowCount(prob)==0) && primalfeasible) { // claim that a more-or-less empty problem was solved to optimality
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

SCIP_RETCODE writeSolution(smagHandle_t prob, SolveStatus& solstatus, SCIP_LPI* lpi, SCIP_Bool solve_final) {
	// (no MIP feasible solution) and (no LP solved or LP infeasible) 
	if (!solstatus.colval && (!lpi || !SCIPlpiIsPrimalFeasible(lpi))) {
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
	
	if (lpi && SCIPlpiIsPrimalFeasible(lpi)) { // read solution from lpi object
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
		if (solve_final)
			smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolving fixed LP failed. Only primal solution values will be available.\n");
		else
			smagStdOutputPrint(prob, SMAG_LOGMASK, "\nSolving fixed LP disabled. Only primal solution values will be available.\n");
		memset(rowmarg, 0, smagRowCount(prob)*sizeof(double));
		for (int i=0; i<smagRowCount(prob); ++i) {
			rowbasstat[i]=SMAG_BASSTAT_SUPERBASIC;
			rowindic[i]=SMAG_RCINDIC_OK;
		}

		if (!solstatus.colval) { // we should have colval here, otherwise we had returned in the first lines of this routine already 
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
	
	double objoffset = prob->niceObjRow ? prob->gms.grhs[prob->gms.slplro-1] * prob->gObjFactor : 0;
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

/* info messages normally go to logfile
 * except if the file argument is explicitely set to the status file, then they will go to status and list file
 */
void printInfoDialog(SCIP_MESSAGEHDLR* messagehdlr, FILE* file, const char* msg) {
	if (file && file==messagehdlr->messagehdlrdata->smag->fpStatus)
		smagStdOutputPrint(messagehdlr->messagehdlrdata->smag, SMAG_STATUSMASK | SMAG_LISTMASK, msg);
	else
		smagStdOutputPrint(messagehdlr->messagehdlrdata->smag, SMAG_LOGMASK, msg);
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
