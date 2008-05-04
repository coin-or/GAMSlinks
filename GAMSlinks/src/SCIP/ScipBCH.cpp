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

/** Callback routine for cutting plane generation.
 */
SCIP_DECL_SEPAEXECLP(sepaExeclpBCH);

struct SCIP_SepaData {
	GamsBCH* bch;
	smagHandle_t smag;
};


/** Initialization of event handler.
 */
SCIP_DECL_EVENTINIT(eventInitNewInc);
/** Deinitialization of event handler.
 */
SCIP_DECL_EVENTEXIT(eventExitNewInc);
/** Callback routine when a new incumbent is found.
 */
SCIP_DECL_EVENTEXEC(eventExecNewInc);

struct SCIP_EventhdlrData {
	GamsBCH* bch;
	smagHandle_t smag;
};

SCIP_RETCODE BCHaddParam(SCIP* scip) {
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usercutcall", "The GAMS command line to call the cut generator", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutfirst", "Calls the cut generator for the first n nodes", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutfreq", "Determines the frequency of the cut generator model calls", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutinterval", "Determines the interval when to apply the multiplier for the frequency of the cut generator model calls", NULL, FALSE, 100, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/usercutmult", "Determines the multiplier for the frequency of the cut generator model calls", NULL, FALSE, 2, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/usercutnewint", "Calls the cut generator if the solver found a new integer feasible solution", NULL, FALSE, TRUE, NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxin", "The name of the GDX file read back into SCIP", NULL, FALSE, "bchin.gdx", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxname", "The name of the GDX file exported from the solver with the solution at the node", NULL, FALSE, "bchout.gdx", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxnameinc", "The name of the GDX file exported from the solver with the incumbent solution", NULL, FALSE, "bchout_i.gdx", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/usergdxprefix", "Prefixes usergdxin, usergdxname, and usergdxnameinc", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/userheurcall", "The GAMS command line to call the heuristic", NULL, FALSE, "", NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurfirst", "Calls the heuristic for the first n nodes", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurfreq", "Determines the frequency of the heuristic model calls", NULL, FALSE, 10, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurinterval", "Determines the interval when to apply the multiplier for the frequency of the heuristic model calls", NULL, FALSE, 100, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurmult", "Determines the multiplier for the frequency of the heuristic model calls", NULL, FALSE, 2, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/userheurnewint", "Calls the heuristic if the solver found a new integer feasible solution", NULL, FALSE, TRUE, NULL, NULL) );
	SCIP_CALL( SCIPaddIntParam(scip, "gams/userheurobjfirst", "Calls the heuristic if the LP value of the node is closer to the best bound than the current incumbent", NULL, FALSE, 0, 0, INT_MAX, NULL, NULL) );
	SCIP_CALL( SCIPaddBoolParam(scip, "gams/userkeep", "Calls gamskeep instead of gams", NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL( SCIPaddStringParam(scip, "gams/userjobid", "Postfixes gdxname, gdxnameinc, and gdxin", NULL, FALSE, "", NULL, NULL) );	

//  userincbcall     .s.(def '')   The GAMS command line to call the incumbent checking program
//  userincbicall    .s.(def '')   The GAMS command line to call the incumbent reporting program
	
	return SCIP_OKAY;
}

SCIP_RETCODE BCHsetup(GamsBCH*& bch, smagHandle_t prob, GamsHandler& gamshandler, GamsDictionary& gamsdict, SCIP* scip) {
	if (!gamsdict.haveNames() && !gamsdict.readDictionary()) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Need dictionary to enable BCH.");
		smagStdOutputFlush(prob, SMAG_ALLMASK);
		return SCIP_ERROR;
	}

	bch=new GamsBCH(gamshandler, gamsdict);

	char* buffer = new char[1024];
	SCIP_Bool b;
	int i;
	
	bool have_cutcall, have_heurcall;
	
	char* gdxprefix = new char[1024];
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxprefix", &gdxprefix) ); // TODO: SCIP seem to overwrite *gdxprefix with an own pointer

	SCIP_CALL( SCIPgetStringParam(scip, "gams/userjobid", &buffer) );
	bch->set_userjobid(buffer);
	
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxname", &buffer) );
	bch->set_usergdxname(buffer, gdxprefix);
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxnameinc", &buffer) );
	bch->set_usergdxnameinc(buffer, gdxprefix);
	SCIP_CALL( SCIPgetStringParam(scip, "gams/usergdxin", &buffer) );
	bch->set_usergdxin(buffer, gdxprefix);
	SCIP_CALL( SCIPgetBoolParam(scip, "gams/userkeep", &b) );
	bch->set_userkeep(b);

	SCIP_CALL( SCIPgetStringParam(scip, "gams/usercutcall", &buffer) );
	bch->set_usercutcall(buffer);
	have_cutcall=*buffer;
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

	SCIP_CALL( SCIPgetStringParam(scip, "gams/userheurcall", &buffer) );
	bch->set_userheurcall(buffer);
	have_heurcall=*buffer;
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
	
	delete[] gdxprefix;
	delete[] buffer;
	
	bch->printParameters();
	
	bch->setGlobalBounds(prob->colLB, prob->colUB);

#define sepaFreeXxx NULL
#define sepaInitXxx NULL
#define sepaExitXxx NULL
#define sepaInitsolXxx NULL
#define sepaExitsolXxx NULL
#define sepaExecsolXxx NULL
	
	if (have_cutcall) {
	  SCIP_SEPADATA* sepadata;
	  SCIP_CALL( SCIPallocMemory(scip, &sepadata) );
	  sepadata->bch=bch;
	  sepadata->smag=prob;
		SCIP_CALL( SCIPincludeSepa(scip, "GamsBCH", "Gams BCH cut generator", 0 /*priority*/, 1 /* frequency */, 1.0 /* maxdistance to dual bound */, FALSE /* delayed */,
			         sepaFreeXxx, sepaInitXxx, sepaExitXxx, 
			         sepaInitsolXxx, sepaExitsolXxx,
			         sepaExeclpBCH, sepaExecsolXxx,
			         sepadata) );
	}

	SCIP_EVENTHDLRDATA* eventhdlrdata = new SCIP_EVENTHDLRDATA;
	eventhdlrdata->bch = bch;
	eventhdlrdata->smag = prob;
	
	SCIP_CALL( SCIPincludeEventhdlr(scip, "GamsBCH", "Gams BCH incumbent updater", NULL, eventInitNewInc, eventExitNewInc, NULL, NULL, NULL, eventExecNewInc, eventhdlrdata) );
/*
	if (opt.isDefined("usercutcall")) {
		GamsCutGenerator gamscutgen(*bch, preprocessedmodel);
		model.addCutGenerator(&gamscutgen, 1, "GamsBCH", true, opt.getBool("usercutnewint"));
	}
	if (opt.isDefined("userheurcall")) {
		GamsHeuristic gamsheu(*bch);
		model.addHeuristic(&gamsheu, "GamsBCH");
	}
*/
	return SCIP_OKAY;
}

