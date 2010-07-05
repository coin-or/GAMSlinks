// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsBonmin.hpp"
#include "GamsBonmin.h"
#include "GamsMINLP.hpp"
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsIpopt.hpp" // in case we need to solve an NLP
#include "GamsCbc.hpp"   // in case we need to solve an LP or MIP

// to be sure to get (or not get) HAVE_M??? and HAVE_PARDISO defined
#include "IpoptConfig.h"

#include "BonBonminSetup.hpp"
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

extern "C" {
#ifndef HAVE_MA27
#define HAVE_HSL_LOADER
#else
# ifndef HAVE_MA28
# define HAVE_HSL_LOADER
# else
#  ifndef HAVE_MA57
#  define HAVE_HSL_LOADER
#  else
#   ifndef HAVE_MC19
#   define HAVE_HSL_LOADER
#   endif
#  endif
# endif
#endif
#ifdef HAVE_HSL_LOADER
#include "HSLLoader.h"
#endif
#ifndef HAVE_PARDISO
#include "PardisoLoader.h"
#endif
}

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gmspal.h"  /* for audit line */
#endif
/* value for not available/applicable */
#if GMOAPIVERSION >= 7
#define GMS_SV_NA     gmoValNA(gmo)
#else
#define GMS_SV_NA     2.0E300
#endif

#if GMOAPIVERSION < 8
#define Hresused     HresUsed
#define Hdomused     HdomUsed
#define Hobjval      HobjVal
#endif

#define ITERLIM_INFINITY 2000000000

using namespace Bonmin;
using namespace Ipopt;

GamsBonmin::GamsBonmin()
: gmo(NULL), gev(NULL), msghandler(NULL), bonmin_setup(NULL), gamsipopt(NULL), gamscbc(NULL)
{
	strcpy(bonmin_message, "COIN-OR Bonmin (Bonmin Library 1.4)\nwritten by P. Bonami\n");
}

GamsBonmin::~GamsBonmin() {
	delete bonmin_setup;
	delete msghandler;
	delete gamsipopt;
	delete gamscbc;
}

int GamsBonmin::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	char buffer[512];

	gmo = gmo_;
	assert(gmo);
	assert(IsNull(minlp));
	assert(bonmin_setup == NULL);

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1;

	gev = (gevRec*)gmoEnvironment(gmo);
	
#ifdef GAMS_BUILD
#include "coinlibdCLsvn.h" 
	auditGetLine(buffer, sizeof(buffer));
	gevLogStat(gev, "");
	gevLogStat(gev, buffer);
	gevStatAudit(gev, buffer);
#endif
	
	gevLogStat(gev, "");
	gevLogStatPChar(gev, getWelcomeMessage());

	gmoObjStyleSet(gmo, ObjType_Fun);
	gmoObjReformSet(gmo, 1);
 	gmoIndexBaseSet(gmo, 0);

 	if (isMIP()) {
 		gevLogStat(gev, "Problem is linear. Passing over to Cbc.");
 		gamscbc = new GamsCbc();
 		return gamscbc->readyAPI(gmo, opt);
 	}

 	if (isNLP()) {
 		gevLogStat(gev, "Problem is continuous. Passing over to Ipopt.");
 		gamsipopt = new GamsIpopt();
 		return gamsipopt->readyAPI(gmo, opt);
 	}

 	if (!gmoN(gmo)) {
 		gevLogStat(gev, "Error: Bonmin requires variables.");
 		gmoSolveStatSet(gmo, SolveStat_Capability);
 		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
 		return 1;
 	}

  if (gmoGetVarTypeCnt(gmo, var_SC) || gmoGetVarTypeCnt(gmo, var_SI)) {
    gevLogStat(gev, "ERROR: Semicontinuous and semiinteger variables not supported by Bonmin.\n");
    gmoSolveStatSet(gmo, SolveStat_Capability);
    gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    return 1;
  }

	bonmin_setup = new BonminSetup();

	// instead of initializeOptionsAndJournalist we do it our own way, so we can use the SmagJournal
  SmartPtr<OptionsList> options = new OptionsList();
	SmartPtr<Journalist> journalist= new Journalist();
  SmartPtr<Bonmin::RegisteredOptions> roptions = new Bonmin::RegisteredOptions();
 	SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY, J_STRONGWARNING);
	jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!journalist->AddJournal(jrnl))
		gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");
	options->SetJournalist(journalist);
	options->SetRegisteredOptions(GetRawPtr(roptions));

	bonmin_setup->setOptionsAndJournalist(roptions, options, journalist);
  bonmin_setup->registerOptions();

	roptions->SetRegisteringCategory("Linear Solver", Bonmin::RegisteredOptions::IpoptCategory);
