// Copyright (C) 2007-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GAMSlinksConfig.h"
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

#include "SmagMINLP.hpp"
#include "SmagJournal.hpp"
#include "GamsHandlerSmag.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsDictionary.hpp"
#include "GamsBCH.hpp"
#include "GamsHeuristic.hpp"
#include "GamsCutGenerator.hpp"
#include "BonBonminSetup.hpp"
#include "BonCbc.hpp"

// in case that we have to solve an NLP only
#include "IpIpoptApplication.hpp"
#include "SmagNLP.hpp"

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

using namespace Ipopt;

void solve_minlp(smagHandle_t);
void solve_nlp(smagHandle_t);
void write_solution(smagHandle_t prob, OsiTMINLPInterface& osi_tminlp, int model_status, int solver_status, double resuse, int domviol, int iterations);
void write_solution_nodual(smagHandle_t prob, OsiTMINLPInterface& osi_tminlp, int model_status, int solver_status, double resuse, int domviol, int iterations);
void BCHsetupOptions(Bonmin::RegisteredOptions& regopt);
void BCHinit(BonminSetup& bonmin_setup, GamsHandlerSmag& gamshandler, GamsDictionary*& dict, GamsBCH*& bch, CbcModel*& modelptr);

int main (int argc, char* argv[]) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif
  smagHandle_t prob;

  if (argc < 2) {
  	fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n", argv[0]);
    return EXIT_FAILURE;
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

  smagReadModelStats (prob);
  smagSetObjFlavor (prob, OBJ_FUNCTION);
  smagSetSqueezeFreeRows (prob, 1);	/* don't show me =n= rows */
  smagReadModel (prob);

#ifdef GAMS_BUILD
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/CoinBonmin MINLP Solver (Bonmin Library 0.99, using MUMPS Library 4.7.3)\nwritten by P. Bonami\n");
#else
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/Bonmin MINLP Solver (Bonmin Library 0.99)\nwritten by P. Bonami\n");
#endif
	smagStdOutputFlush(prob, SMAG_ALLMASK);

	// call Bonmin if there are any discrete variables; call Ipopt otherwise
	if (prob->gms.ndisc || prob->gms.nsos1 || prob->gms.nsos2 || prob->gms.nsemi || prob->gms.nsemii)
		solve_minlp(prob);
	else
		solve_nlp(prob);

#ifdef HAVE_HSL_LOADER
  if (LSL_isHSLLoaded())
  	if (LSL_unloadHSL()!=0)
  		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to unload HSL library.\n");
#endif
#ifndef HAVE_PARDISO
  if (LSL_isPardisoLoaded())
  	if (LSL_unloadPardisoLib()!=0)
  		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to unload Pardiso library.\n");
#endif
  
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);

  return EXIT_SUCCESS;
} // main

/** Solves a MINLP via Bonmin.
 */
void solve_minlp(smagHandle_t prob) {
	// the problem as TMINLP
  SMAG_MINLP* mysmagminlp = new SMAG_MINLP(prob);
  SmartPtr<TMINLP> smagminlp = mysmagminlp;

	BonminSetup bonmin_setup;

	// instead of initializeOptionsAndJournalist we do it our own way, so we can use the SmagJournal
  SmartPtr<OptionsList> options = new OptionsList();
	SmartPtr<Journalist> journalist= new Journalist();
  SmartPtr<Bonmin::RegisteredOptions> roptions = new Bonmin::RegisteredOptions();
 	SmartPtr<Journal> smag_jrnl=new SmagJournal(prob, "console", J_ITERSUMMARY, J_STRONGWARNING);
	smag_jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!journalist->AddJournal(smag_jrnl))
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT output.\n");
	options->SetJournalist(journalist);
	options->SetRegisteredOptions(GetRawPtr(roptions));

	bonmin_setup.setOptionsAndJournalist(roptions, options, journalist);
  bonmin_setup.registerOptions();

