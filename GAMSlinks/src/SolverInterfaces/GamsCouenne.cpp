// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsCouenne.hpp"
#include "GamsCouenne.h"

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

#include "GamsMINLP.hpp"
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsNLinstr.h"
#include "GamsCbc.hpp"

#include "BonCbc.hpp"
#include "BonCouenneSetup.hpp"
#include "BonCouenneInterface.hpp"
#include "CouenneProblem.hpp"
#include "CouenneTypes.hpp"
#include "exprClone.hpp"
#include "exprGroup.hpp"
#include "exprAbs.hpp"
#include "exprConst.hpp"
#include "exprCos.hpp"
#include "exprDiv.hpp"
#include "exprExp.hpp"
#include "exprInv.hpp"
#include "exprLog.hpp"
#include "exprMax.hpp"
#include "exprMin.hpp"
#include "exprMul.hpp"
#include "exprOpp.hpp"
#include "exprPow.hpp"
#include "exprSin.hpp"
#include "exprSub.hpp"
#include "exprSum.hpp"
#include "exprVar.hpp"
//#include "exprQuad.hpp"
//#include "lqelems.hpp"

// to be sure to get (or not get) HAVE_M??? and HAVE_PARDISO defined
#include "IpoptConfig.h"

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

using namespace Bonmin;
using namespace Ipopt;
using namespace std;

GamsCouenne::GamsCouenne()
: gmo(NULL), gev(NULL), gamscbc(NULL)
{
	strcpy(couenne_message, "COIN-OR Couenne (Couenne Library 0.3)\nwritten by P. Belotti\n");
}

GamsCouenne::~GamsCouenne() {
	delete gamscbc;
}

int GamsCouenne::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	gmo = gmo_;
	assert(gmo);
	assert(IsNull(minlp));

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1; 
	gev = (gevRec*)gmoEnvironment(gmo);
	
#ifdef GAMS_BUILD
	char buffer[256];
#include "coinlibdCL2svn.h" 
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
 	
 	if (!gmoN(gmo)) {
 		gevLogStat(gev, "Error: Bonmin requires variables.");
 		return 1;
 	}

  if (gmoGetVarTypeCnt(gmo, var_SC) || gmoGetVarTypeCnt(gmo, var_SI)) {
    gevLogStat(gev, "ERROR: Semicontinuous and semiinteger variables not supported by Couenne.\n");
    gmoSolveStatSet(gmo, SolveStat_Capability);
    gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    return 1;
  }

 	for (int i = 0; i < gmoN(gmo); ++i)
		if (gmoGetVarTypeOne(gmo, i) == var_SC || gmoGetVarTypeOne(gmo, i) == var_SI) {
			gevLogStat(gev, "Error: Semicontinuous and semiinteger variables not supported by Couenne.");
			return 1;
		}
 
#ifdef COIN_HAS_CPX
  if (checkLicense(gmo) && registerGamsCplexLicense(gmo))
    gevLog(gev, "Registered GAMS/CPLEX license.");
#endif

  minlp = new GamsMINLP(gmo);
  minlp->in_couenne = true;

	jnlst = new Ipopt::Journalist();
 	SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY, J_STRONGWARNING);
	jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!jnlst->AddJournal(jrnl))
		gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");
	
	roptions = new Bonmin::RegisteredOptions();
	options = new Ipopt::OptionsList(GetRawPtr(roptions), jnlst);
	
	CouenneSetup::registerAllOptions(roptions);
	roptions->SetRegisteringCategory("Linear Solver", Bonmin::RegisteredOptions::IpoptCategory);
#ifdef HAVE_HSL_LOADER
	// add option to specify path to hsl library
  roptions->AddStringOption1("hsl_library", // name
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
  roptions->AddStringOption1("pardiso_library", // name
			"path and filename of Pardiso library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"path (incl. filename) of Pardiso library", // description1
			"Specify the path to a Pardiso library that and can be load via dynamic linking. "
			"Note, that you still need to specify to pardiso as linear_solver."
	);
#endif

	// Change some options
	options->SetNumericValue("bound_relax_factor", 1e-10, true, true);
  if (GMS_SV_NA != gevGetDblOpt(gev, gevCutOff)) 
  	options->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == Obj_Min ? gevGetDblOpt(gev, gevCutOff) : -gevGetDblOpt(gev, gevCutOff), true, true); 
  options->SetNumericValue("bonmin.allowable_gap", gevGetDblOpt(gev, gevOptCA), true, true); 
 	options->SetNumericValue("bonmin.allowable_fraction_gap", gevGetDblOpt(gev, gevOptCR), true, true); 
 	if (gevGetIntOpt(gev, gevNodeLim))
 		options->SetIntegerValue("bonmin.node_limit", gevGetIntOpt(gev, gevNodeLim), true, true); 
 	options->SetNumericValue("bonmin.time_limit", gevGetDblOpt(gev, gevResLim), true, true); 
 	options->SetIntegerValue("bonmin.problem_print_level", J_STRONGWARNING, true, true); /* otherwise Couenne prints the problem to stdout */ 

  // workaround for bug in couenne reformulation: if there are tiny constants, delete_redundant might setup a nonstandard reformulation (e.g., using x*x instead of x^2) 
  // thus, we change the default of delete_redundant to off in this case
  bool havetinyconst = false;
	int constantlen = gmoNLConst(gmo);
	double* constants = (double*)gmoPPool(gmo);
	for (int i = 0; i < constantlen; ++i)
		if (CoinAbs(constants[i]) < COUENNE_EPS) {
			havetinyconst = true;
			break;
		}
	if (havetinyconst)
    options->SetStringValue("delete_redundant", "no", "couenne.");

	if (gmoNLM(gmo) == 0  && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp))
		options->SetStringValue("hessian_constant", "yes", true, true);
	
  double ipoptinf;
 	options->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
 	options->SetNumericValue("nlp_lower_bound_inf", ipoptinf, false, true); /* to disallow clobber */
 	gmoMinfSet(gmo, ipoptinf);
 	options->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
 	options->SetNumericValue("nlp_upper_bound_inf", ipoptinf, false, true); /* to disallow clobber */
 	gmoPinfSet(gmo, ipoptinf);

// 	printOptions();

 	return 0;
}

