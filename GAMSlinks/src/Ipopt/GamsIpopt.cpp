// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Steve Dirkse, Stefan Vigerske

#include "GAMSlinksConfig.h"

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

#include "IpIpoptApplication.hpp"
#include "SmagNLP.hpp"
#include "SmagJournal.hpp"

extern "C" {
#ifdef COIN_HAS_LSLHSL
#include "HSLLoader.h"
#endif
#ifdef COIN_HAS_LSLPARDISO
#include "PardisoLoader.h"
#endif
}

using namespace Ipopt;

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
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/CoinIpopt NLP Solver (IPOPT Library 3.3, using MUMPS Library 4.7.3)\nwritten by A. Waechter\n");
#else
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/Ipopt NLP Solver (IPOPT Library 3.3)\nwritten by A. Waechter\n");
#endif
	smagStdOutputFlush(prob, SMAG_ALLMASK);

  // Create a new instance of your nlp
  SMAG_NLP* mysmagnlp = new SMAG_NLP (prob);
  SmartPtr<TNLP> smagnlp = mysmagnlp;
  // Create a new instance of IpoptApplication (use a SmartPtr, not raw)
  SmartPtr<IpoptApplication> app = new IpoptApplication(false);

 	SmartPtr<Journal> smag_jrnl=new SmagJournal(prob, "console", J_ITERSUMMARY);
	smag_jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!app->Jnlst()->AddJournal(smag_jrnl))
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT output.\n");

	// Change some options
  app->Options()->SetNumericValue("bound_relax_factor", 0);
	app->Options()->SetIntegerValue("max_iter", prob->gms.itnlim);
  app->Options()->SetStringValue("mu_strategy", "adaptive");
 	app->Options()->SetNumericValue("nlp_lower_bound_inf", -prob->inf, false);
 	app->Options()->SetNumericValue("nlp_upper_bound_inf",  prob->inf, false);
// 	if (smagColCountNL(prob)==0) { // LP
//		app->Options()->SetStringValue("mehrotra_algorithm", "yes");
// 	}
// if we have linear rows and a quadratic objective, then the hessian of the lag. is constant, and Ipopt can make use of this  
	if ((prob->modType==procQCP || prob->modType==procRMIQCP) && prob->rowCountNL==0)
		app->Options()->SetStringValue("hessian_constant", "yes"); 

#if defined(COIN_HAS_LSLHSL) || defined(COIN_HAS_LSLPARDISO)
  // if Ipopt was linked to the LSL, then the default linear solver in Ipopt might require a dynamic library load  
	// we want to avoid that this is the default behaviour
	// thus, we set the default for the linear solver to MUMPS, by what we assume that Ipopt was linked against Mumps
  // the default for the linear_system_scaling we leave at MC19, by what we assume that its availablity is implied by the availability of MA27 (of course user can change)
	app->Options()->SetStringValue("linear_solver", "mumps");
	//	app->Options()->SetStringValue("linear_system_scaling", "none");
#endif
#if defined(COIN_HAS_LSLHSL)
	// add option to specify path to hsl library; currently only working for lowercase paths
	app->RegOptions()->AddStringOption1("hsl_library", // name
			"path and filename of HSL library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"", // description1
			"Specify the path to a library that contains HSL routines and can be load via dynamic linking."
			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options (linear_solver, ...)."
	);
#endif
#if defined(COIN_HAS_LSLPARDISO)
	// add option to specify path to pardiso library; currently only working for lowercase paths
	app->RegOptions()->AddStringOption1("pardiso_library", // name
			"path and filename of Pardiso library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"", // description1
			"Specify the path to a Pardiso library that and can be load via dynamic linking."
			"Note, that you still need to specify to pardiso as linear_solver."
	);
#endif
	
	if (prob->gms.useopt)
		app->Initialize(prob->gms.optFileName);
	else
		app->Initialize("");

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

#ifdef COIN_HAS_LSLHSL
	std::string hsllib;
	app->Options()->GetStringValue("hsl_library", hsllib, "");
	if (hsllib!="") {
		if (LSL_loadHSL(hsllib.c_str(), buffer, 512)!=0) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to load HSL library at user specified path: ");
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
			smagReportSolBrief(prob, 13, 13);
		  return EXIT_SUCCESS;
		}
	}
#endif
#ifdef COIN_HAS_LSLPARDISO
	std::string pardisolib;
	app->Options()->GetStringValue("pardiso_library", pardisolib, "");
	if (pardisolib!="") {
		if (LSL_loadPardisoLib(pardisolib.c_str(), buffer, 512)!=0) {
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to load Pardiso library at user specified path: ");
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputPrint(prob, SMAG_ALLMASK, "\n");
			smagReportSolBrief(prob, 13, 13);
		  return EXIT_SUCCESS;
		}
	}
#endif
	
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

#ifdef COIN_HAS_LSLHSL
  if (LSL_isHSLLoaded())
  	if (LSL_unloadHSL()!=0)
  		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to unload HSL library.\n");
#endif
#ifdef COIN_HAS_LSLPARDISO
  if (LSL_isPardisoLoaded())
  	if (LSL_unloadPardisoLib()!=0)
  		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to unload Pardiso library.\n");
#endif
	
	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);

  return EXIT_SUCCESS;
} // main


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
