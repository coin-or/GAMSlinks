// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsIpopt.hpp"
#include "GamsIpopt.h"
#include "GamsNLP.hpp"
#include "GamsJournal.hpp"
// to be sure to get (or not get) HAVE_M??? and HAVE_PARDISO defined
#include "IpoptConfig.h"

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

using namespace Ipopt;

GamsIpopt::GamsIpopt()
: gmo(NULL), gev(NULL)
{
	strcpy(ipopt_message, "COIN-OR Interior Point Optimizer (Ipopt Library 3.8)\nwritten by A. Waechter\n");
}

GamsIpopt::~GamsIpopt() { }

int GamsIpopt::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	char buffer[1024];
	
	gmo = gmo_;
	assert(gmo);
	assert(IsNull(ipopt));

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1;

	gev = (gevRec*)gmoEnvironment(gmo);
	
#ifdef GAMS_BUILD
#include "coinlibdCL3svn.h" 
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

 	ipopt = new IpoptApplication(false);

 	SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY);
	jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!ipopt->Jnlst()->AddJournal(jrnl))
		gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");

  ipopt->Options()->SetNumericValue("bound_relax_factor", 1e-10, true, true);
	ipopt->Options()->SetIntegerValue("max_iter", gevGetIntOpt(gev, gevIterLim), true, true);
	ipopt->Options()->SetNumericValue("max_cpu_time", gevGetDblOpt(gev, gevResLim), true, true);
  ipopt->Options()->SetStringValue("mu_strategy", "adaptive", true, true);
// 	ipopt->Options()->SetNumericValue("nlp_lower_bound_inf", gmoMinf(gmo), false, true);
// 	ipopt->Options()->SetNumericValue("nlp_upper_bound_inf", gmoPinf(gmo), false, true);
 	// if we have linear rows and a quadratic objective, then the hessian of the Lag.func. is constant, and Ipopt can make use of this
 	if (gmoNLM(gmo) == 0 && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp))
 		ipopt->Options()->SetStringValue("hessian_constant", "yes", true, true);
 	if (gmoSense(gmo) == Obj_Max)
 		ipopt->Options()->SetNumericValue("obj_scaling_factor", -1., true, true);

	ipopt->RegOptions()->SetRegisteringCategory("Linear Solver");
