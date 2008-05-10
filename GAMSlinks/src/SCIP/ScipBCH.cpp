// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsScip.cpp 420 2008-03-27 12:53:42Z stefan $
//
// Author: Stefan Vigerske

#include "ScipBCH.hpp"

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

// for INT_MAX
#ifdef HAVE_CLIMITS
#include <climits>
#else
#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#error "don't have header file for limits"
#endif
#endif


#include "GamsDictionary.hpp"

extern "C" {
#include "scip/sol.h"
#ifndef NDEBUG
#include "scip/struct_scip.h"
#include "scip/struct_sol.h"
#endif
}

/** Data of separator.
 * Also used by event handler.
 */
struct SCIP_SepaData {
	GamsBCH* bch;
	smagHandle_t smag;
	SCIP_VAR*** vars;
	
	/** Scratch space to store node solution.
	 */
	double* x;
	/** Scratch space to store node lower bound.
	 */
	double* lb;
	/** Scratch space to store node upper bound.
	 */
	double* ub;
	
	/** Remember the heuristic pointer.
	 */
	SCIP_HEUR* heur;
};

/** Callback routine for cutting plane generation to cut off an LP solution.
 */
SCIP_DECL_SEPAEXECLP(sepaExeclpBCH);

/** Initialization of event handler.
 */
SCIP_DECL_EVENTINIT(eventInitNewInc);
/** Deinitialization of event handler.
 */
SCIP_DECL_EVENTEXIT(eventExitNewInc);
/** Callback routine when a new incumbent is found.
 */
SCIP_DECL_EVENTEXEC(eventExecNewInc);

/** Initialization of heuristic.
 */
SCIP_DECL_HEURINIT(heurInitBCH);
/** Deinitialization of heuristic.
 */
SCIP_DECL_HEUREXIT(heurExitBCH);
/** Execution method of heuristic.
 */
SCIP_DECL_HEUREXEC(heurExecBCH);

/** Callback routine for output into display column.
 */ 
SCIP_DECL_DISPOUTPUT(dispOutputBCH);

SCIP_RETCODE BCHaddParam(SCIP* scip) {
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usercutcall", "The GAMS command line to call the cut generator", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutfirst", "Calls the cut generator for the first n nodes", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutfreq", "Determines the frequency of the cut generator model calls", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutinterval", "Determines the interval when to apply the multiplier for the frequency of the cut generator model calls", NULL, FALSE, 100, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutmult", "Determines the multiplier for the frequency of the cut generator model calls", NULL, FALSE, 2, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/usercutnewint", "Calls the cut generator if the solver found a new integer feasible solution", NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutpriority", "The priority of the cut generator in SCIP", NULL, FALSE, 0, -INT_MAX, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxin", "The name of the GDX file read back into SCIP", NULL, FALSE, "bchin.gdx", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxname", "The name of the GDX file exported from the solver with the solution at the node", NULL, FALSE, "bchout.gdx", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxnameinc", "The name of the GDX file exported from the solver with the incumbent solution", NULL, FALSE, "bchout_i.gdx", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxprefix", "Prefixes usergdxin, usergdxname, and usergdxnameinc", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/userheurcall", "The GAMS command line to call the heuristic", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurfirst", "Calls the heuristic for the first n nodes", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurfreq", "Determines the frequency of the heuristic model calls", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurinterval", "Determines the interval when to apply the multiplier for the frequency of the heuristic model calls", NULL, FALSE, 100, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurmult", "Determines the multiplier for the frequency of the heuristic model calls", NULL, FALSE, 2, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/userheurnewint", "Calls the heuristic if the solver found a new integer feasible solution", NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurobjfirst", "Calls the heuristic if the LP value of the node is closer to the best bound than the current incumbent", NULL, FALSE, 0, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurpriority", "The priority of the heuristic in SCIP", NULL, FALSE, 0, -INT_MAX, INT_MAX, NULL, NULL) );
//	SCIP_CALL( SCIPaddStringParam(scip, "gams/userincbcall", "The GAMS command line to call the incumbent checker", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/userincbicall", "The GAMS command line to call the incumbent reporter", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/userkeep", "Calls gamskeep instead of gams", NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/userjobid", "Postfixes gdxname, gdxnameinc, and gdxin", NULL, FALSE, "", NULL, NULL) );	
	
	return SCIP_OKAY;
}

