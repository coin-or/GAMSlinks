// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsScip.hpp"
#include "GamsScip.h"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
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

// GAMS
#define GMS_SV_NA     2.0E300   /* not available/applicable */
#include "gmomcc.h"
#include "gevmcc.h"
#include "GamsMessageHandler.hpp"

#include "ClpSimplex.hpp" // for passing in message handler

#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/cons_linear.h"
#include "scip/cons_quadratic.h"
#include "scip/cons_sos1.h"
#include "scip/cons_sos2.h"

// workaround for accessing Objoffset; should be addObjoffset, but that does not seem to be implemented
//#include "scip/struct_scip.h"
//#include "scip/prob.h"

SCIP_DECL_MESSAGEERROR(GamsScipPrintWarningOrError) {
	assert(SCIPmessagehdlrGetData(messagehdlr) != NULL);
	gevLogStatPChar(((GamsScip*)SCIPmessagehdlrGetData(messagehdlr))->gev, msg);
}

SCIP_DECL_MESSAGEINFO(GamsScipPrintInfoOrDialog) {
	assert(SCIPmessagehdlrGetData(messagehdlr) != NULL);
	gevLogPChar(((GamsScip*)SCIPmessagehdlrGetData(messagehdlr))->gev, msg);
}

GamsScip::GamsScip()
: gmo(NULL), gev(NULL), isDemo(false), gamsmsghandler(NULL), scip(NULL), scipmsghandler(NULL), vars(NULL), lpi(NULL)
{
  sprintf(scip_message, "SCIP version %d.%d.%d [LP solver: %s]\n%s\n", SCIPmajorVersion(), SCIPminorVersion(), SCIPtechVersion(), SCIPlpiGetSolverName(), SCIP_COPYRIGHT);
}

GamsScip::~GamsScip() {
	if (scip != NULL) {
		if (vars != NULL) {
			for (int i = 0; i < gmoN(gmo); ++i)
				SCIP_CALL_ABORT( SCIPreleaseVar(scip, &vars[i]) );
			delete[] vars;
		}
		SCIP_CALL_ABORT( SCIPsetDefaultMessagehdlr() );
		SCIP_CALL_ABORT( SCIPfree(&scip) );
		SCIP_CALL_ABORT( SCIPfreeMessagehdlr(&scipmsghandler) );
	}
  if (lpi != NULL)
  	SCIP_CALL_ABORT( SCIPlpiFree(&lpi) );

	delete gamsmsghandler;
}

int GamsScip::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	gmo = gmo_;
	assert(gmo);
	assert(!scip);
	assert(!lpi);

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1;

	gev = (gevRec*)gmoEnvironment(gmo);

#ifdef GAMS_BUILD
#define GEVPTR gev
#include "cgevmagic2.h"
	if (gevLicenseCheck(gev, gmoM(gmo), gmoN(gmo), gmoNZ(gmo), gmoNLNZ(gmo), gmoNDisc(gmo))) {
		// model larger than demo and no solver-specific license; check if we have an academic license
		int isAcademic = 0;
    gevLicenseQueryOption(gev, "GAMS", "ACADEMIC", &isAcademic);
    if (!isAcademic) {
    	char msg[256];
  		while (gevLicenseGetMessage(gev, msg))
  			gevLogStat(gev, msg);
      gevLogStat(gev, "*** Use of SCIP is limited to academic users.");
      gevLogStat(gev, "*** Please contact koch@zib.de to arrange for a license.");
  	  gmoSolveStatSet(gmo, SolveStat_License);
  	  gmoModelStatSet(gmo, ModelStat_LicenseError);
  	  return 1;
    }
    isDemo = false;
	} else {
		// we have a demo modell, or license is available; however, there is nothing like a scip license, so it means the model fits into the demo size
		int isAcademic = 0;
    gevLicenseQueryOption(gev, "GAMS", "ACADEMIC", &isAcademic);
    isDemo = !isAcademic; // we run in demo mode if there is no academic license, that is we cannot allow to open a scip shell
	}
#endif

	gmoObjReformSet(gmo, 1);
	gmoObjStyleSet(gmo, ObjType_Fun);
  gmoIndexBaseSet(gmo, 0);

	if (gmoGetVarTypeCnt(gmo, var_SC) || gmoGetVarTypeCnt(gmo, var_SI)) {
		gevLogStat(gev, "ERROR: Semicontinuous and semiinteger variables not supported by SCIP interface yet.\n");
		gmoSolveStatSet(gmo, SolveStat_Capability);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		return 1;
  }
	
	if (gmoGetEquTypeCnt(gmo, equ_C) || gmoGetEquTypeCnt(gmo, equ_X)) {
		gevLogStat(gev, "ERROR: Conic constraints and external functions not supported by SCIP or SCIP interface.\n");
		gmoSolveStatSet(gmo, SolveStat_Capability);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		return 1;
  }
	
	gamsmsghandler = new GamsMessageHandler(gev);
  
	SCIP_RETCODE scipret;
	if (isLP())
		scipret = setupLPI();
	else
		scipret = setupSCIP();

	if (scipret != SCIP_OKAY) {
		char buffer[512];
		snprintf(buffer, 512, "Error %d in call of SCIP function\n", scipret);
		gevLogStatPChar(gev, buffer);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return 1;
	} 
	
	return 0;
}

