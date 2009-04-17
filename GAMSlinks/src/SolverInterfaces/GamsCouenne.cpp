// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsCouenne.hpp"
#include "GamsCouenne.h"
#include "GamsMINLP.hpp"
#include "GamsCouenneSetup.hpp"
#include "GamsCbc.hpp"

#include "BonCbc.hpp"

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

// GAMS
#ifdef GAMS_BUILD
#include "gmomcc.h"
#else
#include "gmocc.h"
#endif

using namespace Bonmin;
using namespace Ipopt;
using namespace std;

GamsCouenne::GamsCouenne()
: gmo(NULL), couenne_setup(NULL), gamscbc(NULL)
{
#ifdef GAMS_BUILD
	strcpy(couenne_message, "GAMS/CoinCouenne (Couenne Library 0.1)\nwritten by P. Belotti\n");
#else
	strcpy(couenne_message, "GAMS/Couenne (Couenne Library 0.1)\nwritten by P. Belotti\n");
#endif
}

GamsCouenne::~GamsCouenne() {
	delete couenne_setup;
	delete gamscbc;
}

int GamsCouenne::readyAPI(struct gmoRec* gmo_, struct optRec* opt, struct dctRec* gcd) {
	gmo = gmo_;
	assert(gmo);
	assert(IsNull(minlp));
	assert(couenne_setup == NULL);

	char msg[256];
#ifndef GAMS_BUILD
  if (!gmoGetReadyD(GAMSIO_PATH, msg, sizeof(msg)))
#endif
  	if (!gmoGetReady(msg, sizeof(msg))) {
  		fprintf(stderr, "Error loading GMO library: %s\n",msg);
  		return 1;
  	}

	gmoObjStyleSet(gmo, ObjType_Fun);
	gmoObjReformSet(gmo, 1);
 	gmoIndexBaseSet(gmo, 0);

 	if (isMIP()) {
 		gmoLogStat(gmo, "Problem is linear. Passing over to Cbc.");
 		gamscbc = new GamsCbc();
 		return gamscbc->readyAPI(gmo, opt, gcd);
 	}
 	
 	if (!gmoN(gmo)) {
 		gmoLogStat(gmo, "Error: Bonmin requires variables.");
 		return 1;
 	}

 	for (int i = 0; i < gmoN(gmo); ++i)
		if (gmoGetVarTypeOne(gmo, i) == var_SC || gmoGetVarTypeOne(gmo, i) == var_SI) {
			gmoLogStat(gmo, "Error: Semicontinuous and semiinteger variables not supported by Bonmin.");
			return 1;
		}
 
  minlp = new GamsMINLP(gmo);
  minlp->in_couenne = true;

	couenne_setup = new GamsCouenneSetup(gmo);
	if (!couenne_setup->InitializeCouenne(minlp))
		return 1;
  
 	return 0;
}