#ifdef HAVE_HSL_LOADER
	// add option to specify path to hsl library
  bonmin_setup.roptions()->AddStringOption1("hsl_library", // name
			"path and filename of HSL library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"", // description1
			"Specify the path to a library that contains HSL routines and can be load via dynamic linking."
			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options (linear_solver, ...)."
	);
#endif
#ifndef HAVE_PARDISO
	// add option to specify path to pardiso library
  bonmin_setup.roptions()->AddStringOption1("pardiso_library", // name
			"path and filename of Pardiso library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"", // description1
			"Specify the path to a Pardiso library that and can be load via dynamic linking."
			"Note, that you still need to specify to pardiso as linear_solver."
	);
#endif
  
  // add options specific to BCH heuristic callback
  BCHsetupOptions(*bonmin_setup.roptions());
  
	// Change some options
	bonmin_setup.options()->SetNumericValue("bound_relax_factor", 0);
	bonmin_setup.options()->SetNumericValue("nlp_lower_bound_inf", -prob->inf, false);
	bonmin_setup.options()->SetNumericValue("nlp_upper_bound_inf",  prob->inf, false);
	if (prob->gms.icutof)
		bonmin_setup.options()->SetNumericValue("bonmin.cutoff", prob->gms.cutoff);
	bonmin_setup.options()->SetNumericValue("bonmin.allowable_gap", prob->gms.optca);
	bonmin_setup.options()->SetNumericValue("bonmin.allowable_fraction_gap", prob->gms.optcr);
	if (prob->gms.nodlim)
		bonmin_setup.options()->SetIntegerValue("bonmin.node_limit", prob->gms.nodlim);
	else
		bonmin_setup.options()->SetIntegerValue("bonmin.node_limit", prob->gms.itnlim);
	bonmin_setup.options()->SetNumericValue("bonmin.time_limit", prob->gms.reslim);

	if ((prob->modType==procQCP || prob->modType==procMIQCP || prob->modType==procRMIQCP) && prob->rowCountNL==0)
		bonmin_setup.options()->SetStringValue("hessian_constant", "yes"); 
	if (prob->gms.iscopt)
		bonmin_setup.options()->SetStringValue("nlp_scaling_method", "user-scaling");
	if (prob->gms.nosos1 || prob->gms.nosos2) { // strong branching not compatible with sos in 0.99
		smagStdOutputPrint(prob, SMAG_LOGMASK, "Found SOS constraints: Changing default variable selection rule to most-fractional.\n");
		bonmin_setup.options()->SetStringValue("bonmin.variable_selection", "most-fractional");
	}
	
	try {
		if (prob->gms.useopt)
			bonmin_setup.readOptionsFile(prob->gms.optFileName);
		else // need to let Bonmin read something, otherwise it will try to read its default option file bonmin.opt
			bonmin_setup.readOptionsString(std::string());
	} catch (IpoptException error) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, error.Message().c_str());
	  smagReportSolBrief(prob, 13, 13);
	  return;
	} catch (std::bad_alloc) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Not enough memory\n");
		smagReportSolBrief(prob, 13, 13);
		return;
	} catch (...) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Unknown exception thrown.\n");
		smagReportSolBrief(prob, 13, 13);
		return;
	}

	std::string libpath;
#ifdef HAVE_HSL_LOADER
	if (bonmin_setup.options()->GetStringValue("hsl_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadHSL(libpath.c_str(), buffer, 512)!=0) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to load HSL library at user specified path: ");
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
			smagReportSolBrief(prob, 13, 13);
		  return;
		}
	}
#endif
#ifndef HAVE_PARDISO
	if (bonmin_setup.options()->GetStringValue("pardiso_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512)!=0) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to load Pardiso library at user specified path: ");
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
			smagReportSolBrief(prob, 13, 13);
		  return;
		}
	}
#endif

	bonmin_setup.options()->GetNumericValue("diverging_iterates_tol", mysmagminlp->div_iter_tol, "");