SCIP_RETCODE BCHsetup(SCIP* scip, SCIP_VAR*** vars, smagHandle_t prob, GamsHandler& gamshandler, GamsDictionary& gamsdict, GamsBCH*& bch, void*& bchdata) {
	bch = NULL;
	bchdata = NULL;
	if (!gamsdict.haveNames() && !gamsdict.readDictionary()) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Need dictionary to enable BCH.");
		smagStdOutputFlush(prob, SMAG_ALLMASK);
		return SCIP_ERROR;
	}

	bch=new GamsBCH(gamshandler, gamsdict);

	char* s = NULL;
	SCIP_Bool b;
	int i;
	
	bool have_cutcall, have_heurcall, /* have_incbcall, */ have_incbicall;
	int cutpriority, heurpriority;
	
	char* gdxprefix = NULL; //new char[1024];
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxprefix", &gdxprefix) );

	SCIP_CALL( SCIPgetStringParam(scip, "gams/userjobid", &s) );
	bch->set_userjobid(s);
	
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxname", &s) );
	bch->set_usergdxname(s, gdxprefix);
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxnameinc", &s) );
	bch->set_usergdxnameinc(s, gdxprefix);
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxin", &s) );
	bch->set_usergdxin(s, gdxprefix);
	SCIP_CALL( SCIPgetBoolParam(scip, "gams/userkeep", &b) );
	bch->set_userkeep(b);

	SCIP_CALL( SCIPgetStringParam(scip, "gams/usercutcall", &s) );
	bch->set_usercutcall(s);
	have_cutcall=*s;
	SCIP_CALL( SCIPgetIntParam(scip, "gams/usercutfreq", &i) );
	bch->set_usercutfreq(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/usercutinterval", &i) );
	bch->set_usercutinterval(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/usercutmult", &i) );
	bch->set_usercutmult(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/usercutfirst", &i) );
	bch->set_usercutfirst(i);
	SCIP_CALL( SCIPgetBoolParam(scip, "gams/usercutnewint", &b) );
	bch->set_usercutnewint(b);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/usercutpriority", &cutpriority) );

	SCIP_CALL( SCIPgetStringParam(scip, "gams/userheurcall", &s) );
	bch->set_userheurcall(s);
	have_heurcall=*s;
	SCIP_CALL( SCIPgetIntParam(scip, "gams/userheurfreq", &i) );
	bch->set_userheurfreq(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/userheurinterval", &i) );
	bch->set_userheurinterval(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/userheurmult", &i) );
	bch->set_userheurmult(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/userheurfirst", &i) );
	bch->set_userheurfirst(i);
	SCIP_CALL( SCIPgetBoolParam(scip, "gams/userheurnewint", &b) );
	bch->set_userheurnewint(b);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/userheurobjfirst", &i) );
	bch->set_userheurobjfirst(i);
	SCIP_CALL( SCIPgetIntParam(scip, "gams/userheurpriority", &heurpriority) );

//	SCIP_CALL( SCIPgetStringParam(scip, "gams/userincbcall", &s) );
//	bch->set_userincbcall(s);
//	have_incbcall=*s;
	SCIP_CALL( SCIPgetStringParam(scip, "gams/userincbicall", &s) );
	bch->set_userincbicall(s);
	have_incbicall=*s;

//	bch->printParameters();
	
	// could be moved into eventInitNewInc so that we can report global bounds after preprocessing
	bch->setGlobalBounds(prob->colLB, prob->colUB);

	SCIP_SEPADATA* sepadata = new SCIP_SEPADATA;
  sepadata->bch=bch;
  sepadata->smag=prob;
  sepadata->vars=vars;
  sepadata->heur = NULL;
  int n = smagColCount(sepadata->smag);
  sepadata->x  = new double[n];
  sepadata->lb = new double[n];
  sepadata->ub = new double[n];
  
  bchdata = sepadata;
	
	if (have_cutcall) {
  	SCIP_CALL( SCIPincludeSepa(scip, "GamsBCH", "Gams BCH cut generator", cutpriority /*priority*/, 1 /* frequency */, 1.0 /* maxdistance to dual bound */, FALSE /* delayed */,
 			NULL, NULL, NULL, NULL, NULL, sepaExeclpBCH, NULL, sepadata) );
	}
	if (have_heurcall) {
		SCIP_CALL( SCIPincludeHeur(scip, "GamsBCH", "GAMS BCH heuristic", 'G', heurpriority /* priority */, 1 /* frequency */, 1 /* frequency offset */, -1 /* maxdepth */, SCIP_HEURTIMING_AFTERLPLOOP /* timingmask */,
			NULL, heurInitBCH, heurExitBCH, NULL, NULL, heurExecBCH, (SCIP_HEURDATA*)sepadata) );
	}
	
	SCIP_CALL( SCIPincludeEventhdlr(scip, "GamsBCH", "Gams BCH incumbent updater", NULL, eventInitNewInc, eventExitNewInc, NULL, NULL, NULL, eventExecNewInc, (SCIP_EVENTHDLRDATA*)sepadata) );

	const char* dispheader;
	if (have_cutcall && !have_heurcall) {
		dispheader = " BCH cuts";
	} else if (have_heurcall && !have_cutcall) {
		dispheader = " BCH sols";
	} else if (have_cutcall && have_heurcall) {
		dispheader = " BCH cut sol";
	} else { // have only incumbent reporter or checker
		dispheader = " BCH";		
	}
		
	SCIP_CALL( SCIPincludeDisp(scip, "GamsBCH", "Gams BCH display column", dispheader, SCIP_DISPSTATUS_ON,
			NULL, NULL, NULL, NULL, NULL, dispOutputBCH, (SCIP_DISPDATA*)sepadata, strlen(dispheader) /* width */, 110000 /* priority */, 30100 /* rel. position */, TRUE) );

	bch->setLogLevel(0);
	
	return SCIP_OKAY;
}

SCIP_RETCODE BCHcleanup(smagHandle_t prob, GamsBCH*& bch, void*& bchdata) {
	delete (SCIP_SEPADATA*)bchdata; bchdata = NULL;
	delete bch; bch = NULL;
	
	return SCIP_OKAY;
}

SCIP_RETCODE BCHsetNodeSolution(SCIP* scip, SCIP_SEPADATA* sepadata) {
  int n = smagColCount(sepadata->smag);
  double*  x = sepadata->x;
  double* lb = sepadata->lb;
  double* ub = sepadata->ub;
	SCIP_VAR** origvars = *sepadata->vars;

//	SCIP_VAR** transvars = new SCIP_VAR*[n];
//	SCIPgetTransformedVars(scip, norigvars, origvars, transvars);
	
	int ntransvars;
	SCIP_VAR** transvars = NULL;
	SCIP_CALL( SCIPgetVarsData(scip, &transvars, &ntransvars, NULL, NULL, NULL, NULL) );

	// get current LP solution and transform into original space
	SCIP_SOL* sol = NULL;
	SCIP_CALL( SCIPcreateSol(scip, &sol, NULL) );
	for (int i=0; i<ntransvars; ++i) {
		SCIP_CALL( SCIPsetSolVal(scip, sol, transvars[i], SCIPvarGetLPSol(transvars[i])) );
//		printf("%g ", SCIPvarGetLPSol(transvars[i]));
	}
	SCIP_CALL( SCIPsolRetransform (sol, scip->set, scip->stat, scip->origprob) );

	for (int i=0; i<n; ++i)
		x[i] = SCIPgetSolVal(scip, sol, origvars[i]);
	double objval;
	smagEvalObjFunc(sepadata->smag, x, &objval);

	// transform local node bounds back into original space
	for (int i=0; i<n; ++i) {
		SCIP_VAR* var = origvars[i];
		switch (SCIPvarGetStatus(var)) {
			case SCIP_VARSTATUS_ORIGINAL: // variable belongs to original problem
				lb[i] = SCIPvarGetLbLocal(SCIPvarGetTransVar(var));
				ub[i] = SCIPvarGetUbLocal(SCIPvarGetTransVar(var));
				break;
			case SCIP_VARSTATUS_LOOSE: // variable is a loose variable of the transformed problem   (this cannot be the case for an original variable, or?)
			case SCIP_VARSTATUS_COLUMN: // variable is a column of the transformed problem          (this cannot be the case for an original variable, or?)
				lb[i] = SCIPvarGetLbLocal(var);
				ub[i] = SCIPvarGetUbLocal(var);
				break;
			case SCIP_VARSTATUS_FIXED: // variable is fixed to specific value in the transformed problem -> let's take this value from the LP solution
				lb[i] = x[i];
				ub[i] = x[i];
				break;
			case SCIP_VARSTATUS_AGGREGATED: { // variable is aggregated to x = a*y + c in the transformed problem
				SCIP_VAR* avar = SCIPvarGetAggrVar(var);
				double alb = SCIPvarGetLbLocal(avar);
				double aub = SCIPvarGetUbLocal(avar);
				double ascalar = SCIPvarGetAggrScalar(var);
				if (ascalar > 0.) {
					lb[i] = SCIPisInfinity(scip, -alb) ? -SCIPinfinity(scip) : ascalar * alb + SCIPvarGetAggrConstant(var);
					ub[i] = SCIPisInfinity(scip,  aub) ?  SCIPinfinity(scip) : ascalar * aub + SCIPvarGetAggrConstant(var);
				} else {
					lb[i] = SCIPisInfinity(scip,  aub) ? -SCIPinfinity(scip) : ascalar * aub + SCIPvarGetAggrConstant(var);
					ub[i] = SCIPisInfinity(scip, -alb) ?  SCIPinfinity(scip) : ascalar * alb + SCIPvarGetAggrConstant(var);
				}
			} break;
			case SCIP_VARSTATUS_MULTAGGR: { // variable is aggregated to x = a_1*y_1 + ... + a_k*y_k + c
				int navars = SCIPvarGetMultaggrNVars(var);
				SCIP_VAR** avars = SCIPvarGetMultaggrVars(var);
				double* ascalars = SCIPvarGetMultaggrScalars(var);
				double& l(lb[i]);
				double& u(ub[i]);
				l = u = SCIPvarGetMultaggrConstant(var);
				double alb, aub;
				for (int j=0; j<navars && (!SCIPisInfinity(scip, l) || !SCIPisInfinity(scip, u)); ++j, ++ascalars, ++avars) {
					alb = SCIPvarGetLbLocal(*avars);
					aub = SCIPvarGetUbLocal(*avars);
					if (*ascalars > 0.) {
						if (!SCIPisInfinity(scip, -l))
							l = SCIPisInfinity(scip, -alb) ? -SCIPinfinity(scip) : (l+*ascalars*alb);
						if (!SCIPisInfinity(scip,  u))
							u = SCIPisInfinity(scip,  aub) ?  SCIPinfinity(scip) : (u+*ascalars*aub);
					} else {
						if (!SCIPisInfinity(scip, -lb[i]))
							l = SCIPisInfinity(scip,  aub) ? -SCIPinfinity(scip) : (l+*ascalars*aub);
						if (!SCIPisInfinity(scip,  ub[i]))
							u = SCIPisInfinity(scip, -alb) ?  SCIPinfinity(scip) : (u+*ascalars*alb);
					}
				}
			} break;
			case SCIP_VARSTATUS_NEGATED: { // variable is the negation of an original or transformed variable
				double offset = SCIPvarGetNegationConstant(var);
				SCIP_Var* tvar = SCIPvarGetNegationVar(var);
				double tlb = SCIPvarGetLbLocal(tvar);
				double tub = SCIPvarGetUbLocal(tvar);
				if (SCIPisInfinity(scip, -tlb)) // lower bound in transformed problem is -infty -> upper bound is +infty
					ub[i] = SCIPinfinity(scip);
				else
					ub[i] = offset-tlb;
				if (SCIPisInfinity(scip,  tub)) // upper bound in transformed problem is +infty -> lower bound is -infty
					lb[i] = -SCIPinfinity(scip);
				else
					lb[i] = offset-tub;
			} break;
			default: {
				smagStdOutputPrint(sepadata->smag, SMAG_ALLMASK, "GamsBCH: Retransformation of node lower and upper bounds failed: Wrong variable type!");
				return SCIP_ERROR;
			}
		}
		assert(lb[i]>=sepadata->smag->colLB[i]);
		assert(ub[i]<=sepadata->smag->colUB[i]);
	}
	
//	for (int i=0; i<norigvars; ++i)
//		printf("node values: \tlb: %g \tval: %g \tub: %g\t\torig lb: %g \torig ub: %g\n", lb[i], x[i], ub[i], sepadata->smag->colLB[i], sepadata->smag->colUB[i]);
	
	sepadata->bch->setNodeSolution(x, objval, lb, ub);
	
	return SCIP_OKAY;
}

SCIP_RETCODE BCHgenerateCuts(SCIP* scip, SCIP_SEPADATA* sepadata, SCIP_RESULT* result) {
  *result = SCIP_DIDNOTRUN;

  if (!sepadata->bch->doCuts())
		return SCIP_OKAY;
  
  SCIP_CALL( BCHsetNodeSolution(scip, sepadata) );

	std::vector<GamsBCH::Cut> cuts;
	if (!sepadata->bch->generateCuts(cuts) || !cuts.size()) { // message already printed in GamsBCH
		*result = SCIP_DIDNOTFIND;
		return SCIP_OKAY;
	}
	
	for(std::vector<GamsBCH::Cut>::iterator cuts_it(cuts.begin()); cuts_it!=cuts.end(); ++cuts_it) {
		GamsBCH::Cut& cut(*cuts_it);
		SCIP_ROW* row;
		//FIXME: currently we assume that the cuts received via BCH are valid globally; we hope, that in the future BCH will tell us more about it 
		SCIP_CALL( SCIPcreateEmptyRow(scip, &row, "GamsBCH cut", cut.lb, cut.ub, FALSE /* locally */, TRUE /* modifiable */, TRUE /* removable */ ) );
		
		for (int i=0; i<cut.nnz; ++i) {
			SCIP_VAR* var = (*sepadata->vars)[cut.indices[i]];
			switch (SCIPvarGetStatus(var)) {
				case SCIP_VARSTATUS_ORIGINAL: // variable belongs to original problem
					SCIP_CALL( SCIPaddVarToRow(scip, row, SCIPvarGetTransVar(var), cut.coeff[i]) );
					break;
				case SCIP_VARSTATUS_LOOSE: // variable is a loose variable of the transformed problem   (this cannot be the case for an original variable, or?)
				case SCIP_VARSTATUS_COLUMN: // variable is a column of the transformed problem          (this cannot be the case for an original variable, or?)
					SCIP_CALL( SCIPaddVarToRow(scip, row, var, cut.coeff[i]) );
					break;
				case SCIP_VARSTATUS_FIXED: // variable is fixed to specific value in the transformed problem -> modify lb and ub of row
					if (!SCIPisInfinity(scip, -cut.lb)) cut.lb -= cut.coeff[i]*sepadata->lb[cut.indices[i]];
					if (!SCIPisInfinity(scip,  cut.ub)) cut.ub -= cut.coeff[i]*sepadata->lb[cut.indices[i]];
					break;
				case SCIP_VARSTATUS_AGGREGATED: { // variable is aggregated to x = a*y + c in the transformed problem -> y gets coefficient a*coeff[i], lb and ub are reduced by c*coeff[i]
					SCIP_CALL( SCIPaddVarToRow(scip, row, SCIPvarGetAggrVar(var), cut.coeff[i]*SCIPvarGetAggrScalar(var)) );
					double constant = SCIPvarGetAggrConstant(var);
					if (constant) {
						if (!SCIPisInfinity(scip, -cut.lb)) cut.lb -= cut.coeff[i]*constant;
						if (!SCIPisInfinity(scip,  cut.ub)) cut.ub -= cut.coeff[i]*constant;
					}
				} break;
				case SCIP_VARSTATUS_MULTAGGR: { // variable is aggregated to x = a_1*y_1 + ... + a_k*y_k + c -> y_j get coefficients a_j*coeff[i], lb and ub are reduced by c*coeff[i]
					int navars = SCIPvarGetMultaggrNVars(var);
					SCIP_VAR** avars = SCIPvarGetMultaggrVars(var);
					double* ascalars = SCIPvarGetMultaggrScalars(var);
					for (int j=0; j<navars; ++j, ++ascalars, ++avars)
						SCIP_CALL( SCIPaddVarToRow(scip, row, *avars, cut.coeff[i]**ascalars) );

					double constant = SCIPvarGetMultaggrConstant(var);
					if (constant) {
						if (!SCIPisInfinity(scip, -cut.lb)) cut.lb -= cut.coeff[i]*constant;
						if (!SCIPisInfinity(scip,  cut.ub)) cut.ub -= cut.coeff[i]*constant;
					}
				} break;
				case SCIP_VARSTATUS_NEGATED: { // variable is the negation of an original or transformed variable
					SCIPaddVarToRow(scip, row, SCIPvarGetNegationVar(var), -cut.coeff[i]);
					double offset = SCIPvarGetNegationConstant(var);
					if (offset) {
						if (!SCIPisInfinity(scip, -cut.lb)) cut.lb -= cut.coeff[i]*offset;
						if (!SCIPisInfinity(scip,  cut.ub)) cut.ub -= cut.coeff[i]*offset;
					}
				} break;
				default: {
					smagStdOutputPrint(sepadata->smag, SMAG_ALLMASK, "GamsBCH: Transformation of cut failed: Wrong variable type!");
					return SCIP_ERROR;
				}				
			}
		}
		SCIP_CALL( SCIPchgRowLhs(scip, row, cut.lb) );
		SCIP_CALL( SCIPchgRowRhs(scip, row, cut.ub) );
		
//		SCIP_CALL( SCIPprintRow(scip, row, stdout) );
		SCIP_CALL( SCIPaddCut(scip, NULL, row, TRUE /* forcecut */) );
	}

	*result = SCIP_SEPARATED;

	return SCIP_OKAY;
}

SCIP_RETCODE BCHrunHeuristic(SCIP* scip, SCIP_SEPADATA* data, SCIP_HEUR* heur, SCIP_RESULT* result) {
	*result = SCIP_DIDNOTRUN;
	
	double objoffset = data->smag->gms.grhs[data->smag->gms.slplro-1] * data->smag->gObjFactor;
	
	if (!data->bch->doHeuristic(SCIPgetPrimalbound(scip)-objoffset, SCIPgetLocalDualbound(scip)-objoffset)) 
		return SCIP_OKAY;
	
	SCIP_CALL( BCHsetNodeSolution(scip, data) );
	
	double objval;
	if (data->bch->runHeuristic(data->x, objval)) { // success
		SCIP_SOL* sol;
		SCIP_CALL( SCIPcreateOrigSol(scip, &sol, heur) );
		SCIP_CALL( SCIPsetSolVals(scip, sol, smagColCount(data->smag), *data->vars, data->x) );
		
		SCIP_Bool stored;
		SCIP_CALL( SCIPtrySolFree(scip, &sol, TRUE, TRUE, TRUE, &stored) );
		*result = stored ? SCIP_FOUNDSOL : SCIP_DIDNOTFIND;
	} else { // failed
		*result=SCIP_DIDNOTFIND;
	}

	return SCIP_OKAY;
}

SCIP_DECL_SEPAEXECLP(sepaExeclpBCH) {
  return BCHgenerateCuts(scip, SCIPsepaGetData(sepa), result);
}

SCIP_DECL_EVENTINIT(eventInitNewInc) {
	SCIP_CALL( SCIPcatchEvent(scip, SCIP_EVENTTYPE_BESTSOLFOUND, eventhdlr, NULL, NULL) );
	return SCIP_OKAY;
}

SCIP_DECL_EVENTEXIT(eventExitNewInc) {
	SCIP_CALL( SCIPdropEvent(scip, SCIP_EVENTTYPE_BESTSOLFOUND, eventhdlr, NULL, -1) );
	return SCIP_OKAY;
}

SCIP_DECL_EVENTEXEC(eventExecNewInc) {
	SCIP_SEPADATA* data = (SCIP_SEPADATA*)SCIPeventhdlrGetData(eventhdlr);
	assert(data);
	assert(data->x);
	
	int n = smagColCount(data->smag);
	SCIP_VAR** origvars = *data->vars;

	SCIP_SOL* sol = SCIPeventGetSol(event);
	assert(sol);
	
	for (int i=0; i<n; ++i)
		data->x[i] = SCIPsolGetVal(sol, scip->set, scip->stat, origvars[i]);
	double objval;
	smagEvalObjFunc(data->smag, data->x, &objval);
	
	bool accepted=data->bch->setIncumbentSolution(data->x, objval);
	
	if (!accepted){ // reject solution
		// TODO
	}

	if (data->bch->get_usercutnewint() && *data->bch->get_usercutcall()) {
		SCIP_RESULT result;
		SCIP_CALL( BCHgenerateCuts(scip, data, &result) );
	}
	if (data->heur && data->bch->get_userheurnewint() && SCIPsolGetHeur(sol)!=data->heur) {
		SCIP_RESULT result;
		SCIP_CALL( BCHrunHeuristic(scip, data, data->heur, &result) );
	}
	
	return SCIP_OKAY;
}

/** Initialization of heuristic.
 */
SCIP_DECL_HEURINIT(heurInitBCH) {
	SCIP_SEPADATA* data = (SCIP_SEPADATA*)SCIPheurGetData(heur);
	assert(data != NULL);

  data->heur = heur;
  
  return SCIP_OKAY;
}

/** Deinitialization of heuristic.
 */
SCIP_DECL_HEUREXIT(heurExitBCH) {
	SCIP_SEPADATA* data = (SCIP_SEPADATA*)SCIPheurGetData(heur);

	data->heur = NULL;

	return SCIP_OKAY;
}

/** Execution method of heuristic.
 */
SCIP_DECL_HEUREXEC(heurExecBCH) {
	SCIP_SEPADATA* data = (SCIP_SEPADATA*)SCIPheurGetData(heur);
	assert(data != NULL);
	
	return BCHrunHeuristic(scip, data, heur, result);
}

SCIP_DECL_DISPOUTPUT(dispOutputBCH) {
	SCIP_SEPADATA* data = (SCIP_SEPADATA*)SCIPdispGetData(disp);
	assert(data != NULL);
	
	bool have_cutcall=*data->bch->get_usercutcall();
	
	if (!data->heur) { // only cutcallback   " BCH cuts"
	  SCIPinfoMessage(scip, file, "%4d %4d", data->bch->getNumCalls(), data->bch->getNumCuts());
	} else if (!have_cutcall) { // only heuristic   " BCH sols"
	  SCIPinfoMessage(scip, file, "%4d %4d", data->bch->getNumCalls(), data->bch->getNumSols());
	} else if (data->heur && have_cutcall) { // have both heuristic and cut callbacks " BCH cut sol"
	  SCIPinfoMessage(scip, file, "%4d %3d %3d", data->bch->getNumCalls(), data->bch->getNumCuts(), data->bch->getNumSols());
	} else { // have only incumbent reporter or checker   " BCH"
	  SCIPinfoMessage(scip, file, "%4d", data->bch->getNumCalls());
	}
	
	return SCIP_OKAY;
}