#ifdef HAVE_HSL_LOADER
	// add option to specify path to hsl library
	ipopt->RegOptions()->AddStringOption1("hsl_library", // name
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
	ipopt->RegOptions()->AddStringOption1("pardiso_library", // name
			"path and filename of PARDISO library for dynamic load",  // short description
			"", // default value
			"*", // setting1
			"path (incl. filename) of Pardiso library", // description1
			"Specify the path to a PARDISO library that and can be load via dynamic linking. "
			"Note, that you still need to specify to pardiso as linear_solver."
	);
#endif

	if (gmoOptFile(gmo)) {
	 	ipopt->Options()->SetStringValue("print_user_options", "yes", true, true);
		gmoNameOptFile(gmo, buffer);
		ipopt->Initialize(buffer);
	} else
		ipopt->Initialize("");

	double ipoptinf;
	ipopt->Options()->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
	gmoMinfSet(gmo, ipoptinf);
	ipopt->Options()->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
	gmoPinfSet(gmo, ipoptinf);

	SmartPtr<GamsNLP> nlp_ = new GamsNLP(gmo);
	nlp = GetRawPtr(nlp_);

	ipopt->Options()->GetNumericValue("diverging_iterates_tol", nlp_->div_iter_tol, "");
	// or should we also check the tolerance for acceptable points?
	ipopt->Options()->GetNumericValue("tol", nlp_->scaled_conviol_tol, "");
	ipopt->Options()->GetNumericValue("constr_viol_tol", nlp_->unscaled_conviol_tol, "");

	std::string hess_approx;
	ipopt->Options()->GetStringValue("hessian_approximation", hess_approx, "");
	if (hess_approx == "exact") {
		int do2dir = 1;
		int dohess = 1;
		gmoHessLoad(gmo, 0, -1, &do2dir, &dohess); // TODO make "-1" a parameter (like rvhess in CONOPT)
		if (!dohess) {
			gevLogStat(gev, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
			ipopt->Options()->SetStringValue("hessian_approximation", "limited-memory");
	  }
	}

	std::string libpath;
#ifdef HAVE_HSL_LOADER
	if (ipopt->Options()->GetStringValue("hsl_library", libpath, "")) {
		if (LSL_loadHSL(libpath.c_str(), buffer, sizeof(buffer)) != 0) {
			gevLogStatPChar(gev, "Failed to load HSL library at user specified path: ");
			gevLogStat(gev, buffer);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			return -1;
		}
	}
#endif
#ifndef HAVE_PARDISO
	if (ipopt->Options()->GetStringValue("pardiso_library", libpath, "")) {
		if (LSL_loadPardisoLib(libpath.c_str(), buffer, sizeof(buffer)) != 0) {
			gevLogStatPChar(gev, "Failed to load Pardiso library at user specified path: ");
			gevLogStat(gev, buffer);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		  return -1;
		}
	}
#endif

	return 0;
}

int GamsIpopt::callSolver() {
	assert(IsValid(ipopt));
	assert(IsValid(nlp));

	((GamsNLP*)GetRawPtr(nlp))->clockStart = gevTimeDiffStart(gev);
	ApplicationReturnStatus status;
	try {
		status = ipopt->OptimizeTNLP(nlp); // TODO reoptimize if problem is just modified
	} catch (IpoptException e) {
		status = Unrecoverable_Exception;
		gevLogStat(gev, e.Message().c_str());
	}

	switch (status) {
		case Solve_Succeeded:
		case Solved_To_Acceptable_Level:
		case Infeasible_Problem_Detected:
		case Search_Direction_Becomes_Too_Small:
		case Diverging_Iterates:
		case User_Requested_Stop:
		case Maximum_Iterations_Exceeded:
		case Maximum_CpuTime_Exceeded:
		case Restoration_Failed:
		case Error_In_Step_Computation:
			break; // these should have been handled by FinalizeSolution already

		case Not_Enough_Degrees_Of_Freedom:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SolverErr);
			break;
		case Invalid_Problem_Definition:
		case Invalid_Option:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SetupErr);
			break;
		case Invalid_Number_Detected:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_EvalError);
			break;
		case Unrecoverable_Exception:
		case Internal_Error:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_InternalErr);
			break;
		case Insufficient_Memory:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SolverErr);
			break;
		case NonIpopt_Exception_Thrown:
		default:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			break;
	}

#ifdef HAVE_HSL_LOADER
  if (LSL_isHSLLoaded())
  	if (LSL_unloadHSL() != 0)
  		gevLogStat(gev, "Failed to unload HSL library.");
#endif
#ifndef HAVE_PARDISO
  if (LSL_isPardisoLoaded())
  	if (LSL_unloadPardisoLib()!=0)
  		gevLogStat(gev, "Failed to unload Pardiso library.");
#endif

	return 0;
}

DllExport GamsIpopt* STDCALL createNewGamsIpopt() {
	return new GamsIpopt();
}

DllExport int STDCALL ipoCallSolver(ipoRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsIpopt*)Cptr)->callSolver();
}

DllExport int STDCALL ipoModifyProblem(ipoRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsIpopt*)Cptr)->modifyProblem();
}

DllExport int STDCALL ipoHaveModifyProblem(ipoRec_t *Cptr) {
	assert(Cptr != NULL);
	return ((GamsIpopt*)Cptr)->haveModifyProblem();
}

DllExport int STDCALL ipoReadyAPI(ipoRec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr) {
	assert(Cptr != NULL);
	assert(Gptr != NULL);
	char msg[256];
	if (!gmoGetReady(msg, sizeof(msg)))
		return 1;
	if (!gevGetReady(msg, sizeof(msg)))
		return 1;
	return ((GamsIpopt*)Cptr)->readyAPI(Gptr, Optr);
}

DllExport void STDCALL ipoFree(ipoRec_t **Cptr) {
	assert(Cptr != NULL);
	delete (GamsIpopt*)*Cptr;
	*Cptr = NULL;
}

DllExport void STDCALL ipoCreate(ipoRec_t **Cptr, char *msgBuf, int msgBufLen) {
	assert(Cptr != NULL);
	*Cptr = (ipoRec_t*) new GamsIpopt();
	if (msgBufLen && msgBuf)
		msgBuf[0] = 0;
}