//	// or should we also check the tolerance for acceptable points?
//	bonmin_setup.options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, "");
//	bonmin_setup.options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, "");

	bool hessian_is_approx=false;
	std::string parvalue;
	bonmin_setup.options()->GetStringValue("hessian_approximation", parvalue, "");
	if (parvalue=="exact") {
	  if (smagHessInit(prob)) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to initialize Hessian structure. We continue and tell Bonmin to work with a limited-memory Hessian approximation!\n");
			bonmin_setup.options()->SetStringValue("hessian_approximation", "limited-memory");
	  	hessian_is_approx=true;
	  }
	} else
		hessian_is_approx=true;
	
	if (hessian_is_approx) { // check whether QP strong branching is enabled
		bonmin_setup.options()->GetStringValue("varselect_stra", parvalue, "bonmin.");
		if (parvalue=="qp-strong-branching") {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: QP strong branching does not work when the Hessian is approximated. Aborting...\n");
			smagReportSolBrief(prob, 13, 13);
			return;
		}
	}
	
	GamsHandlerSmag gamshandler(prob);
	GamsMessageHandler messagehandler(gamshandler);
	GamsDictionary* dict = NULL;
	GamsBCH* bch = NULL;
	CbcModel* modelptr = NULL;
	
	BCHinit(bonmin_setup, gamshandler, dict, bch, modelptr);
	if (bch)
		bch->setGlobalBounds(prob->colLB, prob->colUB);

	// the easiest would be to call bonmin_setup.initializeBonmin(smagminlp), but then we cannot set the message handler
	// so we do the following
	try {
		{
			OsiTMINLPInterface first_osi_tminlp;
			first_osi_tminlp.initialize(roptions, options, journalist, smagminlp);
			first_osi_tminlp.passInMessageHandler(&messagehandler);
			bonmin_setup.initialize(first_osi_tminlp); // this will clone first_osi_tminlp
		}

		Bab bb;
		modelptr = &bb.model();

		mysmagminlp->clock_start=smagGetCPUTime(prob);
		bb(bonmin_setup); //process parameters and do branch and bound
		
		double best_bound=smagMinim(prob)*bb.bestBound();
		if (best_bound>-1e200 && best_bound<1e200)
			smagSetObjEst(prob, best_bound);
		smagSetNodUsd(prob, bb.numNodes());

		OsiTMINLPInterface osi_tminlp(*bonmin_setup.nonlinearSolver());
		if (bb.bestSolution()) {
			char buf[100];
			snprintf(buf, 100, "\nBonmin finished. Found feasible point. Objective function = %f.\n", bb.bestObj());
			smagStdOutputPrint(prob, SMAG_ALLMASK, buf);

			bool has_free_var=false;
			for (Index i=0; i<smagColCount(prob); ++i)
				if (prob->colType[i]!=SMAG_VAR_CONT)
					osi_tminlp.setColBounds(i, bb.bestSolution()[i], bb.bestSolution()[i]);
				else if ((!has_free_var) && osi_tminlp.getColUpper()[i]-osi_tminlp.getColLower()[i]>1e-5)
					has_free_var=true;
			osi_tminlp.setColSolution(bb.bestSolution());

			if (!has_free_var) {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "All variables are discrete. Dual variables for fixed problem will be not available.\n");
				osi_tminlp.initialSolve(); // this will only evaluate the constraints, so we get correct row levels
				write_solution_nodual(prob, osi_tminlp, mysmagminlp->model_status, mysmagminlp->solver_status, smagGetCPUTime(prob)-mysmagminlp->clock_start, mysmagminlp->domviolations, bb.iterationCount());
			} else {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Resolve with fixed discrete variables to get dual values.\n");
				bool error_in_fixedsolve=false;
				try {
					osi_tminlp.initialSolve();
					error_in_fixedsolve=!osi_tminlp.isProvenOptimal();
				} catch (TNLPSolver::UnsolvedError *E) { // there has been a failure to solve a problem with Ipopt
					char buf[1024];
					snprintf(buf, 1024, "Error: %s exited with error %s\n", E->solverName().c_str(), E->errorName().c_str());
					smagStdOutputPrint(prob, SMAG_ALLMASK, buf);
					error_in_fixedsolve=true;
				}
				if (!error_in_fixedsolve) {
					write_solution(prob, osi_tminlp, mysmagminlp->model_status, mysmagminlp->solver_status, smagGetCPUTime(prob)-mysmagminlp->clock_start, mysmagminlp->domviolations, bb.iterationCount());
				} else {
					smagStdOutputPrint(prob, SMAG_ALLMASK, "Problems solving fixed problem. Dual variables for NLP subproblem not available.\n");
					write_solution_nodual(prob, osi_tminlp, mysmagminlp->model_status, mysmagminlp->solver_status, smagGetCPUTime(prob)-mysmagminlp->clock_start, mysmagminlp->domviolations, bb.iterationCount());
				}
			}
		} else {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\nBonmin finished. No feasible point found.\n");
			smagReportSolBrief(prob, mysmagminlp->model_status, mysmagminlp->solver_status);
		}

		smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
		char buf[1024];
		if (bb.bestSolution()) {
			snprintf(buf, 1024, "MINLP solution: %20.10g   (%d nodes, %g seconds)\n", smagMinim(prob)*osi_tminlp.getObjValue(), bb.numNodes(), smagGetCPUTime(prob)-mysmagminlp->clock_start);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buf);
		}
		if (best_bound>-1e200 && best_bound<1e200) {
			snprintf(buf, 1024, "Best possible: %21.10g\n", best_bound);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buf);

			if (bb.bestSolution()) {
				snprintf(buf, 1024, "Absolute gap: %22.5g\nRelative gap: %22.5g\n", CoinAbs(smagMinim(prob)*osi_tminlp.getObjValue()-best_bound), CoinAbs(smagMinim(prob)*osi_tminlp.getObjValue()-best_bound)/CoinMax(CoinAbs(best_bound), 1.));
				smagStdOutputPrint(prob, SMAG_ALLMASK, buf);
			}
		}
  } catch(CoinError &error) {
  	char buf[1024];
  	snprintf(buf, 1024, "%s::%s\n%s\n", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
		smagStdOutputPrint(prob, SMAG_ALLMASK, buf);
	  smagReportSolBrief(prob, 13, 13);
  } catch (TNLPSolver::UnsolvedError *E) { 	// there has been a failure to solve a problem with Ipopt.
		char buf[1024];
		snprintf(buf, 1024, "Error: %s exited with error %s\n", E->solverName().c_str(), E->errorName().c_str());
		smagStdOutputPrint(prob, SMAG_ALLMASK, buf);
		smagReportSolBrief(prob, 13, 13);
	} catch (std::bad_alloc) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Not enough memory\n");
		smagReportSolBrief(prob, 13, 13);
	} catch (...) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Unknown exception thrown.\n");
		smagReportSolBrief(prob, 13, 13);
	}
	
	delete dict;
	delete bch;
} // solve_minlp()