int GamsScip::callSolver() {
	SCIP_RETCODE scipret;
	
	if (isLP()) {
		scipret = setupInitialBasis();

		double starttime;
		if (scipret == SCIP_OKAY) {
			gevLogStat(gev, "\nCalling SCIP LPI main solution routine...");
			starttime = gevTimeDiffStart(gev);

			scipret = SCIPlpiSolveDual(lpi);
		}

  	if (scipret == SCIP_OKAY)
  		scipret = processLPSolution(gevTimeDiffStart(gev) - starttime);
  	
	} else {
		scipret = setupStartPoint();
		
		if (scipret == SCIP_OKAY) {
			SCIP_Bool interact = FALSE;
			if (!isDemo)
				SCIP_CALL_ABORT( SCIPgetBoolParam(scip, "gams/interactive", &interact) );
			if (interact)
				scipret = SCIPstartInteraction(scip);
			else {
				gevLogStat(gev, "\nCalling SCIP main solution routine...");
				scipret = SCIPsolve(scip);
			}
		}

  	if (scipret == SCIP_OKAY) {
  		SCIP_Bool printstat;
  		SCIP_CALL_ABORT( SCIPgetBoolParam(scip, "gams/print_statistics", &printstat) );
  		if (printstat) {
  			gevLog(gev, "");
  			SCIPprintStatistics(scip, NULL);
  		}
  		
  		scipret = processMIQCPSolution();
  	}
	}
	
	if (scipret != SCIP_OKAY) {
		char buffer[512];
		snprintf(buffer, 512, "Error %d in call of SCIP function\n", scipret);
		gevLogStatPChar(gev, buffer);
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return 1;
	}
	
	return 0;
}

SCIP_RETCODE GamsScip::setupLPI() {
	assert(lpi == NULL);
	
	SCIP_CALL( SCIPlpiCreate(&lpi, "gamsproblem", gmoSense(gmo) == Obj_Max ? SCIP_OBJSEN_MAXIMIZE : SCIP_OBJSEN_MINIMIZE) );

	SCIPlpiSetIntpar(lpi, SCIP_LPPAR_LPINFO, TRUE);
	SCIPlpiSetIntpar(lpi, SCIP_LPPAR_LPITLIM, gevGetIntOpt(gev, gevIterLim));
	SCIPlpiSetRealpar(lpi, SCIP_LPPAR_LPTILIM, gevGetDblOpt(gev, gevResLim));

	if (strncmp(SCIPlpiGetSolverName(), "Clp", 3) == 0) {
		gamsmsghandler->setLogLevel(1); // SCIP has set Clp's loglevel to 2, but then it might print to stdout
		((ClpSimplex*)SCIPlpiGetSolverPointer(lpi))->passInMessageHandler(gamsmsghandler);
	}

	gmoPinfSet(gmo,  SCIPlpiInfinity(lpi));
	gmoMinfSet(gmo, -SCIPlpiInfinity(lpi));

	if (gmoObjConst(gmo)) {
		char buffer[256];
		sprintf(buffer, "Note: Constant %g in objective function is ignored during LP solve.\n", gmoObjConst(gmo));
		gevLogStatPChar(gev, buffer);
	}
	
	double* objcoefs = new double[gmoN(gmo)];
	double* collb    = new double[gmoN(gmo)];
	double* colub    = new double[gmoN(gmo)];
	gmoGetObjVector(gmo, objcoefs);
	gmoGetVarLower(gmo, collb);
	gmoGetVarUpper(gmo, colub);
	SCIP_CALL( SCIPlpiAddCols(lpi, gmoN(gmo), objcoefs, collb, colub, NULL, 0, NULL, NULL, NULL) );
	
	delete[] objcoefs;
	delete[] collb;
	delete[] colub;
	
	int*    rowstarts = new int[gmoM(gmo)+1];
	int*    cols   = new int[gmoNZ(gmo)];
	double* values = new double[gmoNZ(gmo)];
	double* rowlhs = new double[gmoM(gmo)];
	double* rowrhs = new double[gmoM(gmo)];
	
	gmoGetMatrixRow(gmo, rowstarts, cols, values, NULL);
  rowstarts[gmoM(gmo)] = gmoNZ(gmo); /* just to be sure it's there */
  
  for (int i = 0; i < gmoM(gmo); ++i)
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E: 
				rowlhs[i] = rowrhs[i] = gmoGetRhsOne(gmo, i);
				break;
			case equ_G:
				rowlhs[i] = gmoGetRhsOne(gmo, i);
				rowrhs[i] = SCIPlpiInfinity(lpi);
				break;
			case equ_L:
				rowlhs[i] = -SCIPlpiInfinity(lpi);
				rowrhs[i] = gmoGetRhsOne(gmo, i);
				break;
			case equ_N:
				rowlhs[i] = -SCIPlpiInfinity(lpi);
				rowrhs[i] =  SCIPlpiInfinity(lpi);
				break;
			case equ_X:
			case equ_C:
			default:
				gevLogStat(gev, "ERROR: Unsupported equation type for LPI.");
				return SCIP_READERROR;
		}
  