int GamsCouenne::callSolver() {
	assert(gmo);
	
 	if (isMIP()) {
 		assert(gamscbc);
 		return gamscbc->callSolver();
 	}
	
	assert(couenne_setup);
	
	try {
		Bab bb;
		bb.setUsingCouenne(true);
		minlp->nlp->clockStart = gmoTimeDiffStart(gmo);
		bb(*couenne_setup); // do branch and bound
		
		double best_val = bb.model().getObjValue();
		double best_bound = bb.model().getBestPossibleObjValue();
		if (gmoSense(gmo) == Obj_Max) {
			best_val   *= -1;
			best_bound *= -1;
		}
//		double best_bound = (gmoSense(gmo) == Obj_Min) ? bb.bestBound() : -bb.bestBound();

//		OsiTMINLPInterface& osi_tminlp(*couenne_setup->nonlinearSolver());
		if (bb.bestSolution()) {
			char buf[100];
			snprintf(buf, 100, "\nCouenne finished. Found feasible point. Objective function value = %g.", best_val);
			gmoLogStat(gmo, buf);

			double* negLambda = new double[gmoM(gmo)];
			memset(negLambda, 0, gmoM(gmo)*sizeof(double));
			
			gmoSetSolution2(gmo, bb.bestSolution(), negLambda);
			gmoSetHeadnTail(gmo, HobjVal,   best_val);
			
			delete[] negLambda;
		} else {
			gmoLogStat(gmo, "\nCouenne finished. No feasible point found.");
		}

		if (best_bound > -1e200 && best_bound < 1e200)
			gmoSetHeadnTail(gmo, Tmipbest, best_bound);
		gmoSetHeadnTail(gmo, Tmipnod, bb.numNodes());
		gmoSetHeadnTail(gmo, Hiterused, bb.iterationCount());
		gmoSetHeadnTail(gmo, HresUsed,  gmoTimeDiffStart(gmo) - minlp->nlp->clockStart);
		gmoSetHeadnTail(gmo, HdomUsed,  minlp->nlp->domviolations);
		gmoSolveStatSet(gmo, minlp->solver_status);
		gmoModelStatSet(gmo, minlp->model_status);

		gmoLogStat(gmo, "");
		char buf[1024];
//		double best_val = gmoSense(gmo) == Obj_Min ? osi_tminlp.getObjValue() : -osi_tminlp.getObjValue();
		if (bb.bestSolution()) {
			snprintf(buf, 1024, "MINLP solution: %20.10g   (%d nodes, %g seconds)", best_val, bb.numNodes(), gmoTimeDiffStart(gmo) - minlp->nlp->clockStart);
			gmoLogStat(gmo, buf);
		}
		if (best_bound > -1e200 && best_bound < 1e200) {
			snprintf(buf, 1024, "Best possible: %21.10g\n", best_bound);
			gmoLogStat(gmo, buf);

			if (bb.bestSolution()) {
				snprintf(buf, 1024, "Absolute gap: %22.5g\nRelative gap: %22.5g\n", CoinAbs(best_val-best_bound), CoinAbs(best_val-best_bound)/CoinMax(CoinAbs(best_bound), 1.));
				gmoLogStat(gmo, buf);
			}
		}
		
  } catch(CoinError &error) {
  	char buf[1024];
  	snprintf(buf, 1024, "%s::%s\n%s", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
		gmoLogStat(gmo, buf);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
	  return 1;
  } catch (TNLPSolver::UnsolvedError *E) { 	// there has been a failure to solve a problem with Ipopt.
		char buf[1024];
		snprintf(buf, 1024, "Error: %s exited with error %s", E->solverName().c_str(), E->errorName().c_str());
		gmoLogStat(gmo, buf);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return 1;
	} catch (std::bad_alloc) {
		gmoLogStat(gmo, "Error: Not enough memory!");
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return -1;
	} catch (...) {
		gmoLogStat(gmo, "Error: Unknown exception thrown.\n");
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return -1;
	}
	
	return 0;
}

bool GamsCouenne::isMIP() {
	return !gmoNLNZ(gmo) && !gmoObjNLNZ(gmo);
}

DllExport GamsCouenne* STDCALL createNewGamsCouenne() {
	return new GamsCouenne();
}

DllExport int STDCALL couCallSolver(couRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsCouenne*)Cptr)->callSolver();
}

DllExport int STDCALL couModifyProblem(couRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsCouenne*)Cptr)->modifyProblem();
}

DllExport int STDCALL couHaveModifyProblem(couRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsCouenne*)Cptr)->haveModifyProblem();
}

DllExport int STDCALL couReadyAPI(couRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr, dctHandle_t Dptr) {
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	gmoLogStatPChar(Gptr, ((GamsCouenne*)Cptr)->getWelcomeMessage());
	return ((GamsCouenne*)Cptr)->readyAPI(Gptr, Optr, Dptr);
}

DllExport void STDCALL couFree(couRec_t **Cptr) {
	assert(Cptr != NULL);
	delete (GamsCouenne*)*Cptr;
	*Cptr = NULL;
}

DllExport void STDCALL couCreate(couRec_t **Cptr, char *msgBuf, int msgBufLen) {
	assert(Cptr != NULL);
	*Cptr = (couRec_t*) new GamsCouenne();
	if (msgBufLen && msgBuf)
		msgBuf[0] = 0;
}