/** Processes Ipopt solution and calls method to report the solution.
 */
void write_solution(smagHandle_t prob, OsiTMINLPInterface& osi_tminlp, int model_status, int solver_status, double resuse, int domviol, int iterations) {
	int n=smagColCount(prob);
	int m=smagRowCount(prob);
	int isMin=smagMinim(prob);

	const double* lambda=osi_tminlp.getRowPrice();
	const double* z_L=lambda+m;
	const double* z_U=z_L+n;
	unsigned char* colBasStat=new unsigned char[n];
	unsigned char* colIndic=new unsigned char[n];
	double* colMarg=new double[n];
	for (Index i=0; i<n; ++i) {
		colBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
		colIndic[i]=SMAG_RCINDIC_OK;
		// if, e.g., x_i has no lower bound, then the dual z_L[i] is -infinity
		colMarg[i]=0;
		if (z_L[i]>-prob->inf) colMarg[i]+=isMin*z_L[i];
		if (z_U[i]<prob->inf) colMarg[i]-=isMin*z_U[i];
	}

	unsigned char* rowBasStat=new unsigned char[m];
	unsigned char* rowIndic=new unsigned char[m];
	double* negLambda=new double[m];
  for (Index i = 0;  i < m;  i++) {
		rowBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
		rowIndic[i]=SMAG_RCINDIC_OK; // TODO: not ok, if over the bounds
		negLambda[i]=-lambda[i]*isMin;
  }
	smagReportSolFull(prob, model_status, solver_status,
		iterations, resuse, osi_tminlp.getObjValue()*isMin, domviol,
		osi_tminlp.getRowActivity(), negLambda, rowBasStat, rowIndic,
		osi_tminlp.getColSolution(), colMarg, colBasStat, colIndic);

	delete[] colBasStat;
	delete[] colIndic;
	delete[] colMarg;
	delete[] rowBasStat;
	delete[] rowIndic;
	delete[] negLambda;
} // write_solution