//  printf("%d cols; %d rows; %d nz:\n", gmoN(gmo),  gmoM(gmo), gmoNZ(gmo));
//  for (int i = 0; i < gmoM(gmo); ++i) {
//  	printf("%d;  %g <= ", rowstarts[i], rowlhs[i]);
//  	for (int j = rowstarts[i]; j < rowstarts[i+1]; ++j)
//  		printf("%+g*x%d ", values[j], cols[j]);
//  	printf("<= %g\n", rowrhs[i]);
//  }
	
  SCIP_CALL( SCIPlpiAddRows(lpi, gmoM(gmo), rowlhs, rowrhs, NULL, gmoNZ(gmo), rowstarts, cols, values) );
  
  delete[] values;
  delete[] cols;
  delete[] rowstarts;
  delete[] rowlhs;
  delete[] rowrhs;
  
  if (gmoN(gmo) == 0) { // if no columns, add a "fake" one to prevent Clp from crashing
  	double zero = 0.0;
  	int zeroint = 0;
  	SCIP_CALL( SCIPlpiAddCols(lpi, 1, &zero, &zero, &zero, NULL, 0, &zeroint, NULL, NULL) );
  }

	return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::setupSCIP() {
	assert(scip == NULL);
	assert(scipmsghandler == NULL);
	
  SCIP_CALL( SCIPcreateMessagehdlr(&scipmsghandler, FALSE, GamsScipPrintWarningOrError, GamsScipPrintWarningOrError, GamsScipPrintInfoOrDialog, GamsScipPrintInfoOrDialog, (SCIP_MESSAGEHDLRDATA*)this) );
	SCIP_CALL( SCIPsetMessagehdlr(scipmsghandler) );

	SCIP_CALL( SCIPcreate(&scip) );
	
	gmoPinfSet(gmo,  SCIPinfinity(scip));
	gmoMinfSet(gmo, -SCIPinfinity(scip));
	
	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
	
	SCIP_CALL( setupSCIPParameters() );
	SCIP_CALL( setupMIQCP() );

	if (strncmp(SCIPlpiGetSolverName(), "Clp", 3) == 0) {
		assert(SCIPgetStage(scip) >= SCIP_STAGE_TRANSFORMED);
		gamsmsghandler->setLogLevel(0);
		SCIP_LPI* lpi;
		SCIP_CALL( SCIPgetLPI(scip, &lpi) );
		if (lpi)
			((ClpSimplex*)SCIPlpiGetSolverPointer(lpi))->passInMessageHandler(gamsmsghandler);
	}
	
	return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::setupSCIPParameters() {
	assert(scip != NULL);
	
	char buffer[512];
	if (SCIPisInfinity(scip, gevGetDblOpt(gev, gevOptCA))) {
		gevSetDblOpt(gev, gevOptCA, 0.999*SCIPinfinity(scip));
		snprintf(buffer, sizeof(buffer), "Warning: Value for optca greater or equal than value for infinity. Reduced to %g.\n", 0.999*SCIPinfinity(scip));
		gevLogStatPChar(gev, buffer);
	}
	
	if (gevGetIntOpt(gev, gevNodeLim))
		SCIP_CALL( SCIPsetLongintParam(scip, "limits/nodes", gevGetIntOpt(gev, gevNodeLim)) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/time", gevGetDblOpt(gev, gevResLim)) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/gap", gevGetDblOpt(gev, gevOptCR)) );
	SCIP_CALL( SCIPsetRealParam(scip, "limits/absgap", gevGetDblOpt(gev, gevOptCA)) );
	SCIP_CALL( SCIPsetIntParam(scip, "display/width", 80) );

	SCIPchgFeastol(scip, 1e-7);
	SCIPchgDualfeastol(scip, 1e-7);

	SCIP_CALL( SCIPaddBoolParam(scip, "gams/names",            "whether the gams dictionary should be read and col/row names be given to scip",         NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/solvefinal",       "whether the problem should be solved with fixed discrete variables to get dual values", NULL, FALSE, TRUE,  NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/mipstart",         "whether to try initial point as first primal solution",                                 NULL, FALSE, TRUE,  NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/print_statistics", "whether to print statistics on a MIP solve",                                            NULL, FALSE, FALSE, NULL, NULL) );
	if (!isDemo)
		SCIP_CALL( SCIPaddBoolParam(scip, "gams/interactive",      "whether a SCIP shell should be opened instead of issuing a solve command",              NULL, FALSE, FALSE, NULL, NULL) );
	
  if (gmoOptFile(gmo)) {
  	char optfilename[1024];
  	gmoNameOptFile(gmo, optfilename);
  	SCIP_RETCODE ret = SCIPreadParams(scip, optfilename);
  	if (ret != SCIP_OKAY ) {
  		snprintf(buffer, sizeof(buffer), "WARNING: Reading of optionfile %s failed with error %d ! We continue.\n", optfilename, ret);
  		gevLogStatPChar(gev, buffer);
  	} else {
  		snprintf(buffer, sizeof(buffer), "Optionfile %s successfully read.\n", optfilename);
  		gevLogPChar(gev, buffer);
  	}
  }
	
	return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::setupMIQCP() {
	assert(scip != NULL);
	assert(vars == NULL);

	char buffer[256];

	SCIP_CALL( SCIPcreateProb(scip, "gamsmodel", NULL, NULL, NULL, NULL, NULL, NULL) );

	SCIP_Bool names = FALSE;
	if (gmoDict(gmo))
		SCIP_CALL( SCIPgetBoolParam(scip, "gams/names", &names) );
	
	if (gmoNLNZ(gmo) || gmoObjNLNZ(gmo)) {
		if (gmoQMaker(gmo, 0.5) < 0) { // negative number is error; positive number is number of nonquadratic nonlinear equations
			gevLogStat(gev, "ERROR: Problems extracting information on quadratic functions in GMO.");
			return SCIP_READERROR;
		}
	}

	vars = new SCIP_VAR*[gmoN(gmo)];
	
	double minprior = SCIPinfinity(scip);
	double maxprior = 0.0;
	if (gmoPriorOpt(gmo) && gmoNDisc(gmo) > 0) { // compute range of given priorities
		for (int i = 0; i < gmoN(gmo); ++i) {
			if (gmoGetVarTypeOne(gmo, i) == var_X)
				continue; // gams forbids branching priorities for continuous variables
			if (gmoGetVarPriorOne(gmo,i) < minprior)
				minprior = gmoGetVarPriorOne(gmo,i);
			if (gmoGetVarPriorOne(gmo,i) > maxprior)
				maxprior = gmoGetVarPriorOne(gmo,i);
		}
	}

	double* coefs;
	SCIP_CALL( SCIPallocBufferArray(scip, &coefs, gmoN(gmo)+1) ); // +1 if we have to transform the objective into a constraint
	
	if (gmoObjNLNZ(gmo) == 0) {
		assert(gmoGetObjOrder(gmo) == order_L);
		gmoGetObjVector(gmo, coefs);
	} else
		memset(coefs, 0, gmoN(gmo)*sizeof(double));
	for (int i = 0; i < gmoN(gmo); ++i) {
		SCIP_VARTYPE vartype;
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
			case var_S1:
			case var_S2:
				vartype = SCIP_VARTYPE_CONTINUOUS;
				break;
			case var_B:
				vartype = SCIP_VARTYPE_BINARY;
				break;
			case var_I:
				vartype = SCIP_VARTYPE_INTEGER;
				break;
			default :
				gevLogStat(gev, "Error: semicontinuous and semiinteger variables not supported.\n");
				return SCIP_READERROR;
		}
		if (names)
			gmoGetVarNameOne(gmo, i, buffer);
		else
			sprintf(buffer, "x%d", i);
		SCIP_CALL( SCIPcreateVar(scip, &vars[i], buffer, gmoGetVarLowerOne(gmo, i), gmoGetVarUpperOne(gmo, i), coefs[i], vartype, TRUE, FALSE, NULL, NULL, NULL, NULL) );
		SCIP_CALL( SCIPaddVar(scip, vars[i]) );
		
		if (gmoPriorOpt(gmo) && minprior < maxprior && gmoGetVarTypeOne(gmo, i) != var_X) {
			// in GAMS: higher priorities are given by smaller .prior values
			// in SCIP: variables with higher branch priority are always preferred to variables with lower priority in selection of branching variable
			// thus, we scale the values from GAMS to lie between 0 (lowest prior) and 1000 (highest prior) 
			int branchpriority = (int)(1000.0 / (maxprior - minprior) * (maxprior - gmoGetVarPriorOne(gmo, i)));
			SCIP_CALL( SCIPchgVarBranchPriority(scip, vars[i], branchpriority) );
		}
	}
	
	int*       indices;
	SCIP_VAR** consvars;
	SCIP_CALL( SCIPallocBufferArray(scip, &indices,  gmoN(gmo)) );
	SCIP_CALL( SCIPallocBufferArray(scip, &consvars, gmoN(gmo)+1) ); // +1 if we have to transform the objective into a constraint
	
	SCIP_VAR** quadvars1 = NULL;
	SCIP_VAR** quadvars2 = NULL;
	SCIP_Real* quadcoefs = NULL;
	int*    qrow = NULL;
	int*    qcol = NULL;
	if (gmoNLNZ(gmo) || gmoObjNLNZ(gmo)) {
		SCIP_CALL( SCIPallocBufferArray(scip, &quadvars1, gmoMaxQnz(gmo)) );
		SCIP_CALL( SCIPallocBufferArray(scip, &quadvars2, gmoMaxQnz(gmo)) );
		SCIP_CALL( SCIPallocBufferArray(scip, &quadcoefs, gmoMaxQnz(gmo)) );
		SCIP_CALL( SCIPallocBufferArray(scip, &qrow, gmoMaxQnz(gmo)) );
		SCIP_CALL( SCIPallocBufferArray(scip, &qcol, gmoMaxQnz(gmo)) );
	}

  SCIP_CONS* con;
	for (int i = 0; i < gmoM(gmo); ++i) {
		double lhs, rhs;
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E: 
				lhs = rhs = gmoGetRhsOne(gmo, i);
				break;
			case equ_G:
				lhs = gmoGetRhsOne(gmo, i);
				rhs = SCIPinfinity(scip);
				break;
			case equ_L:
				lhs = -SCIPinfinity(scip);
				rhs = gmoGetRhsOne(gmo, i);
				break;
			case equ_N:
				lhs = -SCIPinfinity(scip);
				rhs =  SCIPinfinity(scip);
				break;
			case equ_X:
				gevLogStat(gev, "ERROR: External functions not supported by SCIP.");
				return SCIP_READERROR;
			case equ_C:
				gevLogStat(gev, "ERROR: Conic constraints not supported by SCIP interface yet.");
				return SCIP_READERROR;
			default:
				gevLogStat(gev, "ERROR: unknown equation type.");
				return SCIP_READERROR;
		}

		if (names)
			gmoGetEquNameOne(gmo, i, buffer);
		else
			sprintf(buffer, "e%d", i);

		switch (gmoGetEquOrderOne(gmo, i)) {
			case order_L: { // linear constraint
				int nz, nlnz;
				gmoGetRowSparse(gmo, i, indices, coefs, NULL, &nz, &nlnz);
				assert(nlnz == 0);

				for (int j = 0; j < nz; ++j)
					consvars[j] = vars[indices[j]];

				SCIP_CALL( SCIPcreateConsLinear(scip, &con, buffer, nz, consvars, coefs, lhs, rhs,
						TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );
				break;
			}
			
			case order_Q: { // quadratic constraint 
				int nz, nlnz, qnz, qdiagnz;
				
				gmoGetRowSparse(gmo, i, indices, coefs, NULL, &nz, &nlnz);
				for (int j = 0; j < nz; ++j)
					consvars[j] = vars[indices[j]];
				
				gmoGetRowQ(gmo, i, &qnz, &qdiagnz, qcol, qrow, quadcoefs);
				for (int j = 0; j < qnz; ++j) {
					assert(qcol[j] >= 0);
					assert(qrow[j] >= 0);
					assert(qcol[j] < gmoN(gmo));
					assert(qrow[j] < gmoN(gmo));
					quadvars1[j] = vars[qcol[j]];
					quadvars2[j] = vars[qrow[j]];
					if (qcol[j] == qrow[j])
						quadcoefs[j] /= 2.0; // for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO.
				}

				SCIP_CALL( SCIPcreateConsQuadratic(scip, &con, buffer, nz, consvars, coefs, qnz, quadvars1, quadvars2, quadcoefs, lhs, rhs,
						TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
				break;
			}
			
			case order_NL: // nonlinear constraint
				gevLogStat(gev, "ERROR: General nonlinear constraints not supported by SCIP (yet).");
				return SCIP_READERROR;
			default:
				gevLogStat(gev, "ERROR: Only linear and quadratic constraints are supported.");
				return SCIP_READERROR;
		}
		
		SCIP_CALL( SCIPaddCons(scip, con) );		
		SCIP_CALL( SCIPreleaseCons(scip, &con) );
	}
	
	// TODO if there was cancelation (e.g., qcp04), then it could actually be a linear objective, which is indicated by gmoGetObjOrder(gmo) == order_L  
	if (gmoObjNLNZ(gmo)) { // make constraint to represent objective function
		SCIP_VAR* objvar = NULL;
		int nz, /*nlnz,*/ qnz, qdiagnz;
		double lhs, rhs;
		if (gmoGetObjOrder(gmo) != order_L && gmoGetObjOrder(gmo) != order_Q) {
			gevLogStat(gev, "ERROR: General nonlinear objective functions not supported by SCIP (yet).");
			return SCIP_READERROR;
		}
		
		SCIP_CALL( SCIPcreateVar(scip, &objvar, "xobj", -SCIPinfinity(scip), SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL) );
		SCIP_CALL( SCIPaddVar(scip, objvar) );

		gmoGetObjVector(gmo, coefs);
		nz = 0;
		for (int j = 0; j < gmoN(gmo); ++j)
			if (coefs[j]) {
				coefs[nz] = coefs[j]; // should be safe since nz <= j
				consvars[nz] = vars[j];
				++nz;
			}
		
//		gmoGetObjSparse(gmo, indices, coefs, NULL, &nz, &nlnz);
//		for (int j = 0; j < nz; ++j) {
//			consvars[j] = vars[indices[j]];
//			printf("%+g*%s ", coefs[j], SCIPvarGetName(consvars[j]));
//		}
//		printf("\n");
		consvars[nz] = objvar;
		coefs[nz] = -1.0;

		gmoGetObjQ(gmo, &qnz, &qdiagnz, qcol, qrow, quadcoefs);
		for (int j = 0; j < qnz; ++j) {
			assert(qcol[j] >= 0);
			assert(qrow[j] >= 0);
			assert(qcol[j] < gmoN(gmo));
			assert(qrow[j] < gmoN(gmo));
			quadvars1[j] = vars[qcol[j]];
			quadvars2[j] = vars[qrow[j]];
			if (qcol[j] == qrow[j])
				quadcoefs[j] /= 2.0; // for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO.
		}

		if (gmoSense(gmo) == Obj_Min) {
			lhs = -SCIPinfinity(scip);
			rhs = -gmoObjConst(gmo);
		} else {
			lhs = -gmoObjConst(gmo);
			rhs = SCIPinfinity(scip);
		}

		SCIP_CALL( SCIPcreateConsQuadratic(scip, &con, "objective", nz+1, consvars, coefs, qnz, quadvars1, quadvars2, quadcoefs, lhs, rhs,
				TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
		SCIP_CALL( SCIPaddCons(scip, con) );		
		SCIP_CALL( SCIPreleaseCons(scip, &con) );
		
		SCIP_CALL( SCIPreleaseVar(scip, &objvar) );
		
	} else if (!SCIPisZero(scip, gmoObjConst(gmo))) {
		SCIP_VAR* objconst;
		SCIP_CALL( SCIPcreateVar(scip, &objconst, "objconst", 1.0, 1.0, gmoObjConst(gmo), SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL) );
		SCIP_CALL( SCIPaddVar(scip, objconst) );
		SCIP_CALL( SCIPreleaseVar(scip, &objconst) );
//		SCIPprobAddObjoffset(scip->origprob, gmoObjConst(gmo));
	}

	if (gmoSense(gmo) == Obj_Max)
		SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );
	
	int numSos1, numSos2, nzSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	if (nzSos)
	{
		int numSos = numSos1 + numSos2;
		int* sostype =  new int[numSos];
		int* sosbeg  =  new int[numSos+1];
		int* sosind  =  new int[nzSos];
		double* soswt = new double[nzSos];

		gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);
		
		for (int i = 0; i < numSos; ++i)
		{
			int k = 0;
			for (int j = sosbeg[i]; j < sosbeg[i+1]; ++j, ++k) {
				consvars[k] = vars[sosind[j]];
				assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? var_S1 : var_S2));
			}
			
			sprintf(buffer, "sos%d", i);
			if (sostype[i] == 1) {
				SCIP_CALL( SCIPcreateConsSOS1(scip, &con, buffer, k, consvars, &soswt[sosbeg[i]], TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
			} else {
				assert(sostype[i] == 2);
				SCIP_CALL( SCIPcreateConsSOS2(scip, &con, buffer, k, consvars, &soswt[sosbeg[i]], TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
			}
			
			SCIP_CALL( SCIPaddCons(scip, con) );
			SCIP_CALL( SCIPreleaseCons(scip, &con) );
		}
	}

	SCIPfreeBufferArray(scip, &coefs);
	SCIPfreeBufferArray(scip, &indices);
	SCIPfreeBufferArray(scip, &consvars);
	SCIPfreeBufferArrayNull(scip, &quadvars1);
	SCIPfreeBufferArrayNull(scip, &quadvars2);
	SCIPfreeBufferArrayNull(scip, &quadcoefs);
	SCIPfreeBufferArrayNull(scip, &qrow);
	SCIPfreeBufferArrayNull(scip, &qcol);
	
//	SCIP_CALL( SCIPwriteOrigProblem(scip, NULL, "gms", FALSE) );
	
	// need to transform before doing getLPI and before providing initial solution
	// also objective offset does not seem to be copied to transformed problem
	SCIP_CALL( SCIPtransformProb(scip) );
//	if (gmoObjNLNZ(gmo) == 0)
//		SCIPprobAddObjoffset(scip->transprob, gmoObjConst(gmo));
	//TODO there seem to be a problem with the offset when SCIP does restarts

	if (gevGetDblOpt(gev, gevCutOff) != GMS_SV_NA)
		SCIP_CALL( SCIPsetObjlimit(scip, gevGetDblOpt(gev, gevCutOff)) );
	
	return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::setupInitialBasis() {
	assert(lpi != NULL);
	
	if (!gmoHaveBasis(gmo))
		return SCIP_OKAY;
	
	int* cstat = new int[gmoN(gmo)];
	int* rstat = new int[gmoM(gmo)];

	gevLog(gev, "Setting initial basis.");
	
	int nbas = 0;
	for (int j = 0; j < gmoN(gmo); ++j) {
		switch (gmoGetVarBasOne(gmo, j)) {
			case 0: // this seem to mean that variable should be basic
				if (nbas < gmoM(gmo)) {
					cstat[j] = SCIP_BASESTAT_BASIC;
					++nbas;
				} else if (gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo))
					cstat[j] = SCIP_BASESTAT_ZERO;
				else if (fabs(gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j)) < fabs(gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j)))
					cstat[j] = SCIP_BASESTAT_LOWER;
				else
					cstat[j] = SCIP_BASESTAT_UPPER;
				break;
			case 1:
				if (fabs(gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j)) < fabs(gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j)))
					cstat[j] = SCIP_BASESTAT_LOWER;
				else
					cstat[j] = SCIP_BASESTAT_UPPER;
				break;
			default:
				gevLogStat(gev, "ERROR: invalid basis indicator for column.");
				delete[] cstat;
				delete[] rstat;
				return SCIP_ERROR;
		}
	}

	for (int j = 0; j< gmoM(gmo); ++j) {
		switch (gmoGetEquBasOne(gmo, j)) {
			case 0:
				if (nbas < gmoM(gmo)) {
					rstat[j] = SCIP_BASESTAT_BASIC;
					++nbas;
				} else
					rstat[j] = SCIP_BASESTAT_LOWER;
				break;
			case 1:
				rstat[j] = SCIP_BASESTAT_LOWER;
				break;
			default:
				gevLogStat(gev, "ERROR: invalid basis indicator for row.");
				delete[] cstat;
				delete[] rstat;
				return SCIP_ERROR;
		}
	}
	
	SCIP_CALL( SCIPlpiSetBase(lpi, cstat, rstat) );
	SCIP_CALL( SCIPlpiSetIntpar(lpi, SCIP_LPPAR_FROMSCRATCH, 0) );

	delete[] cstat;
	delete[] rstat;

	return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::setupStartPoint() {
	assert(scip != NULL);
	
	SCIP_Bool mipstart;
	SCIP_CALL( SCIPgetBoolParam(scip, "gams/mipstart", &mipstart) );
	if (mipstart == FALSE)
		return SCIP_OKAY;
	
	assert(SCIPgetStage(scip) >= SCIP_STAGE_TRANSFORMED);
	
	SCIP_SOL* sol;
	SCIP_CALL( SCIPcreateOrigSol(scip, &sol, NULL) );
	
	double* vals;
	SCIP_CALL( SCIPallocBufferArray(scip, &vals, gmoN(gmo)) );
	gmoGetVarL(gmo, vals);
	
	SCIP_CALL( SCIPsetSolVals(scip, sol, gmoN(gmo), vars, vals) );
	
	SCIPfreeBufferArray(scip, &vals);
	
	SCIP_Bool stored;
	SCIP_CALL( SCIPtrySolFree(scip, &sol, TRUE, TRUE, TRUE, &stored) );
	
	if (stored)
		gevLog(gev, "Feasible initial solution used to initialize primal bound.");
	
	return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::processLPSolution(double time) {
	assert(lpi != NULL);
	
	SCIP_Bool primalfeasible = SCIPlpiIsPrimalFeasible(lpi);

	gevLogPChar(gev, "\nSolution status: ");
	
	if (SCIPlpiIsPrimalUnbounded(lpi)) {
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, primalfeasible ? ModelStat_Unbounded : ModelStat_UnboundedNoSolution);
		gevLog(gev, "Problem is unbounded.");
	} else if (SCIPlpiIsPrimalInfeasible(lpi)) {
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
		gevLog(gev, "Problem is infeasible.");
	} else if (SCIPlpiIsOptimal(lpi)) {
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
		gevLog(gev, "Problem solved to optimality.");
	} else if (SCIPlpiIsIterlimExc(lpi)) {
		gmoSolveStatSet(gmo, SolveStat_Iteration);
		gmoModelStatSet(gmo, primalfeasible ? ModelStat_NonOptimalIntermed : ModelStat_InfeasibleIntermed);
		gevLog(gev, "Iteration limit exceeded.");
	} else if (SCIPlpiIsTimelimExc(lpi)) {
		gmoSolveStatSet(gmo, SolveStat_Resource);
		gmoModelStatSet(gmo, primalfeasible ? ModelStat_NonOptimalIntermed : ModelStat_InfeasibleIntermed);
		gevLog(gev, "Time limit exceeded.");
	} else if (SCIPlpiIsObjlimExc(lpi)) {
		gmoSolveStatSet(gmo, SolveStat_Solver);
		gmoModelStatSet(gmo, primalfeasible ? ModelStat_NonOptimalIntermed : ModelStat_InfeasibleIntermed);
		gevLog(gev, "Objective limit exceeded.");
	} else if ((gmoN(gmo)==0 || gmoM(gmo)==0) && primalfeasible) { // claim that a more-or-less empty problem was solved to optimality
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
		gevLog(gev, "Problem probably solved to optimality.");
	} else { // don't have idea about status
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, primalfeasible ? ModelStat_NonOptimalIntermed : ModelStat_InfeasibleIntermed);
		gevLog(gev, "Status unknown.");
	}

	int iter;
	SCIPlpiGetIterations(lpi, &iter);
	gmoSetHeadnTail(gmo, Hiterused, iter);
	
	if (primalfeasible && gmoN(gmo)) {
		double optval;
		double* collev  = new double[gmoN(gmo)];
		double* colmarg = new double[gmoN(gmo)];
		double* rowlev  = new double[gmoM(gmo)];
		double* rowmarg = new double[gmoM(gmo)];
		int* rowbasstat = new int[gmoM(gmo)];
		int* rowindic   = new int[gmoM(gmo)];
		int* colbasstat = new int[gmoN(gmo)];
		int* colindic   = new int[gmoN(gmo)];

		SCIPlpiGetSol(lpi, &optval, collev, rowmarg, rowlev, colmarg);
		SCIPlpiGetBase(lpi, colbasstat, rowbasstat);
		
		for (int i = 0; i < gmoN(gmo); ++i) {
//			if (gmoGetVarTypeOne(gmo, i) != var_X)
//				colbasstat[i] = SMAG_BASSTAT_SUPERBASIC;
//			else
			switch(colbasstat[i]) {
				case SCIP_BASESTAT_LOWER: colbasstat[i] = Bstat_Lower; break;
				case SCIP_BASESTAT_UPPER: colbasstat[i] = Bstat_Upper; break;
				case SCIP_BASESTAT_BASIC: colbasstat[i] = Bstat_Basic; break;
				case SCIP_BASESTAT_ZERO:  
				default:                  colbasstat[i] = Bstat_Super; break;
			}			
			colindic[i] = Cstat_OK;
		}
		
		for (int i = 0; i < gmoM(gmo); ++i) {
			switch(rowbasstat[i]) {
				case SCIP_BASESTAT_LOWER: rowbasstat[i] = Bstat_Lower; break;
				case SCIP_BASESTAT_UPPER: rowbasstat[i] = Bstat_Upper; break;
				case SCIP_BASESTAT_BASIC: rowbasstat[i] = Bstat_Basic; break;
				case SCIP_BASESTAT_ZERO:  
				default:                  rowbasstat[i] = Bstat_Super; break;
			}
			rowindic[i] = Cstat_OK;
		}
		
		gmoSetHeadnTail(gmo, HobjVal, optval + gmoObjConst(gmo));
		gmoSetSolution8(gmo, collev, colmarg, rowmarg, rowlev, colbasstat, colindic, rowbasstat, rowindic);

		delete[] collev;
		delete[] colmarg;
		delete[] rowmarg;
		delete[] rowlev;
		delete[] colbasstat;
		delete[] rowbasstat;
		delete[] colindic;
		delete[] rowindic;
	} else if (gmoN(gmo) == 0) {
		gmoSetHeadnTail(gmo, HobjVal, gmoObjConst(gmo));
		
		double* rowmarg = new double[gmoM(gmo)];
		memset(rowmarg, 0, gmoM(gmo)*sizeof(double));
		gmoSetSolution2(gmo, rowmarg, rowmarg);
		delete[] rowmarg;
	}

	gmoSetHeadnTail(gmo, HresUsed, time);

	return SCIP_OKAY;	
}

SCIP_RETCODE GamsScip::processMIQCPSolution() {
	assert(scip != NULL);
	
	int nrsol = SCIPgetNSols(scip);

	switch (SCIPgetStatus(scip)) {
		default:
		case SCIP_STATUS_UNKNOWN: // the solving status is not yet known
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			break;
		case SCIP_STATUS_USERINTERRUPT: // the user interrupted the solving process (by pressing Ctrl-C)
			gmoSolveStatSet(gmo, SolveStat_User);
			gmoModelStatSet(gmo, nrsol ? (gmoNDisc(gmo) ? ModelStat_Integer : ModelStat_OptimalLocal) : ModelStat_NoSolutionReturned);
			break;
		case SCIP_STATUS_NODELIMIT: // the solving process was interrupted because the node limit was reached
		case SCIP_STATUS_STALLNODELIMIT: // the solving process was interrupted because the node limit was reached
			gmoSolveStatSet(gmo, SolveStat_Iteration);
			gmoModelStatSet(gmo, nrsol ? (gmoNDisc(gmo) ? ModelStat_Integer : ModelStat_OptimalLocal) : ModelStat_NoSolutionReturned);
			break;
		case SCIP_STATUS_TIMELIMIT: // the solving process was interrupted because the time limit was reached
		case SCIP_STATUS_MEMLIMIT: // the solving process was interrupted because the memory limit was reached
			gmoSolveStatSet(gmo, SolveStat_Resource);
			gmoModelStatSet(gmo, nrsol ? (gmoNDisc(gmo) ? ModelStat_Integer : ModelStat_OptimalLocal) : ModelStat_NoSolutionReturned);
			break;
		case SCIP_STATUS_GAPLIMIT: // the solving process was interrupted because the gap limit was reached
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, nrsol ? (SCIPgetGap(scip) > 0.0 ? (gmoNDisc(gmo) ? ModelStat_Integer : ModelStat_OptimalLocal) : ModelStat_OptimalGlobal): ModelStat_NoSolutionReturned);
			break;
		case SCIP_STATUS_SOLLIMIT: // the solving process was interrupted because the solution limit was reached
		case SCIP_STATUS_BESTSOLLIMIT: // the solving process was interrupted because the solution improvement limit was reached
			gmoSolveStatSet(gmo, SolveStat_Resource);
			gmoModelStatSet(gmo, nrsol ? (gmoNDisc(gmo) ? ModelStat_Integer : ModelStat_OptimalLocal) : ModelStat_NoSolutionReturned);
			break;
		case SCIP_STATUS_OPTIMAL: // the problem was solved to optimality, an optimal solution is available
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
			break;
		case SCIP_STATUS_INFEASIBLE: // the problem was proven to be infeasible
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
			break;
		case SCIP_STATUS_UNBOUNDED: // the problem was proven to be unbounded
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, nrsol ? ModelStat_Unbounded : ModelStat_UnboundedNoSolution);
			break;
		case SCIP_STATUS_INFORUNBD: // the problem was proven to be either infeasible or unbounded
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			break;
	}

	// if the scip shell was used, the user might have done nasty things with the problem, so make sure that we do not crash here 
	if (nrsol && gmoN(gmo) <= SCIPgetNOrigVars(scip)) {
		SCIP_SOL* sol = SCIPgetBestSol(scip);
		assert(sol != NULL);
		
		double* collev = new double[gmoN(gmo)];
		double* lambda = new double[gmoM(gmo)];
		memset(lambda, 0, gmoM(gmo)*sizeof(double));
		
		SCIP_CALL( SCIPgetSolVals(scip, sol, gmoN(gmo), vars, collev) );
		gmoSetSolution2(gmo, collev, lambda);
		
		delete[] collev;
		delete[] lambda;
		
		gmoSetHeadnTail(gmo, HobjVal, SCIPgetSolOrigObj(scip, sol));
	}
	
	gmoSetHeadnTail(gmo, Tmipbest, SCIPgetDualbound(scip));
	gmoSetHeadnTail(gmo, Tmipnod,  (int)SCIPgetNNodes(scip));
	gmoSetHeadnTail(gmo, HresUsed, SCIPgetSolvingTime(scip));

	return SCIP_OKAY;
}