int GamsCouenne::callSolver() {
	assert(gmo);
	
 	if (isMIP()) {
 		assert(gamscbc);
 		return gamscbc->callSolver();
 	}
 	
	char buffer[1024];
	
 	CouenneProblem* problem;
#if GMOAPIVERSION >= 7
		problem = setupProblemNew();
#else
 	if (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp)
 		problem = setupProblemMIQQP();
 	else
 		problem = setupProblem();
#endif
 	if (!problem) {
 		gevLogStat(gev, "Error in setting up problem for Couenne.\n");
 		return -1;
 	}
	
	try {
		Bab bb;
		bb.setUsingCouenne(true);
		
		CouenneSetup couenne;
		couenne.setOptionsAndJournalist(roptions, options, jnlst);
		
		if (gmoOptFile(gmo)) {
			options->SetStringValue("print_user_options", "yes", true, true);
			gmoNameOptFile(gmo, buffer);
			couenne.BabSetupBase::readOptionsFile(buffer);
		} else // need to call readOptionsFile so that Couenne does not try reading couenne.opt later
			couenne.BabSetupBase::readOptionsFile("");
	   problem->initOptions(options);
		
		std::string libpath;
#ifdef HAVE_HSL_LOADER
		if (options->GetStringValue("hsl_library", libpath, "")) {
			if (LSL_loadHSL(libpath.c_str(), buffer, 1024) != 0) {
				gevLogStatPChar(gev, "Failed to load HSL library at user specified path: ");
				gevLogStat(gev, buffer);
			  return -1;
			}
		}
#endif
#ifndef HAVE_PARDISO
		if (options->GetStringValue("pardiso_library", libpath, "")) {
			if (LSL_loadPardisoLib(libpath.c_str(), buffer, 1024) != 0) {
				gevLogStatPChar(gev, "Failed to load Pardiso library at user specified path: ");
				gevLogStat(gev, buffer);
			  return -1;
			}
		}
#endif

		options->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");
	//	// or should we also check the tolerance for acceptable points?
	//	bonmin_setup.options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, "");
	//	bonmin_setup.options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, "");

		std::string s;
		options->GetStringValue("hessian_approximation", s, "");
		if (s == "exact") {
			int do2dir = 1;
			int dohess = 1;
			gmoHessLoad(gmo, 0, -1, &do2dir, &dohess); // TODO make "-1" a parameter (like rvhess in CONOPT)
			if (!dohess) { // TODO make "-1" a parameter (like rvhess in CONOPT)
				gevLogStat(gev, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
				options->SetStringValue("hessian_approximation", "limited-memory");
		  }
		}
		
		CouenneInterface* ci = new CouenneInterface;
		ci->initialize(roptions, options, jnlst, GetRawPtr(minlp));
		GamsMessageHandler* msghandler = new GamsMessageHandler(gev);
		ci->passInMessageHandler(msghandler);

		minlp->nlp->clockStart = gevTimeDiffStart(gev);

		char** argv = NULL;
		if (!couenne.InitializeCouenne(argv, problem, ci)) {
			gevLogStat(gev, "Reformulation finds model infeasible.\n");
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
			return 0;
		}
		
		double preprocessTime = gevTimeDiffStart(gev) - minlp->nlp->clockStart;
		
		snprintf(buffer, 1024, "Couenne initialized (%g seconds).", preprocessTime);
		gevLogStat(gev, buffer);
		
		double reslim = gevGetDblOpt(gev, gevResLim);
		if (preprocessTime >= reslim) {
			gevLogStat(gev, "Time is up.\n");
			gmoSolveStatSet(gmo, SolveStat_Resource);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			return 0;
		}
		
		options->SetNumericValue("bonmin.time_limit", reslim - preprocessTime, true, true);
    couenne.setDoubleParameter(BabSetupBase::MaxTime, reslim - preprocessTime);
				
		bb(couenne); // do branch and bound
		
		double best_val = bb.model().getObjValue();
		double best_bound = bb.model().getBestPossibleObjValue();
		if (gmoSense(gmo) == Obj_Max) {
			best_val   *= -1;
			best_bound *= -1;
		}

		if (bb.bestSolution()) {
			snprintf(buffer, 100, "\nCouenne finished. Found feasible point. Objective function value = %g.", best_val);
			gevLogStat(gev, buffer);

			double* negLambda = new double[gmoM(gmo)];
			memset(negLambda, 0, gmoM(gmo)*sizeof(double));
			
			gmoSetSolution2(gmo, bb.bestSolution(), negLambda);
			gmoSetHeadnTail(gmo, HobjVal,   best_val);
			
			delete[] negLambda;
		} else {
         gmoSetHeadnTail(gmo, HobjVal,   gmoPinf(gmo));
			gevLogStat(gev, "\nCouenne finished. No feasible point found.");
		}

		if (best_bound > -1e200 && best_bound < 1e200)
			gmoSetHeadnTail(gmo, Tmipbest, best_bound);
		gmoSetHeadnTail(gmo, Tmipnod, bb.numNodes());
		gmoSetHeadnTail(gmo, Hiterused, bb.iterationCount());
		gmoSetHeadnTail(gmo, HresUsed,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
		gmoSetHeadnTail(gmo, HdomUsed,  minlp->nlp->domviolations);
		gmoSolveStatSet(gmo, minlp->solver_status);
		gmoModelStatSet(gmo, minlp->model_status);

		gevLogStat(gev, "");
		if (bb.bestSolution()) {
			snprintf(buffer, 1024, "MINLP solution: %20.10g   (%d nodes, %g seconds)", best_val, bb.numNodes(), gevTimeDiffStart(gev) - minlp->nlp->clockStart);
			gevLogStat(gev, buffer);
		}
		if (best_bound > -1e200 && best_bound < 1e200) {
			snprintf(buffer, 1024, "Best possible: %21.10g\n", best_bound);
			gevLogStat(gev, buffer);

			if (bb.bestSolution()) {
				snprintf(buffer, 1024, "Absolute gap: %22.5g\nRelative gap: %22.5g\n", CoinAbs(best_val-best_bound), CoinAbs(best_val-best_bound)/CoinMax(CoinAbs(best_bound), 1.));
				gevLogStat(gev, buffer);
			}
		}
		
	} catch (IpoptException error) {
		gevLogStat(gev, error.Message().c_str());
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
	  return -1;
  } catch(CoinError &error) {
  	snprintf(buffer, 1024, "%s::%s\n%s", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
		gevLogStat(gev, buffer);
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
	  return 1;
  } catch (TNLPSolver::UnsolvedError *E) { 	// there has been a failure to solve a problem with Ipopt.
		snprintf(buffer, 1024, "Error: %s exited with error %s", E->solverName().c_str(), E->errorName().c_str());
		gevLogStat(gev, buffer);
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

bool GamsCouenne::isMIP() {
	return !gmoNLNZ(gmo) && !gmoObjNLNZ(gmo);
}

CouenneProblem* GamsCouenne::setupProblem() {
	CouenneProblem* prob = new CouenneProblem(NULL, NULL, jnlst);
	
	//add variables
	for (int i = 0; i < gmoN(gmo); ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				prob->addVariable(false, prob->domain());
				break;
			case var_B:
			case var_I:
				prob->addVariable(true, prob->domain());
				break;
			case var_S1:
			case var_S2:
		  	//TODO prob->addVariable(false, prob->domain());
		  	gevLogStat(gev, "Special ordered sets not supported by Gams/Couenne link yet.");
		  	return NULL;
			case var_SC:
			case var_SI:
		  	gevLogStat(gev, "Semicontinuous and semiinteger variables not supported by Couenne.");
		  	return NULL;
			default:
		  	gevLogStat(gev, "Unknown variable type.");
		  	return NULL;
		}
	}
	
	// add variable bounds and initial values
	CouNumber* x_ = new CouNumber[gmoN(gmo)];
	CouNumber* lb = new CouNumber[gmoN(gmo)];
	CouNumber* ub = new CouNumber[gmoN(gmo)];
	
	gmoGetVarL(gmo, x_);
	gmoGetVarLower(gmo, lb);
	gmoGetVarUpper(gmo, ub);
	
	// translate from gmoM/Pinf to Couenne infinity
	for (int i = 0; i < gmoN(gmo); ++i)	{
		if (lb[i] <= gmoMinf(gmo))
			lb[i] = -COUENNE_INFINITY;
		if (ub[i] >= gmoPinf(gmo))
			ub[i] =  COUENNE_INFINITY;
	}
	
	prob->domain()->push(gmoN(gmo), x_, lb, ub);
	
	delete[] x_;
	delete[] lb;
	delete[] ub;
	

	int* opcodes = new int[gmoMaxSingleFNL(gmo)+1];
	int* fields  = new int[gmoMaxSingleFNL(gmo)+1];
	int constantlen = gmoNLConst(gmo);
	double* constants = (double*)gmoPPool(gmo); //new double[gmoNLConst(gmo)];
	int codelen;
	
//	memcpy(constants, gmoPPool(gmo), constantlen*sizeof(double));
//	for (int i = 0; i < constantlen; ++i)
//		if (fabs(constants[i]) < COUENNE_EPS)
//			constants[i] = 0.;

	exprGroup::lincoeff lin;
	expression *body = NULL;

	// add objective function: first linear part, then nonlinear
	double isMin = (gmoSense(gmo) == Obj_Min) ? 1 : -1;
	
	lin.reserve(gmoObjNZ(gmo) - gmoObjNLNZ(gmo));
	
	int* colidx = new int[gmoObjNZ(gmo)];
	double* val = new double[gmoObjNZ(gmo)];
	int* nlflag = new int[gmoObjNZ(gmo)];
	int dummy;
	
	if (gmoObjNZ(gmo)) nlflag[0] = 0; // workaround for gmo bug
	gmoGetObjSparse(gmo, colidx, val, nlflag, &dummy, &dummy);
	for (int i = 0; i < gmoObjNZ(gmo); ++i) {
		if (nlflag[i])
			continue;
		lin.push_back(pair<exprVar*, CouNumber>(prob->Var(colidx[i]), isMin*val[i]));
	}
  assert((int)lin.size() == gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

	delete[] colidx;
	delete[] val;
	delete[] nlflag;
		
	if (gmoObjNLNZ(gmo)) {
		gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);
		
		expression** nl = new expression*[1];
		nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
		if (!nl[0])
			return NULL;

		double objjacval = isMin*gmoObjJacVal(gmo);
		//std::clog << "obj jac val: " << objjacval << std::endl;
		if (objjacval == 1.) { // scale by -1/objjacval = negate
			nl[0] = new exprOpp(nl[0]);
		} else if (objjacval != -1.) { // scale by -1/objjacval
			nl[0] = new exprMul(nl[0], new exprConst(-1/objjacval));
		}
		
		body = new exprGroup(isMin*gmoObjConst(gmo), lin, nl, 1);
	} else {
		body = new exprGroup(isMin*gmoObjConst(gmo), lin, NULL, 0);			
	}
	
	prob->addObjective(body, "min");
	
	int nz = gmoNZ(gmo);
	double* values  = new double[nz];
	int* rowstarts  = new int[gmoM(gmo)+1];
	int* colindexes = new int[nz];
	int* nlflags    = new int[nz];
	
	gmoGetMatrixRow(gmo, rowstarts, colindexes, values, nlflags);
	rowstarts[gmoM(gmo)] = nz;

	for (int i = 0; i < gmoM(gmo); ++i) {
		lin.clear();
		lin.reserve(rowstarts[i+1] - rowstarts[i]);
		for (int j = rowstarts[i]; j < rowstarts[i+1]; ++j) {
			if (nlflags[j])
				continue;
			lin.push_back(pair<exprVar*, CouNumber>(prob->Var(colindexes[j]), values[j]));
		}
#if defined(GAMS_BUILD) || defined(GMOAPIVERSION)
		if (gmoGetEquOrderOne(gmo, i) > order_L) {
#else
		if (gmoNLfunc(gmo, i)) {
#endif
			gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
			expression** nl = new expression*[1];
			nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
			if (!nl[0])
				return NULL;
			body = new exprGroup(0., lin, nl, 1);
		} else {
			body = new exprGroup(0., lin, NULL, 0);			
		}
		
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E:
				prob->addEQConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_L:
				prob->addLEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_G:
				prob->addGEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_N:
				// TODO I doubt that adding a RNG constraint with -infty/infty bounds would work here
				gevLogStat(gev, "Free constraints not supported by Gams/Couenne link yet. Constraint ignored.");
				break;
		}
	}
	
	delete[] opcodes;
	delete[] fields;
	delete[] values;
	delete[] rowstarts;
	delete[] colindexes;
	delete[] nlflags;
//	delete[] constants;
	
	return prob;
}

CouenneProblem* GamsCouenne::setupProblemNew() {
	CouenneProblem* prob = new CouenneProblem(NULL, NULL, jnlst);
	
	if (gmoQMaker(gmo, 0.5) < 0) { // negative number is error; positive number is number of nonquadratic nonlinear equations
		gevLogStat(gev, "ERROR: Problems extracting information on quadratic functions in GMO.");
		return NULL;
	}
	
	int nz, nlnz;

	double* lincoefs  = new double[gmoN(gmo)];
	int*    lincolidx = new int[gmoN(gmo)];
	int*    nlflag    = new int[gmoN(gmo)];
	
	double* quadcoefs = new double[gmoMaxQnz(gmo)];
	int* qrow =  new int[gmoMaxQnz(gmo)];
	int* qcol =  new int[gmoMaxQnz(gmo)];
	int qnz, qdiagnz;
	
	int* opcodes = new int[gmoMaxSingleFNL(gmo)+1];
	int* fields  = new int[gmoMaxSingleFNL(gmo)+1];
	int constantlen = gmoNLConst(gmo);
	double* constants = (double*)gmoPPool(gmo); //new double[gmoNLConst(gmo)];
	int codelen;
	
	//	memcpy(constants, gmoPPool(gmo), constantlen*sizeof(double));
	//	for (int i = 0; i < constantlen; ++i)
	//		if (fabs(constants[i]) < COUENNE_EPS)
	//			constants[i] = 0.;

	exprGroup::lincoeff lin;
	expression *body = NULL;

	// add variables
	for (int i = 0; i < gmoN(gmo); ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				prob->addVariable(false, prob->domain());
				break;
			case var_B:
			case var_I:
				prob->addVariable(true, prob->domain());
				break;
			case var_S1:
			case var_S2:
		  	//TODO prob->addVariable(false, prob->domain());
		  	gevLogStat(gev, "Special ordered sets not supported by Gams/Couenne link yet.");
		  	return NULL;
			case var_SC:
			case var_SI:
		  	gevLogStat(gev, "Semicontinuous and semiinteger variables not supported by Couenne.");
		  	return NULL;
			default:
		  	gevLogStat(gev, "Unknown variable type.");
		  	return NULL;
		}
	}
	
	// add variable bounds and initial values
	CouNumber* x_ = new CouNumber[gmoN(gmo)];
	CouNumber* lb = new CouNumber[gmoN(gmo)];
	CouNumber* ub = new CouNumber[gmoN(gmo)];
	
	gmoGetVarL(gmo, x_);
	gmoGetVarLower(gmo, lb);
	gmoGetVarUpper(gmo, ub);
	
	// translate from gmoM/Pinf to Couenne infinity
	for (int i = 0; i < gmoN(gmo); ++i)	{
		if (lb[i] <= gmoMinf(gmo))
			lb[i] = -COUENNE_INFINITY;
		if (ub[i] >= gmoPinf(gmo))
			ub[i] =  COUENNE_INFINITY;
	}
	
	prob->domain()->push(gmoN(gmo), x_, lb, ub);
	
	delete[] x_;
	delete[] lb;
	delete[] ub;

	// add objective function
	double isMin = (gmoSense(gmo) == Obj_Min) ? 1 : -1;
	gmoGetObjSparse(gmo, lincolidx, lincoefs, nlflag, &nz, &nlnz);

	if (gmoGetObjOrder(gmo) <= order_Q) {
		lin.reserve(nz);
		for (int i = 0; i < nz; ++i)
			lin.push_back(pair<exprVar*, CouNumber>(prob->Var(lincolidx[i]), isMin*lincoefs[i]));
		
		if( gmoGetObjOrder(gmo) == order_Q ) {
			gmoGetObjQ(gmo, &qnz, &qdiagnz, qcol, qrow, quadcoefs);
			expression** quadpart = new expression*[qnz];
			
			for (int j = 0; j < qnz; ++j) {
				assert(qcol[j] >= 0);
				assert(qrow[j] >= 0);
				assert(qcol[j] < gmoN(gmo));
				assert(qrow[j] < gmoN(gmo));
				if (qcol[j] == qrow[j]) {
					quadcoefs[j] /= 2.0; // for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO.
					quadpart[j] = new exprPow(new exprClone(prob->Var(qcol[j])), new exprConst(2.));
				} else {
					quadpart[j] = new exprMul(new exprClone(prob->Var(qcol[j])), new exprClone(prob->Var(qrow[j])));
				}
				quadcoefs[j] *= isMin;
				if (quadcoefs[j] == -1.0)
					quadpart[j] = new exprOpp(quadpart[j]);
				else if (quadcoefs[j] != 1.0)
					quadpart[j] = new exprMul(quadpart[j], new exprConst(quadcoefs[j]));
			}

			body = new exprGroup(isMin*gmoObjConst(gmo), lin, quadpart, qnz);

		} else {
			body = new exprGroup(isMin*gmoObjConst(gmo), lin, NULL, 0);
		}
		
	} else { /* general nonlinear objective */ 
		lin.reserve(nz-nlnz);
		for (int i = 0; i < nz; ++i) {
			if (nlflag[i])
				continue;
			lin.push_back(pair<exprVar*, CouNumber>(prob->Var(lincolidx[i]), isMin*lincoefs[i]));
		}
		
		gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);
		
		expression** nl = new expression*[1];
		nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
		if (!nl[0])
			return NULL;

		double objjacval = isMin*gmoObjJacVal(gmo);
		//std::clog << "obj jac val: " << objjacval << std::endl;
		if (objjacval == 1.) { // scale by -1/objjacval = negate
			nl[0] = new exprOpp(nl[0]);
		} else if (objjacval != -1.) { // scale by -1/objjacval
			nl[0] = new exprMul(nl[0], new exprConst(-1/objjacval));
		}
		
		body = new exprGroup(isMin*gmoObjConst(gmo), lin, nl, 1);
	}

	prob->addObjective(body, "min");
	
	for (int i = 0; i < gmoM(gmo); ++i) {
		gmoGetRowSparse(gmo, i, lincolidx, lincoefs, nlflag, &nz, &nlnz);
		lin.clear();
		
		if (gmoGetEquOrderOne(gmo, i) <= order_Q) {
			lin.reserve(nz);
			for (int j = 0; j < nz; ++j)
				lin.push_back(pair<exprVar*, CouNumber>(prob->Var(lincolidx[j]), isMin*lincoefs[j]));
			
			if( gmoGetObjOrder(gmo) == order_Q ) {
				gmoGetRowQ(gmo, i, &qnz, &qdiagnz, qcol, qrow, quadcoefs);
				expression** quadpart = new expression*[qnz];
				
				for (int j = 0; j < qnz; ++j) {
					assert(qcol[j] >= 0);
					assert(qrow[j] >= 0);
					assert(qcol[j] < gmoN(gmo));
					assert(qrow[j] < gmoN(gmo));
					if (qcol[j] == qrow[j]) {
						quadcoefs[j] /= 2.0; // for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO.
						quadpart[j] = new exprPow(new exprClone(prob->Var(qcol[j])), new exprConst(2.));
					} else {
						quadpart[j] = new exprMul(new exprClone(prob->Var(qcol[j])), new exprClone(prob->Var(qrow[j])));
					}
					quadcoefs[j] *= isMin;
					if (quadcoefs[j] == -1.0)
						quadpart[j] = new exprOpp(quadpart[j]);
					else if (quadcoefs[j] != 1.0)
						quadpart[j] = new exprMul(quadpart[j], new exprConst(quadcoefs[j]));
				}

				body = new exprGroup(0, lin, quadpart, qnz);

			} else {
				body = new exprGroup(0, lin, NULL, 0);
			}
			
		} else { /* general nonlinear constraint */ 
			lin.reserve(nz-nlnz);
			for (int j = 0; j < nz; ++j) {
				if (nlflag[j])
					continue;
				lin.push_back(pair<exprVar*, CouNumber>(prob->Var(lincolidx[j]), isMin*lincoefs[j]));
			}

			gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
			expression** nl = new expression*[1];
			nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
			if (!nl[0])
				return NULL;
			
			body = new exprGroup(0, lin, nl, 1);
		}

		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E:
				prob->addEQConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_L:
				prob->addLEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_G:
				prob->addGEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_N:
				// TODO I doubt that adding a RNG constraint with -infty/infty bounds would work here
				gevLogStat(gev, "Free constraints not supported by Gams/Couenne link yet. Constraint ignored.");
				break;
		}
	}
	
  delete[] lincolidx;
	delete[] lincoefs;
	delete[] nlflag;

	delete[] quadcoefs;
	delete[] qrow;
	delete[] qcol;
	
	delete[] opcodes;
	delete[] fields;
//	delete[] constants;
	
	return prob;
}

expression* GamsCouenne::parseGamsInstructions(CouenneProblem* prob, int codelen, int* opcodes, int* fields, int constantlen, double* constants) {
	const bool debugoutput = false;

	list<expression*> stack;
	
	int nargs = -1;

	for (int pos = 0; pos < codelen; ++pos)
	{	
		GamsOpCode opcode = (GamsOpCode)opcodes[pos];
		int address = fields[pos]-1;

		if (debugoutput) std::clog << '\t' << GamsOpCodeName[opcode] << ": ";
		
		expression* exp = NULL;
		
		switch(opcode) {
			case nlNoOp : { // no operation
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushV : { // push variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "push variable " << address << std::endl;
				exp = new exprClone(prob->Variables()[address]);
			} break;
			case nlPushI : { // push constant
				if (debugoutput) std::clog << "push constant " << constants[address] << std::endl;
				exp = new exprConst(constants[address]);
			} break;
			case nlStore: { // store row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlAdd : { // add
				if (debugoutput) std::clog << "add" << std::endl;
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				exp = new exprSum(term1, term2);
			} break;
			case nlAddV: { // add variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "add variable " << address << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				exp = new exprSum(term1, term2);
			} break;
			case nlAddI: { // add immediate
				if (debugoutput) std::clog << "add constant " << constants[address] << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprSum(term1, term2);
			} break;
			case nlSub: { // minus
				if (debugoutput) std::clog << "minus" << std::endl;
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				exp = new exprSub(term2, term1);
			} break;
			case nlSubV: { // subtract variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "substract variable " << address << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				exp = new exprSub(term1, term2);
			} break;
			case nlSubI: { // subtract immediate
				if (debugoutput) std::clog << "substract constant " << constants[address] << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprSub(term1, term2);
			} break;
			case nlMul: { // multiply
				if (debugoutput) std::clog << "multiply" << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				exp = new exprMul(term1, term2);
			} break;
			case nlMulV: { // multiply variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "multiply variable " << address << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				exp = new exprMul(term1, term2);
			} break;
			case nlMulI: { // multiply immediate
				if (debugoutput) std::clog << "multiply constant " << constants[address] << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprMul(term1, term2);
			} break;
			case nlDiv: { // divide
				if (debugoutput) std::clog << "divide" << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				if (term2->Type() == CONST)
					exp = new exprMul(term2, new exprInv(term1));
				else
					exp = new exprDiv(term2, term1);
			} break;
			case nlDivV: { // divide variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "divide variable " << address << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				if (term1->Type() == CONST)
					exp = new exprMul(term1, new exprInv(term2));
				else
					exp = new exprDiv(term1, term2);
			} break;
			case nlDivI: { // divide immediate
				if (debugoutput) std::clog << "divide constant " << constants[address] << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprDiv(term1, term2);
			} break;
			case nlUMin: { // unary minus
				if (debugoutput) std::clog << "negate" << std::endl;
				
				expression* term = stack.back(); stack.pop_back();
				exp = new exprOpp(term);
			} break;
			case nlUMinV: { // unary minus variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "push negated variable " << address << std::endl;

				exp = new exprOpp(new exprClone(prob->Variables()[address]));
			} break;
			case nlCallArg1 :
			case nlCallArg2 :
			case nlCallArgN : {
				if (debugoutput) std::clog << "call function ";
				GamsFuncCode func = GamsFuncCode(address+1); // here the shift by one was not a good idea
				
				switch (func) {
					case fnmin : {
						if (debugoutput) std::clog << "min" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						exp = new exprMin(term1, term2);
					} break;
					case fnmax : {
						if (debugoutput) std::clog << "max" << std::endl;

						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						exp = new exprMax(term1, term2);
					} break;
					case fnsqr : {
						if (debugoutput) std::clog << "square" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprPow(term, new exprConst(2.));
					} break;
					case fnexp:
					case fnslexp:
					case fnsqexp: {
						if (debugoutput) std::clog << "exp" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprExp(term);
					} break;
					case fnlog : {
						if (debugoutput) std::clog << "ln" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprLog(term);
					} break;
					case fnlog10:
					case fnsllog10:
					case fnsqlog10: {
						if (debugoutput) std::clog << "log10 = ln * 1/ln(10)" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprMul(new exprLog(term), new exprConst(1./log(10.)));
					} break;
					case fnlog2 : {
						if (debugoutput) std::clog << "log2 = ln * 1/ln(2)" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprMul(new exprLog(term), new exprConst(1./log(2.)));
					} break;
					case fnsqrt: {
						if (debugoutput) std::clog << "sqrt" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprPow(term, new exprConst(.5));
					} break;
					case fnabs: {
						if (debugoutput) std::clog << "abs" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprAbs(term);
					} break;
					case fncos: {
						if (debugoutput) std::clog << "cos" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprCos(term);
					} break;
					case fnsin: {
						if (debugoutput) std::clog << "sin" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprSin(term);
					} break;
					case fnpower:
					case fnrpower: { // x ^ y
						if (debugoutput) std::clog << "power" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						if (term1->Type() == CONST)
							exp = new exprPow(term2, term1);
						else
							exp = new exprExp(new exprMul(new exprLog(term2), term1));
					} break;
					case fncvpower: { // constant ^ x
						if (debugoutput) std::clog << "power" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();

						assert(term2->Type() == CONST);
						exp = new exprExp(new exprMul(new exprConst(log(((exprConst*)term2)->Value())), term1));
						delete term2;
					} break;
					case fnvcpower: { // x ^ constant
						if (debugoutput) std::clog << "power" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						assert(term1->Type() == CONST);
						exp = new exprPow(term2, term1);
					} break;
					case fnpi: {
						if (debugoutput) std::clog << "pi" << std::endl;
						//TODO
						assert(false);
					} break;
					case fndiv:
					case fndiv0: {
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						if (term2->Type() == CONST)
							exp = new exprMul(term2, new exprInv(term1));
						else
							exp = new exprDiv(term2, term1);
					} break;
					case fnslrec: // 1/x
					case fnsqrec: { // 1/x
						if (debugoutput) std::clog << "divide" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprInv(term);
					} break;
			    case fnpoly: { /* simple polynomial */
						if (debugoutput) std::clog << "polynom of degree " << nargs-1 << std::endl;
						assert(nargs >= 0);
						switch (nargs) {
							case 0:
								delete stack.back(); stack.pop_back(); // delete variable of polynom
								exp = new exprConst(0.);
								break;
							case 1: // "constant" polynom
								exp = stack.back(); stack.pop_back();
								delete stack.back(); stack.pop_back(); // delete variable of polynom
								break;
							default: { // polynom is at least linear
								std::vector<expression*> coeff(nargs);
								while (nargs) {
									assert(!stack.empty());
									coeff[nargs-1] = stack.back();
									stack.pop_back();
									--nargs;
								}
								assert(!stack.empty());
								expression* var = stack.back(); stack.pop_back();
								expression** monoms = new expression*[coeff.size()];
								monoms[0] = coeff[0];
								monoms[1] = new exprMul(coeff[1], var);
								for (size_t i = 2; i < coeff.size(); ++i)
									monoms[i] = new exprMul(coeff[i], new exprPow(new exprClone(var), new exprConst(i)));
								exp = new exprSum(monoms, coeff.size());
							}
						}
						nargs = -1;
			    } break;
					case fnceil: case fnfloor: case fnround:
					case fnmod: case fntrunc: case fnsign:
					case fnarctan: case fnerrf: case fndunfm:
					case fndnorm: case fnerror: case fnfrac: case fnerrorl:
			    case fnfact /* factorial */: 
			    case fnunfmi /* uniform random number */:
			    case fnncpf /* fischer: sqrt(x1^2+x2^2+2*x3) */:
			    case fnncpcm /* chen-mangasarian: x1-x3*ln(1+exp((x1-x2)/x3))*/:
			    case fnentropy /* x*ln(x) */: case fnsigmoid /* 1/(1+exp(-x)) */:
			    case fnboolnot: case fnbooland:
			    case fnboolor: case fnboolxor: case fnboolimp:
			    case fnbooleqv: case fnrelopeq: case fnrelopgt:
			    case fnrelopge: case fnreloplt: case fnrelople:
			    case fnrelopne: case fnifthen:
			    case fnedist /* euclidian distance */:
			    case fncentropy /* x*ln((x+d)/(y+d))*/:
			    case fngamma: case fnloggamma: case fnbeta:
			    case fnlogbeta: case fngammareg: case fnbetareg:
			    case fnsinh: case fncosh: case fntanh:
			    case fnsignpower /* sign(x)*abs(x)^c */:
			    case fnncpvusin /* veelken-ulbrich */:
			    case fnncpvupow /* veelken-ulbrich */:
			    case fnbinomial:
			    case fntan: case fnarccos:
			    case fnarcsin: case fnarctan2 /* arctan(x2/x1) */:
					default : {
						char buffer[256];
						sprintf(buffer, "Gams function code %d not supported.", func);
						gevLogStat(gev, buffer);
						return NULL;
					}
				}
			} break;
			case nlMulIAdd: {
				if (debugoutput) std::clog << "multiply constant " << constants[address] << " and add " << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				term1 = new exprMul(term1, new exprConst(constants[address]));
				expression* term2 = stack.back(); stack.pop_back();
				
				exp = new exprSum(term1, term2);
			} break;
			case nlFuncArgN : {
				nargs = address;
				if (debugoutput) std::clog << nargs << " arguments" << std::endl;
			} break;
			case nlArg: {
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlHeader: { // header
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushZero: {
				if (debugoutput) std::clog << "push constant zero" << std::endl;
				exp = new exprConst(0.);
			} break;
			case nlStoreS: { // store scaled row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			// the following three should have been taken out by reorderInstr above; the remaining ones seem to be unused by now
			case nlPushS: // duplicate value from address levels down on top of stack
			case nlPopup: // duplicate value from this level to at address levels down and pop entries in between
			case nlSwap: // swap two positions on top of stack
			case nlAddL: // add local
			case nlSubL: // subtract local
			case nlMulL: // multiply local
			case nlDivL: // divide local
			case nlPushL: // push local
			case nlPopL: // pop local
			case nlPopDeriv: // pop derivative
			case nlUMinL: // push umin local
			case nlPopDerivS: // store scaled gradient
			case nlEquScale: // equation scale
			case nlEnd: // end of instruction list
			default: {
				char buffer[256];
				sprintf(buffer, "Gams instruction %d not supported.", opcode);
				gevLogStat(gev, buffer);
				return NULL;
			}
		}
		
		if (exp)
			stack.push_back(exp);
	}

	assert(stack.size() == 1);	
	return stack.back();
}

CouenneProblem* GamsCouenne::setupProblemMIQQP() {
//	printf("using MIQQP problem setup method\n");
	
	int do2dir = 1;
	int dohess = 1;
	gmoHessLoad(gmo, 0, -1, &do2dir, &dohess);
	if (!dohess) {
		gevLogStat(gev, "Failed to initialize Hessian structure. Trying usual setupProblem.");
		return setupProblem();
  }

	CouenneProblem* prob = new CouenneProblem(NULL, NULL, jnlst);

	//add variables
	for (int i = 0; i < gmoN(gmo); ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				prob->addVariable(false, prob->domain());
				break;
			case var_B:
			case var_I:
				prob->addVariable(true, prob->domain());
				break;
			case var_S1:
			case var_S2:
		  	//TODO prob->addVariable(false, prob->domain());
		  	gevLogStat(gev, "Special ordered sets not supported by Gams/Couenne link yet.");
		  	return NULL;
			case var_SC:
			case var_SI:
		  	gevLogStat(gev, "Semicontinuous and semiinteger variables not supported by Couenne.");
		  	return NULL;
			default:
		  	gevLogStat(gev, "Unknown variable type.");
		  	return NULL;
		}
	}
	
	// add variable bounds and initial values
	CouNumber* x_ = new CouNumber[gmoN(gmo)];
	CouNumber* lb = new CouNumber[gmoN(gmo)];
	CouNumber* ub = new CouNumber[gmoN(gmo)];
	
	gmoGetVarL(gmo, x_);
	gmoGetVarLower(gmo, lb);
	gmoGetVarUpper(gmo, ub);
	
	// translate from gmoM/Pinf to Couenne infinity
	for (int i = 0; i < gmoN(gmo); ++i)	{
		if (lb[i] <= gmoMinf(gmo))
			lb[i] = -COUENNE_INFINITY;
		if (ub[i] >= gmoPinf(gmo))
			ub[i] =  COUENNE_INFINITY;
	}
	
	prob->domain()->push(gmoN(gmo), x_, lb, ub);
	
	delete[] x_;
	delete[] lb;
	delete[] ub;
	
	exprGroup::lincoeff lin;
	expression *body = NULL;
//	std::vector<quadElem> qcoeff;
	
  lin.reserve(gmoN(gmo));
//  qcoeff.reserve(gmoHessLagNz(gmo));

	double* null   = new double[gmoN(gmo)];
	double* lambda = new double[gmoM(gmo)];
	for (int i = 0; i < gmoN(gmo); ++i)
		null[i] = 0.;
	for (int i = 0; i < gmoM(gmo); ++i)
		lambda[i] = 0.;
	
	double constant, dummy;
	double* linear = new double[gmoN(gmo)];
	int nerror;
	
	int* hess_iRow   = new int[gmoHessLagNz(gmo)];
	int* hess_jCol   = new int[gmoHessLagNz(gmo)];
	double* hess_val = new double[gmoHessLagNz(gmo)];
	
	gmoHessLagStruct(gmo, hess_iRow, hess_jCol);

	// add objective function

	double isMin = (gmoSense(gmo) == Obj_Min) ? 1 : -1;

	memset(linear, 0, gmoN(gmo)*sizeof(double));
	nerror = gmoEvalObjGrad(gmo, null, &constant, linear, &dummy);
	assert(nerror == 0);

	for (int i = 0; i < gmoN(gmo); ++i) {
		if (!linear[i])
			continue;
		lin.push_back(pair<exprVar*, CouNumber>(prob->Var(i), isMin*linear[i]));
	}
  assert((int)lin.size() == gmoObjNZ(gmo));

	if (gmoObjNLNZ(gmo)) {
		memset(hess_val, 0, gmoHessLagNz(gmo)*sizeof(double));
		nerror = gmoHessLagValue(gmo, null, lambda, hess_val, isMin, 0.);
		if (nerror) {
			gevLogStat(gev, "Error evaluation hessian of objective function.\n");
			return NULL;
		}
		
		int nzcount = 0;
		for (int i = 0; i < gmoHessLagNz(gmo); ++i) {
			if (hess_val[i])
				++nzcount;
		}

		expression** summands = new expression*[nzcount];
		int k = 0;
		for (int i = 0; i < gmoHessLagNz(gmo); ++i) {
			if (!hess_val[i])
				continue;
			double coeff = hess_val[i];
			if (hess_iRow[i] == hess_jCol[i]) {
				summands[k] = new exprPow(new exprClone(prob->Var(hess_iRow[i])), new exprConst(2.));
				if (coeff != 2.)
					summands[k] = new exprMul(new exprConst(coeff/2.), summands[k]);
			} else if (coeff == 1.) {
				summands[k] = new exprMul(new exprClone(prob->Var(hess_iRow[i])), new exprClone(prob->Var(hess_jCol[i])));
			} else {
				expression** prod = new expression*[3];
				prod[0] = new exprConst(coeff);
				prod[1] = new exprClone(prob->Var(hess_iRow[i]));
				prod[2] = new exprClone(prob->Var(hess_jCol[i]));
				summands[k] = new exprMul(prod, 3);
			}
			++k;
		}
		
		body = new exprGroup(isMin*constant, lin, summands, nzcount);

//		for (int i = 0; i < gmoHessLagNz(gmo); ++i) {
//			if (!hess_val[i])
//				continue;
//			qcoeff.push_back(quadElem(prob->Var(hess_iRow[i]), prob->Var(hess_jCol[i]),
//				(hess_iRow[i] == hess_jCol[i] ? 0.5 : 1) * 2 * hess_val[i]));
//		}
		
//	  body = new exprQuad(isMin*constant, lin, qcoeff, NULL, 0);
	} else {
	  body = new exprGroup(isMin*constant, lin, NULL, 0);
	}

	prob->addObjective(body, "min");

	// add constraints
	
	for (int i = 0; i < gmoM(gmo); ++i) {
		lin.resize(0);
//		qcoeff.clear();

		memset(linear, 0, gmoN(gmo)*sizeof(double));
		nerror = gmoEvalGrad(gmo, i, null, &constant, linear, &dummy);
		assert(nerror == 0);

		for (int j = 0; j < gmoN(gmo); ++j) {
			if (!linear[j])
				continue;
			lin.push_back(pair<exprVar*, CouNumber>(prob->Var(j), linear[j]));
		}

#if defined(GAMS_BUILD) || defined(GMOAPIVERSION)
		if (gmoGetEquOrderOne(gmo, i) > order_L)
#else
		if (gmoNLfunc(gmo, i))
#endif
		{
			lambda[i] = -1.;
			memset(hess_val, 0, gmoHessLagNz(gmo)*sizeof(double));
			nerror = gmoHessLagValue(gmo, null, lambda, hess_val, 0., 1.);
			lambda[i] = 0.;
			if (nerror) {
				gevLogStat(gev, "Error evaluation hessian of constraint function.\n");
				return NULL;
			}

			int nzcount = 0;
			for (int j = 0; j < gmoHessLagNz(gmo); ++j) {
				if (hess_val[j])
					++nzcount;
			}
			
			expression** summands = new expression*[nzcount];
			int k = 0;
			for (int j = 0; j < gmoHessLagNz(gmo); ++j) {
				if (!hess_val[j])
					continue;
				double coeff = hess_val[j];
				if (hess_iRow[j] == hess_jCol[j]) {
					summands[k] = new exprPow(new exprClone(prob->Var(hess_iRow[j])), new exprConst(2.));
					if (coeff != 2.)
						summands[k] = new exprMul(new exprConst(coeff/2.), summands[k]);
				} else if (coeff == 1.) {
					summands[k] = new exprMul(new exprClone(prob->Var(hess_iRow[j])), new exprClone(prob->Var(hess_jCol[j])));
				} else {
					expression** prod = new expression*[3];
					prod[0] = new exprConst(coeff);
					prod[1] = new exprClone(prob->Var(hess_iRow[j]));
					prod[2] = new exprClone(prob->Var(hess_jCol[j]));
					summands[k] = new exprMul(prod, 3);
				}
				++k;
			}
			
			body = new exprGroup(constant, lin, summands, nzcount);
			
//			for (int j = 0; j < gmoHessLagNz(gmo); ++j) {
//				if (!hess_val[j])
//					continue;
//				qcoeff.push_back(quadElem(prob->Var(hess_iRow[j]), prob->Var(hess_jCol[j]),
//					(hess_iRow[j] == hess_jCol[j] ? 0.5 : 1) * 2 * hess_val[j]));
//			}
//			
//		  body = new exprQuad(constant, lin, qcoeff, NULL, 0);
		} else {
		  body = new exprGroup(constant, lin, NULL, 0);
		}
		
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E:
				prob->addEQConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_L:
				prob->addLEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_G:
				prob->addGEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_N:
				// TODO I doubt that adding a RNG constraint with -infty/infty bounds would work here
				gevLogStat(gev, "Free constraints not supported by Gams/Couenne link yet. Constraint ignored.");
				break;
		}
	}
	
	delete[] null;
	delete[] lambda;
	delete[] linear;
	delete[] hess_iRow;
	delete[] hess_jCol;
	delete[] hess_val;

	return prob;
}

void GamsCouenne::printOptions() {
	const Bonmin::RegisteredOptions::RegOptionsList& optionlist(roptions->RegisteredOptionsList());

	std::ofstream tabfile("couenne_options_table.tex");
	roptions->writeLatexOptionsTable(tabfile, Bonmin::RegisteredOptions::CouenneCategory);

	// options sorted by category
	std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;

	for (Bonmin::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it!=optionlist.end(); ++it) {
		//  	jnlst->Printf(J_SUMMARY, J_DOCUMENTATION, "%s %s %d\n", it->first.c_str(), it->second->RegisteringCategory().c_str(), regoptions->categoriesInfo(it->second->RegisteringCategory()));

		std::string category(it->second->RegisteringCategory());

		if (category.empty()) continue;
		// skip ipopt and bonmin options
		if (roptions->categoriesInfo(category)==Bonmin::RegisteredOptions::IpoptCategory) continue;
		if (roptions->categoriesInfo(category)==Bonmin::RegisteredOptions::BonminCategory) continue;

//		if (it->second->Name()=="nlp_solver" ||
//				it->second->Name()=="file_solution" ||
//				it->second->Name()=="sos_constraints")
//			continue;
//
//		if (category=="Bonmin ecp based strong branching")
//			category="ECP based strong branching";
//		if (category=="MILP cutting planes in hybrid")
//			category+=" algorithm (B-Hyb)";
//		if (category=="Nlp solution robustness")
//			category="NLP solution robustness";
//		if (category=="Nlp solve options in B-Hyb")
//			category="NLP solves in hybrid algorithm (B-Hyb)";
//		if (category=="Options for MILP subsolver in OA decomposition" || category=="Options for OA decomposition")
//			category="Outer Approximation Decomposition (B-OA)";
//		if (category=="Options for ecp cuts generation")
//			category="ECP cuts generation";
//		if (category=="Options for non-convex problems")
//			category="Nonconvex problems";
//		if (category=="Output ond log-levels ptions")
//			category="Output";
//		if (category=="nlp interface option")
//			category="NLP interface";
//
//		if (it->second->Name()=="oa_cuts_log_level" ||
//				it->second->Name()=="nlp_log_level" ||
//				it->second->Name()=="milp_log_level" ||
//				it->second->Name()=="oa_log_level" ||
//				it->second->Name()=="oa_log_frequency")
//			category="Output";

		opts[category].push_back(it->second);
	}

	for (std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ!=opts.end(); ++it_categ) {
		std::string category(it_categ->first);
		//  	jnlst->Printf(J_SUMMARY, J_DOCUMENTATION, "category %s:\n", it_categ->first.c_str());
		jnlst->Printf(J_SUMMARY, J_DOCUMENTATION, "\\subsubsection{%s}\n", category.c_str());
		for (std::string::size_type spacepos = category.find(' '); spacepos != std::string::npos; spacepos = category.find(' '))
			category[spacepos]='_';
		jnlst->Printf(J_SUMMARY, J_DOCUMENTATION, "\\label{sec:%s}\n\n", category.c_str());

		for (std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt!=it_categ->second.end(); ++it_opt) {
			//    	jnlst->Printf(J_SUMMARY, J_DOCUMENTATION, "   %s\n", (*it_opt)->Name().c_str());
			(*it_opt)->OutputLatexDescription(*jnlst);
		}
	}
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

DllExport int STDCALL couReadyAPI(couRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr) {
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	if (!gevGetReady(msg, sizeof(msg)))
		return 1;
	return ((GamsCouenne*)Cptr)->readyAPI(Gptr, Optr);
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