void write_solution_nodual(smagHandle_t prob, OsiTMINLPInterface& osi_tminlp, int model_status, int solver_status, double resuse, int domviol, int iterations) {
	int n=smagColCount(prob);
	int m=smagRowCount(prob);

	unsigned char* colBasStat=new unsigned char[n];
	unsigned char* colIndic=new unsigned char[n];
	for (Index i=0; i<n; ++i) {
		colBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
		colIndic[i]=SMAG_RCINDIC_OK;
	}

	unsigned char* rowBasStat=new unsigned char[m];
	unsigned char* rowIndic=new unsigned char[m];
  for (Index i = 0;  i < m;  i++) {
		rowBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
		rowIndic[i]=SMAG_RCINDIC_OK;
  }
	smagReportSolFull(prob, model_status, solver_status,
		iterations, resuse, osi_tminlp.getObjValue()*smagMinim(prob), domviol,
		osi_tminlp.getRowActivity(), prob->rowPi, rowBasStat, rowIndic,
		osi_tminlp.getColSolution(), prob->colRC, colBasStat, colIndic);

	delete[] colBasStat;
	delete[] colIndic;
	delete[] rowBasStat;
	delete[] rowIndic;
} // write_solution_nodual


/** Solves an NLP via Ipopt.
 */
void solve_nlp(smagHandle_t prob) {
	smagStdOutputPrint(prob, SMAG_LOGMASK, "The problem is continuous (NLP). Solving with IPOPT.\n");

  SMAG_NLP* mysmagnlp = new SMAG_NLP (prob);
  SmartPtr<TNLP> smagnlp = mysmagnlp;
  // Create a new instance of IpoptApplication (use a SmartPtr, not raw)
  SmartPtr<IpoptApplication> app = new IpoptApplication(false);

 	SmartPtr<Journal> smag_jrnl=new SmagJournal(prob, "console", J_ITERSUMMARY);
	smag_jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!app->Jnlst()->AddJournal(smag_jrnl))
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT output.\n");

	// register Bonmin options so that Ipopt does not stumble over bonmin options
  SmartPtr<Bonmin::RegisteredOptions> roptions = new Bonmin::RegisteredOptions();
	BonminSetup::registerAllOptions(roptions);
	app->Options()->SetRegisteredOptions(GetRawPtr(roptions));

//	OsiTMINLPInterface::registerOptions(app->RegOptions());

#ifdef HAVE_HSL_LOADER
	// add option to specify path to hsl library
	roptions->AddStringOption1("hsl_library", // name
			"path and filename of HSL library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"", // description1
			"Specify the path to a library that contains HSL routines and can be load via dynamic linking."
			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options (linear_solver, ...)."
	);
#endif
#ifndef HAVE_PARDISO
	// add option to specify path to pardiso library
  roptions->AddStringOption1("pardiso_library", // name
			"path and filename of Pardiso library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"", // description1
			"Specify the path to a Pardiso library that and can be load via dynamic linking."
			"Note, that you still need to specify to pardiso as linear_solver."
	);