#ifdef HAVE_HSL_LOADER
	// add option to specify path to hsl library
  bonmin_setup->roptions()->AddStringOption1("hsl_library", // name
			"path and filename of HSL library for dynamic load",  // short description
			"", // default value
			"*", // setting1
			"path (incl. filename) of HSL library", // description1
			"Specify the path to a library that contains HSL routines and can be load via dynamic linking. "
			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options, e.g., ``linear_solver''."
	);
#endif
#ifndef HAVE_PARDISO
	// add option to specify path to pardiso library
  bonmin_setup->roptions()->AddStringOption1("pardiso_library", // name
			"path and filename of Pardiso library for dynamic load",  // short description
			"", // default value
			"*", // setting1
			"path (incl. filename) of Pardiso library", // description1
			"Specify the path to a Pardiso library that and can be load via dynamic linking. "
			"Note, that you still need to specify to use pardiso as linear_solver."
	);
#endif

  // add options specific to BCH heuristic callback
//TODO  BCHsetupOptions(*bonmin_setup.roptions());
//  printOptions(journalist, bonmin_setup.roptions());

	// Change some options
	bonmin_setup->options()->SetNumericValue("bound_relax_factor", 1e-10, true, true);
//	bonmin_setup->options()->SetNumericValue("nlp_lower_bound_inf", gmoMinf(gmo), false, true);
//	bonmin_setup->options()->SetNumericValue("nlp_upper_bound_inf", gmoPinf(gmo), false, true);
	bonmin_setup->options()->SetIntegerValue("bonmin.nlp_log_at_root", Ipopt::J_ITERSUMMARY, true, true);
#if GMOAPIVERSION >= 7
	if (gevGetIntOpt(gev, gevUseCutOff))
		bonmin_setup->options()->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == Obj_Min ? gevGetDblOpt(gev, gevCutOff) : -gevGetDblOpt(gev, gevCutOff), true, true);
#endif
	bonmin_setup->options()->SetNumericValue("bonmin.allowable_gap", gevGetDblOpt(gev, gevOptCA), true, true);
	bonmin_setup->options()->SetNumericValue("bonmin.allowable_fraction_gap", gevGetDblOpt(gev, gevOptCR), true, true);
	if (gevGetIntOpt(gev, gevNodeLim))
		bonmin_setup->options()->SetIntegerValue("bonmin.node_limit", gevGetIntOpt(gev, gevNodeLim), true, true);
	bonmin_setup->options()->SetNumericValue("bonmin.time_limit", gevGetDblOpt(gev, gevResLim), true, true);
	if (gevGetIntOpt(gev, gevIterLim) < ITERLIM_INFINITY)
      bonmin_setup->options()->SetIntegerValue("bonmin.iteration_limit", gevGetIntOpt(gev, gevIterLim), true, true);

	if (gmoNLM(gmo) == 0  && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp))
		bonmin_setup->options()->SetStringValue("hessian_constant", "yes", true, true);

	try {
		if (gmoOptFile(gmo)) {
		 	bonmin_setup->options()->SetStringValue("print_user_options", "yes", true, true);
			gmoNameOptFile(gmo, buffer);
			bonmin_setup->readOptionsFile(buffer);
		}	else { // need to let Bonmin read something, otherwise it will try to read its default option file bonmin.opt
			bonmin_setup->readOptionsString(std::string());
		}
	} catch (IpoptException error) {
		gevLogStat(gev, error.Message().c_str());
	  return 1;
	} catch (std::bad_alloc) {
		gevLogStat(gev, "Error: Not enough memory\n");
		return -1;
	} catch (...) {
		gevLogStat(gev, "Error: Unknown exception thrown.\n");
		return -1;
	}

  std::string parvalue;
#ifdef COIN_HAS_CPX
  std::string prefixes[7] = { "", "bonmin", "oa_decomposition.", "pump_for_minlp.", "rins.", "rens.", "local_branch." };
  for( int i = 0; i < 7; ++i ) {
    bonmin_setup->options()->GetStringValue("milp_solver", parvalue, prefixes[i]);
    if (parvalue == "Cplex" && (!checkLicense(gmo) || !registerGamsCplexLicense(gmo))) {
      gevLogStat(gev, "No valid CPLEX license found.");
      return 1;
    }
  }