SCIP_DECL_SEPAEXECLP(sepaExeclpBCH) {
  SCIP_SEPADATA* sepadata = SCIPsepaGetData(sepa);
  assert(sepadata != NULL);

  *result = SCIP_DIDNOTRUN;

  if (!sepadata->bch->doCuts())
		return SCIP_OKAY;
  
  int n = smagColCount(sepadata->smag);
  double* x  = new double[n];
  double* lb = new double[n];
  double* ub = new double[n];
	double objval; 
	
	SCIP_VAR** origvars = NULL;
	int norigvars;
	SCIP_CALL( SCIPgetOrigVarsData(scip, &origvars, &norigvars, NULL, NULL, NULL, NULL) );
	assert(norigvars == n);

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
	
//	SCIP_CALL( SCIPcreateLPSol(scip, &sol, NULL) );
	SCIP_CALL( SCIPsolRetransform (sol, scip->set, scip->stat, scip->origprob) );

	for (int i=0; i<norigvars; ++i) {
//		printf("value variable %d is %g\n", i, SCIPgetSolVal(scip, sol, origvars[i]));
		x[i] = SCIPgetSolVal(scip, sol, origvars[i]);
//		if (SCIPvarGetLPSol(transvars[i]))
//			printf("%d: %g -> %g\n", i, SCIPvarGetLPSol(transvars[i]), x[i]);
	}
//	objval = sol->obj;
//	double objoffset = sepadata->smag->gms.grhs[sepadata->smag->gms.slplro-1] * sepadata->smag->gObjFactor;
//	objval -= objoffset;
	
	smagEvalObjFunc(sepadata->smag, x, &objval);

	// transform local node bounds back into original space
	for (int i=0; i<norigvars; ++i) {
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
				double tub = SCIPvarGetLbLocal(tvar);
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
	}
	
//	for (int i=0; i<norigvars; ++i)
//		printf("node values: \tlb: %g \tval: %g \tub: %g\n", lb[i], x[i], ub[i]);
	
	sepadata->bch->setNodeSolution(x, objval, lb, ub);

	std::vector<GamsBCH::Cut> cuts;
	if (!sepadata->bch->generateCuts(cuts)) {
		smagStdOutputPrint(sepadata->smag, SMAG_ALLMASK, "GamsBCH: Generation of cuts failed!\n");
		*result = SCIP_DIDNOTFIND;
		// TODO: memory leak x, lb, ub
		return SCIP_OKAY;
	}
	
	for(std::vector<GamsBCH::Cut>::iterator cuts_it(cuts.begin()); cuts_it!=cuts.end(); ++cuts_it) {
		GamsBCH::Cut& cut(*cuts_it);
		SCIP_ROW* row;
		SCIP_CALL( SCIPcreateEmptyRow(scip, &row, "GamsBCH cut", cut.lb, cut.ub, TRUE /* locally */, TRUE /* modifiable */, TRUE /* removable */ ) );
		
		for (int i=0; i<cut.nnz; ++i) {
			SCIP_VAR* var = origvars[cut.indices[i]];
			switch (SCIPvarGetStatus(var)) {
				case SCIP_VARSTATUS_ORIGINAL: // variable belongs to original problem
					SCIP_CALL( SCIPaddVarToRow(scip, row, SCIPvarGetTransVar(var), cut.coeff[i]) );
					break;
				case SCIP_VARSTATUS_LOOSE: // variable is a loose variable of the transformed problem   (this cannot be the case for an original variable, or?)
				case SCIP_VARSTATUS_COLUMN: // variable is a column of the transformed problem          (this cannot be the case for an original variable, or?)
					SCIP_CALL( SCIPaddVarToRow(scip, row, var, cut.coeff[i]) );
					break;
				case SCIP_VARSTATUS_FIXED: // variable is fixed to specific value in the transformed problem -> modify lb and ub of row
					if (!SCIPisInfinity(scip, -cut.lb)) cut.lb -= cut.coeff[i]*lb[cut.indices[i]];
					if (!SCIPisInfinity(scip,  cut.ub)) cut.ub -= cut.coeff[i]*lb[cut.indices[i]];
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
	
	if (cuts.size())
		*result = SCIP_SEPARATED;
	
	delete[] x;
	delete[] lb;
	delete[] ub;
	
//	char buffer[128];
//	sprintf(buffer, "GamsBCH: Generated %d cuts!\n", (int)cuts.size());
//	smagStdOutputPrint(sepadata->smag, SMAG_LOGMASK, buffer);
	
   return SCIP_OKAY;
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
	SCIP_EVENTHDLRDATA* data = SCIPeventhdlrGetData(eventhdlr);
	assert(data);
	
	int n = smagColCount(data->smag);
	
	SCIP_VAR** origvars = NULL;
	int norigvars;
	SCIP_CALL( SCIPgetOrigVarsData(scip, &origvars, &norigvars, NULL, NULL, NULL, NULL) );
	assert(norigvars == n);

	SCIP_SOL* sol = SCIPeventGetSol(event);
	assert(sol);
	
	double* x = new double[n];
	for (int i=0; i<n; ++i)
		x[i] = SCIPsolGetVal(sol, scip->set, scip->stat, origvars[i]);
	
	data->bch->setIncumbentSolution(x, sol->obj);
//	printf("updated to sol with val %g\n", sol->obj);
	
	delete[] x;
	
	return SCIP_OKAY;
}