#endif

	// Change some options
  app->Options()->SetNumericValue("bound_relax_factor", 0);
	app->Options()->SetIntegerValue("max_iter", prob->gms.itnlim);
  app->Options()->SetStringValue("mu_strategy", "adaptive");
 	app->Options()->SetNumericValue("nlp_lower_bound_inf", -prob->inf, false);
 	app->Options()->SetNumericValue("nlp_upper_bound_inf",  prob->inf, false);
	if ((prob->modType==procQCP || prob->modType==procMIQCP || prob->modType==procRMIQCP) && prob->rowCountNL==0)
		app->Options()->SetStringValue("hessian_constant", "yes"); 
	if (prob->gms.iscopt)
		app->Options()->SetStringValue("nlp_scaling_method", "user-scaling");

	if (prob->gms.useopt)
		app->Initialize(prob->gms.optFileName);
	else
		app->Initialize("");

	std::string libpath;
#ifdef HAVE_HSL_LOADER
	if (app->Options()->GetStringValue("hsl_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadHSL(libpath.c_str(), buffer, 512)!=0) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to load HSL library at user specified path: ");
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
			smagReportSolBrief(prob, 13, 13);
		  return;
		}
	}
#endif
#ifndef HAVE_PARDISO
	if (app->Options()->GetStringValue("pardiso_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512)!=0) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to load Pardiso library at user specified path: ");
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
			smagReportSolBrief(prob, 13, 13);
		  return;
		}
	}
#endif

	app->Options()->GetNumericValue("diverging_iterates_tol", mysmagnlp->div_iter_tol, "");
	// or should we also check the tolerance for acceptable points?
	app->Options()->GetNumericValue("tol", mysmagnlp->scaled_conviol_tol, "");
	app->Options()->GetNumericValue("constr_viol_tol", mysmagnlp->unscaled_conviol_tol, "");

	std::string hess_approx;
	app->Options()->GetStringValue("hessian_approximation", hess_approx, "");
	if (hess_approx=="exact") {
	  if (smagHessInit(prob)) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to initialize Hessian structure. We continue and tell Ipopt to work with a limited-memory Hessian approximation!\n");
			app->Options()->SetStringValue("hessian_approximation", "limited-memory");
	  }
	}

  // Ask Ipopt to solve the problem
  ApplicationReturnStatus status = app->OptimizeTNLP(smagnlp);

	switch (status) {
		case Solve_Succeeded:
		case Solved_To_Acceptable_Level:
		case Infeasible_Problem_Detected:
		case Search_Direction_Becomes_Too_Small:
		case Diverging_Iterates:
		case User_Requested_Stop:
		case Maximum_Iterations_Exceeded:
		case Restoration_Failed:
		case Error_In_Step_Computation:
			break; // these should have been catched by FinalizeSolution already

		case Not_Enough_Degrees_Of_Freedom:
			smagReportSolBrief(prob, 13, 10);
			break;
		case Invalid_Problem_Definition:
			smagReportSolBrief(prob, 13, 9);
			break;
		case Invalid_Option:
			smagReportSolBrief(prob, 13, 9);
			break;
		case Invalid_Number_Detected:
			smagReportSolBrief(prob, 13, 10);
			break;
		case Unrecoverable_Exception:
			smagReportSolBrief(prob, 13, 11);
			break;
		case NonIpopt_Exception_Thrown:
			smagReportSolBrief(prob, 13, 13);
			break;
		case Insufficient_Memory:
			smagReportSolBrief(prob, 13, 10);
			break;
		case Internal_Error:
			smagReportSolBrief(prob, 13, 11);
			break;
		default:
			smagReportSolBrief(prob, 13, 13);
			break;
	}
} // solve_nlp()