#endif

	std::string libpath;
#ifdef HAVE_HSL_LOADER
	if (bonmin_setup->options()->GetStringValue("hsl_library", libpath, "")) {
		if (LSL_loadHSL(libpath.c_str(), buffer, 512) != 0) {
			gevLogStatPChar(gev, "Failed to load HSL library at user specified path: ");
			gevLogStat(gev, buffer);
		  return 1;
		}
	}
#endif
#ifndef HAVE_PARDISO
	if (bonmin_setup->options()->GetStringValue("pardiso_library", libpath, "")) {
		if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512) != 0) {
			gevLogStatPChar(gev, "Failed to load Pardiso library at user specified path: ");
			gevLogStat(gev, buffer);
		  return 1;
		}
	}
#endif

	double ipoptinf;
	bonmin_setup->options()->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
	gmoMinfSet(gmo, ipoptinf);
	bonmin_setup->options()->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
	gmoPinfSet(gmo, ipoptinf);

  minlp = new GamsMINLP(gmo);

	bonmin_setup->options()->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");
//	// or should we also check the tolerance for acceptable points?
//	bonmin_setup.options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, "");
//	bonmin_setup.options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, "");

	bool hessian_is_approx = false;
	bonmin_setup->options()->GetStringValue("hessian_approximation", parvalue, "");
	if (parvalue == "exact") {
		int do2dir = 1;
		int dohess = 1;
		gmoHessLoad(gmo, 0, -1, &do2dir, &dohess); // TODO make "-1" a parameter (like rvhess in CONOPT)
		if (!dohess) { // TODO make "-1" a parameter (like rvhess in CONOPT)
			gevLogStat(gev, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
			bonmin_setup->options()->SetStringValue("hessian_approximation", "limited-memory");
			hessian_is_approx = true;
	  }
	} else
		hessian_is_approx = true;

	if (hessian_is_approx) { // check whether QP strong branching is enabled
		bonmin_setup->options()->GetStringValue("variable_selection", parvalue, "bonmin.");
		if (parvalue == "qp-strong-branching") {
			gevLogStat(gev, "Error: QP strong branching does not work when the Hessian is approximated. Aborting...");
			return 1;
		}
	}

	msghandler = new GamsMessageHandler(gev);
//	GamsBCH* bch = NULL;
//	CbcModel* modelptr = NULL;

//	BCHinit(bonmin_setup, gamshandler, dict, bch, modelptr);
//	if (bch)
//		bch->setGlobalBounds(prob->colLB, prob->colUB);

	// the easiest would be to call bonmin_setup.initializeBonmin(smagminlp), but then we cannot set the message handler
	// so we do the following
	try {
		OsiTMINLPInterface first_osi_tminlp;
		//first_osi_tminlp.initialize(roptions, options, journalist, "bonmin.", smagminlp);
		first_osi_tminlp.initialize(roptions, options, journalist, GetRawPtr(minlp));
		first_osi_tminlp.passInMessageHandler(msghandler);
		bonmin_setup->initialize(first_osi_tminlp); // this will clone first_osi_tminlp
  } catch(CoinError &error) {
  	char buf[1024];
  	snprintf(buf, 1024, "%s::%s\n%s\n", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
		gevLogStatPChar(gev, buf);
		return 1;
	} catch (std::bad_alloc) {
		gevLogStat(gev, "Error: Not enough memory!");
		return -1;
	} catch (...) {
		gevLogStat(gev, "Error: Unknown exception thrown!");
		return -1;
	}

 	return 0;
}

int GamsBonmin::callSolver() {
	assert(gmo);

	if (isMIP()) {
		assert(gamscbc);
		return gamscbc->callSolver();
	}

 	if (isNLP()) {
 		assert(gamsipopt);
 		return gamsipopt->callSolver();
 	}

	assert(bonmin_setup);
	assert(IsValid(minlp));

	try {
		Bab bb;

		minlp->nlp->clockStart = gevTimeDiffStart(gev);
		bb(*bonmin_setup); //process parameters and do branch and bound

		double best_bound = (gmoSense(gmo) == Obj_Min) ? bb.bestBound() : -bb.bestBound();
		if (best_bound > -1e200 && best_bound < 1e200)
			gmoSetHeadnTail(gmo, Tmipbest, best_bound);
		gmoSetHeadnTail(gmo, Tmipnod, bb.numNodes());

		OsiTMINLPInterface& osi_tminlp(*bonmin_setup->nonlinearSolver());
		if (bb.bestSolution()) {
			char buf[100];
			snprintf(buf, 100, "\nBonmin finished. Found feasible point. Objective function value = %g.", bb.bestObj());
			gevLogStat(gev, buf);

			bool has_free_var = false;
			for (Index i = 0; i < gmoN(gmo); ++i)
				if (gmoGetVarTypeOne(gmo, i) != var_X)
					osi_tminlp.setColBounds(i, bb.bestSolution()[i], bb.bestSolution()[i]);
				else if ((!has_free_var) && osi_tminlp.getColUpper()[i]-osi_tminlp.getColLower()[i] > 1e-5)
					has_free_var = true;
			osi_tminlp.setColSolution(bb.bestSolution());

			if (!has_free_var) {
				gevLog(gev, "All variables are discrete. Dual variables for fixed problem will be not available.");
				osi_tminlp.initialSolve(); // this will only evaluate the constraints, so we get correct row levels
				writeSolutionNoDual(osi_tminlp, bb.iterationCount());
			} else {
				gevLog(gev, "Resolve with fixed discrete variables to get dual values.");
				bool error_in_fixedsolve = false;
				try {
					osi_tminlp.initialSolve();
					error_in_fixedsolve = !osi_tminlp.isProvenOptimal();
				} catch (TNLPSolver::UnsolvedError *E) { // there has been a failure to solve a problem with Ipopt
					char buf[1024];
					snprintf(buf, 1024, "Error: %s exited with error %s", E->solverName().c_str(), E->errorName().c_str());
					gevLogStat(gev, buf);
					error_in_fixedsolve = true;
				}
				if (!error_in_fixedsolve) {
					writeSolution(osi_tminlp, bb.iterationCount());
				} else {
					gevLogStat(gev, "Problems solving fixed problem. Dual variables for NLP subproblem not available.");
					writeSolutionNoDual(osi_tminlp, bb.iterationCount());
				}
			}
		} else {
			gevLogStat(gev, "\nBonmin finished. No feasible point found.");
			gmoSolveStatSet(gmo, minlp->solver_status);
			gmoModelStatSet(gmo, minlp->model_status);
			gmoSetHeadnTail(gmo, Hiterused, bb.iterationCount());
			gmoSetHeadnTail(gmo, Hdomused,  minlp->nlp->domviolations);
			gmoSetHeadnTail(gmo, Hresused,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
		}

		gevLogStat(gev, "");
		char buf[1024];
		double best_val = gmoSense(gmo) == Obj_Min ? osi_tminlp.getObjValue() : -osi_tminlp.getObjValue();
		if (bb.bestSolution()) {
			snprintf(buf, 1024, "MINLP solution: %20.10g   (%d nodes, %g seconds)", best_val, bb.numNodes(), gevTimeDiffStart(gev) - minlp->nlp->clockStart);
			gevLogStat(gev, buf);
		}
		if (best_bound > -1e200 && best_bound < 1e200) {
			snprintf(buf, 1024, "Best possible: %21.10g\n", best_bound);
			gevLogStat(gev, buf);

			if (bb.bestSolution()) {
				snprintf(buf, 1024, "Absolute gap: %22.5g\nRelative gap: %22.5g\n", CoinAbs(best_val-best_bound), CoinAbs(best_val-best_bound)/CoinMax(CoinAbs(best_bound), 1.));
				gevLogStat(gev, buf);
			}
		}
  } catch(CoinError &error) {
  	char buf[1024];
  	snprintf(buf, 1024, "%s::%s\n%s", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
		gevLogStat(gev, buf);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
	  return 1;
  } catch (TNLPSolver::UnsolvedError *E) { 	// there has been a failure to solve a problem with Ipopt.
		char buf[1024];
		snprintf(buf, 1024, "Error: %s exited with error %s", E->solverName().c_str(), E->errorName().c_str());
		gevLogStat(gev, buf);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return 1;
	} catch (std::bad_alloc) {
		gevLogStat(gev, "Error: Not enough memory!");
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return -1;
	} catch (...) {
		gevLogStat(gev, "Error: Unknown exception thrown.\n");
		gmoSolveStatSet(gmo, SolveStat_SystemErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		return -1;
	}

	return 0;
}

void GamsBonmin::writeSolution(OsiTMINLPInterface& osi_tminlp, int itercount) {
	int n = gmoN(gmo);
	int m = gmoM(gmo);
	int isMin = gmoSense(gmo) == Obj_Min ? 1 : -1;

	const double* lambda = osi_tminlp.getRowPrice();
	const double* z_L    = lambda+m;
	const double* z_U    = z_L+n;

	double* colMarg    = new double[n];
	for (Index i = 0; i < n; ++i) {
		// if, e.g., x_i has no lower bound, then the dual z_L[i] is -infinity
		colMarg[i]    = 0;
		if (z_L[i] > gmoMinf(gmo)) colMarg[i] += isMin*z_L[i];
		if (z_U[i] < gmoPinf(gmo)) colMarg[i] -= isMin*z_U[i];
	}

	double* negLambda  = new double[m];
  for (Index i = 0;  i < m;  i++) {
  	negLambda[i] = -isMin*lambda[i];
  }

	gmoSetSolution(gmo, osi_tminlp.getColSolution(), colMarg, negLambda, osi_tminlp.getRowActivity());
	gmoSetHeadnTail(gmo, Hobjval,   isMin*osi_tminlp.getObjValue());
	gmoSetHeadnTail(gmo, Hiterused, itercount);
	gmoSetHeadnTail(gmo, Hresused,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
	gmoSetHeadnTail(gmo, Hdomused,  minlp->nlp->domviolations);

	gmoModelStatSet(gmo, minlp->model_status);
	gmoSolveStatSet(gmo, minlp->solver_status);

	delete[] colMarg;
	delete[] negLambda;
}

void GamsBonmin::writeSolutionNoDual(OsiTMINLPInterface& osi_tminlp, int itercount) {
	int n = gmoN(gmo);
	int m = gmoM(gmo);
	int isMin = gmoSense(gmo) == Obj_Min ? 1 : -1;

	double* colMarg   = new double[n];
	double* negLambda = new double[m];
	memset(colMarg,   0, n*sizeof(double));
	memset(negLambda, 0, m*sizeof(double));

	gmoSetSolution(gmo, osi_tminlp.getColSolution(), colMarg, negLambda, osi_tminlp.getRowActivity());
	gmoSetHeadnTail(gmo, Hobjval,   isMin*osi_tminlp.getObjValue());
	gmoSetHeadnTail(gmo, Hiterused, itercount);
	gmoSetHeadnTail(gmo, Hresused,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
	gmoSetHeadnTail(gmo, Hdomused,  minlp->nlp->domviolations);

	gmoModelStatSet(gmo, minlp->model_status);
	gmoSolveStatSet(gmo, minlp->solver_status);

	delete[] colMarg;
	delete[] negLambda;
}

bool GamsBonmin::isNLP() {
	switch(gmoModelType(gmo)) {
		case Proc_lp:
		case Proc_rmip:
		case Proc_qcp:
		case Proc_rmiqcp:
		case Proc_nlp:
		case Proc_rminlp:
			return true;
	}
	if (gmoNDisc(gmo))
		return false;
	int numSos1, numSos2, nzSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	if (nzSos)
		return false;
	return true;
}

bool GamsBonmin::isMIP() {
	return !gmoNLNZ(gmo) && !gmoObjNLNZ(gmo);
}

DllExport GamsBonmin* STDCALL createNewGamsBonmin() {
	return new GamsBonmin();
}

DllExport int STDCALL bonCallSolver(bonRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsBonmin*)Cptr)->callSolver();
}

DllExport int STDCALL bonModifyProblem(bonRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsBonmin*)Cptr)->modifyProblem();
}

DllExport int STDCALL bonHaveModifyProblem(bonRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsBonmin*)Cptr)->haveModifyProblem();
}

DllExport int STDCALL bonReadyAPI(bonRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr) {
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	if (!gevGetReady(msg, sizeof(msg)))
		return 1;
	return ((GamsBonmin*)Cptr)->readyAPI(Gptr, Optr);
}

DllExport void STDCALL bonFree(bonRec_t **Cptr) {
	assert(Cptr != NULL);
	delete (GamsBonmin*)*Cptr;
	*Cptr = NULL;
}

DllExport void STDCALL bonCreate(bonRec_t **Cptr, char *msgBuf, int msgBufLen) {
	assert(Cptr != NULL);
	*Cptr = (bonRec_t*) new GamsBonmin();
	if (msgBufLen && msgBuf)
		msgBuf[0] = 0;
}