bool GamsScip::isLP() {
	if (gmoModelType(gmo) == Proc_lp)
		return true;
	if (gmoModelType(gmo) == Proc_rmip)
		return true;
	if (gmoNDisc(gmo))
		return false;
	if (gmoNLNZ(gmo))
		return false;
	if (gmoObjNLNZ(gmo))
		return false;
	int numSos1, numSos2, nzSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	if (nzSos)
		return false;
	return true;
}

DllExport GamsScip* STDCALL createNewGamsScip() {
	return new GamsScip();
}

DllExport int STDCALL scpCallSolver(scpRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsScip*)Cptr)->callSolver();
}

DllExport int STDCALL scpModifyProblem(scpRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsScip*)Cptr)->modifyProblem();
}

DllExport int STDCALL scpHaveModifyProblem(scpRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsScip*)Cptr)->haveModifyProblem();
}

DllExport int STDCALL scpReadyAPI(scpRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr) {
	gevHandle_t Eptr;
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	if (!gevGetReady(msg, sizeof(msg)))
		return 1;
	Eptr = (gevHandle_t) gmoEnvironment(Gptr);
	gevLogStatPChar(Eptr, ((GamsScip*)Cptr)->getWelcomeMessage());
	return ((GamsScip*)Cptr)->readyAPI(Gptr, Optr);
}

DllExport void STDCALL scpFree(scpRec_t **Cptr) {
	assert(Cptr != NULL);
	delete (GamsScip*)*Cptr;
	*Cptr = NULL;
}

DllExport void STDCALL scpCreate(scpRec_t **Cptr, char *msgBuf, int msgBufLen) {
	assert(Cptr != NULL);
	*Cptr = (scpRec_t*) new GamsScip();
	if (msgBufLen && msgBuf)
		msgBuf[0] = 0;
}