void BCHsetupOptions(Bonmin::RegisteredOptions& regopt) {
  regopt.AddStringOption1("usergdxin",
  		"The name of the GDX file read back into Bonmin.", "bchin.gdx",
  		"*", "name of GDX file", "");
  regopt.AddStringOption1("usergdxname",
  		"The name of the GDX file exported from the solver with the solution at the node.", "bchout.gdx",
  		"*", "name of GDX file", "");
  regopt.AddStringOption1("usergdxnameinc",
  		"The name of the GDX file exported from the solver with the incumbent solution.", "bchout_i.gdx",
  		"*", "name of GDX file", "");
  regopt.AddStringOption1("usergdxprefix",
  		"Prefixes usergdxin, usergdxname, and usergdxnameinc.", "",
  		"*", "the prefix", "");
  regopt.AddStringOption1("usercutcall",
  		"The GAMS command line to call the cut generator.", "",
  		"*", "GAMS command line", "");
  regopt.AddLowerBoundedIntegerOption("usercutfirst",
  		"Calls the cut generator for the first n nodes.",
  		0, 10, "");
  regopt.AddLowerBoundedIntegerOption("usercutfreq",
  		"Determines the frequency of the cut generator model calls.",
  		0, 10, "");
  regopt.AddLowerBoundedIntegerOption("usercutinterval",
  		"Determines the interval when to apply the multiplier for the frequency of the cut generator model calls.",
  		0, 100, "");
  regopt.AddLowerBoundedIntegerOption("usercutmult",
  		"Determines the multiplier for the frequency of the cut generator model calls.",
  		0, 2, "");
  regopt.AddBoundedIntegerOption("usercutnewint",
  		"Calls the cut generator if the solver found a new integer feasible solution.",
  		0, 1, 0, "");
  regopt.AddStringOption1("userheurcall",
  		"The GAMS command line to call the heuristic.", "",
  		"*", "GAMS command line", "");
  regopt.AddLowerBoundedIntegerOption("userheurfirst",
  		"Calls the heuristic for the first n nodes.",
  		0, 10, "");
  regopt.AddLowerBoundedIntegerOption("userheurfreq",
  		"Determines the frequency of the heuristic model calls.",
  		0, 10, "");
  regopt.AddLowerBoundedIntegerOption("userheurinterval",
  		"Determines the interval when to apply the multiplier for the frequency of the heuristic model calls.",
  		0, 100, "");
  regopt.AddLowerBoundedIntegerOption("userheurmult",
  		"Determines the multiplier for the frequency of the heuristic model calls.",
  		0, 2, "");
  regopt.AddBoundedIntegerOption("userheurnewint",
  		"Calls the heuristic if the solver found a new integer feasible solution.",
  		0, 1, 0, "");
  regopt.AddLowerBoundedIntegerOption("userheurobjfirst",
  		"Calls the heuristic if the LP value of the node is closer to the best bound than the current incumbent.",
  		0, 0, "");
  regopt.AddBoundedIntegerOption("userkeep",
  		"Calls gamskeep instead of gams.",
  		0, 1, 0, "");
  regopt.AddStringOption1("userjobid",
  		"Postfixes gdxname, gdxnameinc, and gdxin.", "",
  		"*", "the postfix", "");	
} // BCHsetupOptions

