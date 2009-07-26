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
: gmo(NULL), gamscbc(NULL)
{
#ifdef GAMS_BUILD
	strcpy(couenne_message, "GAMS/CoinCouenne (Couenne Library 0.2)\nwritten by P. Belotti\n");
#else
	strcpy(couenne_message, "GAMS/Couenne (Couenne Library 0.2)\nwritten by P. Belotti\n");
#endif
}

GamsCouenne::~GamsCouenne() {
	delete gamscbc;
}

int GamsCouenne::readyAPI(struct gmoRec* gmo_, struct optRec* opt, struct dctRec* gcd) {
	gmo = gmo_;
	assert(gmo);
	assert(IsNull(minlp));

	if (getGmoReady())
		return 1;

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

	jnlst = new Ipopt::Journalist();
 	SmartPtr<Journal> jrnl = new GamsJournal(gmo, "console", J_ITERSUMMARY, J_STRONGWARNING);
	jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!jnlst->AddJournal(jrnl))
		gmoLogStat(gmo, "Failed to register GamsJournal for IPOPT output.");
	
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
	options->SetNumericValue("bound_relax_factor", 0, true, true);
	options->SetNumericValue("nlp_lower_bound_inf", gmoMinf(gmo), false, true);
	options->SetNumericValue("nlp_upper_bound_inf", gmoPinf(gmo), false, true);
	if (gmoUseCutOff(gmo))
		options->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == Obj_Min ? gmoCutOff(gmo) : -gmoCutOff(gmo), true, true);
	options->SetNumericValue("bonmin.allowable_gap", gmoOptCA(gmo), true, true);
	options->SetNumericValue("bonmin.allowable_fraction_gap", gmoOptCR(gmo), true, true);
	if (gmoNodeLim(gmo))
		options->SetIntegerValue("bonmin.node_limit", gmoNodeLim(gmo), true, true);
	options->SetNumericValue("bonmin.time_limit", gmoResLim(gmo), true, true);
  options->SetIntegerValue("bonmin.problem_print_level", J_STRONGWARNING, true, true); /* so that Couenne does not print the whole problem before reformulation */

  //TODO: still needed?  options->SetStringValue("delete_redundant", "no", "couenne.");

	if (gmoNLM(gmo) == 0  && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp))
		options->SetStringValue("hessian_constant", "yes", true, true); 

 	return 0;
}

int GamsCouenne::callSolver() {
	assert(gmo);
	
 	if (isMIP()) {
 		assert(gamscbc);
 		return gamscbc->callSolver();
 	}
	
 	CouenneProblem* problem = setupProblem();
 	if (!problem) {
 		gmoLogStat(gmo, "Error in setting up problem for Couenne.\n");
 		return -1;
 	}
	
	try {
		Bab bb;
		bb.setUsingCouenne(true);
		
		CouenneSetup couenne;
		couenne.setOptionsAndJournalist(roptions, options, jnlst);
		
		if (gmoOptFile(gmo)) {
			options->SetStringValue("print_user_options", "yes", true, true);
			char buffer[1024];
			gmoNameOptFile(gmo, buffer);
			couenne.BabSetupBase::readOptionsFile(buffer);
		}
		
		std::string libpath;
	#ifdef HAVE_HSL_LOADER
		if (options->GetStringValue("hsl_library", libpath, "")) {
			char buffer[512];
			if (LSL_loadHSL(libpath.c_str(), buffer, 512) != 0) {
				gmoLogStatPChar(gmo, "Failed to load HSL library at user specified path: ");
				gmoLogStat(gmo, buffer);
			  return -1;
			}
		}
	#endif
	#ifndef HAVE_PARDISO
		if (options->GetStringValue("pardiso_library", libpath, "")) {
			char buffer[512];
			if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512) != 0) {
				gmoLogStatPChar(gmo, "Failed to load Pardiso library at user specified path: ");
				gmoLogStat(gmo, buffer);
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
			int do2dir, dohess;
			if (gmoHessLoad(gmo, 0, -1, &do2dir, &dohess) || !dohess) { // TODO make "-1" a parameter (like rvhess in CONOPT)
				gmoLogStat(gmo, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
				options->SetStringValue("hessian_approximation", "limited-memory");
		  }
		}
		
		CouenneInterface* ci = new CouenneInterface;
		ci->initialize(roptions, options, jnlst, GetRawPtr(minlp));
		GamsMessageHandler* msghandler = new GamsMessageHandler(gmo);
		ci->passInMessageHandler(msghandler);
		
		char** argv = NULL;
		if (!couenne.InitializeCouenne(argv, problem, ci)) {
			gmoLogStat(gmo, "Reformulation finds model infeasible.\n");
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
			return 0;
		}
				
		minlp->nlp->clockStart = gmoTimeDiffStart(gmo);
		bb(couenne); // do branch and bound
		
		double best_val = bb.model().getObjValue();
		double best_bound = bb.model().getBestPossibleObjValue();
		if (gmoSense(gmo) == Obj_Max) {
			best_val   *= -1;
			best_bound *= -1;
		}

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
		
	} catch (IpoptException error) {
		gmoLogStat(gmo, error.Message().c_str());
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
	  return -1;
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
		  	gmoLogStat(gmo, "Special ordered sets not supported by Gams/Couenne link yet.");
		  	return NULL;
			case var_SC:
			case var_SI:
		  	gmoLogStat(gmo, "Semicontinuous and semiinteger variables not supported by Couenne.");
		  	return NULL;
			default:
		  	gmoLogStat(gmo, "Unknown variable type.");
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
	int* dummy  = new int[gmoObjNZ(gmo)];
	
	if (gmoObjNZ(gmo)) nlflag[0] = 0; // workaround for gmo bug
	gmoGetObjSparse(gmo, colidx, val, nlflag, dummy, dummy);
	for (int i = 0; i < gmoObjNZ(gmo); ++i) {
		if (nlflag[i])
			continue;
		lin.push_back(pair<exprVar*, CouNumber>(prob->Var(colidx[i]), isMin*val[i]));
	}
  assert((int)lin.size() == gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

	delete[] colidx;
	delete[] val;
	delete[] nlflag;
	delete[] dummy;
		
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
		if (gmoNLfunc(gmo, i)) {
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
				gmoLogStat(gmo, "Free constraints not supported by Gams/Couenne link yet. Constraint ignored.");
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
						gmoLogStat(gmo, buffer);
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
				gmoLogStat(gmo, buffer);
				return NULL;
			}
		}
		
		if (exp)
			stack.push_back(exp);
	}

	assert(stack.size() == 1);	
	return stack.back();
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