void BCHinit(BonminSetup& bonmin_setup, GamsHandlerSmag& gamshandler, GamsDictionary*& dict, GamsBCH*& bch, CbcModel*& modelptr) {
	std::string s;
	int i;
	bool have_cutcall, have_heurcall;

	bonmin_setup.options()->GetStringValue("usercutcall", s, "");
	have_cutcall = !s.empty();
	bonmin_setup.options()->GetStringValue("userheurcall", s, "");
	have_heurcall = !s.empty();
	
	if (!have_cutcall && !have_heurcall) return;

	dict = new GamsDictionary(gamshandler);
	dict->readDictionary();
	if (!dict->haveNames()) {
		gamshandler.print(GamsHandler::AllMask, "Error reading dictionary file. Cannot use BCH without dictionary.\n");
		return;
	}
	bch = new GamsBCH(gamshandler, *dict);

	std::string gdxprefix;
	bonmin_setup.options()->GetStringValue("usergdxprefix", gdxprefix, "");

	bonmin_setup.options()->GetStringValue("userjobid", s, "");
	bch->set_userjobid(s.c_str());
	
	bonmin_setup.options()->GetStringValue("usergdxname", s, "");
	bch->set_usergdxname(s.c_str(), gdxprefix.c_str());
	bonmin_setup.options()->GetStringValue("usergdxnameinc", s, "");
	bch->set_usergdxnameinc(s.c_str(), gdxprefix.c_str());
	bonmin_setup.options()->GetStringValue("usergdxin", s, "");
	bch->set_usergdxin(s.c_str(), gdxprefix.c_str());
	bonmin_setup.options()->GetIntegerValue("userkeep", i, "");
	bch->set_userkeep(i);

	bool cutnewint = false;
	if (have_cutcall) {
		bonmin_setup.options()->GetStringValue("usercutcall", s, "");
		bch->set_usercutcall(s.c_str());
		bonmin_setup.options()->GetIntegerValue("usercutfreq", i, "");
		bch->set_usercutfreq(i);
		bonmin_setup.options()->GetIntegerValue("usercutinterval", i, "");
		bch->set_usercutinterval(i);
		bonmin_setup.options()->GetIntegerValue("usercutmult", i, "");
		bch->set_usercutmult(i);
		bonmin_setup.options()->GetIntegerValue("usercutfirst", i, "");
		bch->set_usercutfirst(i);
		bonmin_setup.options()->GetIntegerValue("usercutnewint", i, "");
		bch->set_usercutnewint(i);
		cutnewint = i;
	}
	
	if (have_heurcall) {
		bonmin_setup.options()->GetStringValue("userheurcall", s, "");
		bch->set_userheurcall(s.c_str());
		bonmin_setup.options()->GetIntegerValue("userheurfreq", i, "");
		bch->set_userheurfreq(i);
		bonmin_setup.options()->GetIntegerValue("userheurinterval", i, "");
		bch->set_userheurinterval(i);
		bonmin_setup.options()->GetIntegerValue("userheurmult", i, "");
		bch->set_userheurmult(i);
		bonmin_setup.options()->GetIntegerValue("userheurfirst", i, "");
		bch->set_userheurfirst(i);
		bonmin_setup.options()->GetIntegerValue("userheurnewint", i, "");
		bch->set_userheurnewint(i);
		bonmin_setup.options()->GetIntegerValue("userheurobjfirst", i, "");
		bch->set_userheurobjfirst(i);
	}

	if (have_heurcall)
		bonmin_setup.heuristics().push_back(new GamsHeuristic(*bch)); // BabSetupBase takes care of deleting this one
	
	if (have_cutcall) {
		bonmin_setup.cutGenerators().push_back(BabSetupBase::CuttingMethod());
		BabSetupBase::CuttingMethod& cutmeth(bonmin_setup.cutGenerators().back());
		cutmeth.frequency = 1;
		cutmeth.id = "GamsBCH";
		cutmeth.cgl = new GamsCutGenerator(*bch, modelptr);
		cutmeth.atSolution = cutnewint;
		
		bonmin_setup.options()->GetStringValue("bonmin.algorithm", s, "");
		if (s=="B-BB") {
			gamshandler.print(GamsHandler::AllMask, "Cut generators not supported by Branch & Bound algorithm (B-BB). Changing to Hybrid algorithm (B-Hyb).\n");
			bonmin_setup.options()->SetStringValue("bonmin.algorithm", "B-Hyb");
		}
	}
}

// enum ApplicationReturnStatus
//   {
//     Solve_Succeeded=0,
//     Solved_To_Acceptable_Level=1,
//     Infeasible_Problem_Detected=2,
//     Search_Direction_Becomes_Too_Small=3,
//     Diverging_Iterates=4,
//     User_Requested_Stop=5,
//
//     Maximum_Iterations_Exceeded=-1,
//     Restoration_Failed=-2,
//     Error_In_Step_Computation=-3,
//     Not_Enough_Degrees_Of_Freedom=-10,
//     Invalid_Problem_Definition=-11,
//     Invalid_Option=-12,
//     Invalid_Number_Detected=-13,
//
//     Unrecoverable_Exception=-100,
//     NonIpopt_Exception_Thrown=-101,
//     Insufficient_Memory=-102,
//     Internal_Error=-199
//   };

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
