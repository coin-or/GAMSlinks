// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsIpopt.cpp 65 2007-02-06 17:29:45Z stefan $
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

#include "IpoptInterface.hpp"
#include "SmagMINLP.hpp"
#include "SmagMessageHandler.hpp"
#include "CbcBonmin.hpp"


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
  if (smagStdOutputStart(prob, SMAG_STATUS_OVERWRITE_IFDUMMY, buffer, sizeof(buffer)!=0))
  	fprintf(stderr, "Warning: Error opening GAMS output files .. continuing anyhow\t%s\n", buffer);
  
  smagReadModelStats (prob);
  smagSetObjFlavor (prob, OBJ_FUNCTION);
  smagSetSqueezeFreeRows (prob, 1);	/* don't show me =n= rows */
  smagReadModel (prob);
  smagHessInit (prob);

#ifdef GAMS_BUILD
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/CoinBonmin MINLP Solver (Bonmin Library 0.1.1)\nwritten by ...\n");
#else
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/Bonmin MINLP Solver (Bonmin Library 0.1.1)\nwritten by ...\n");
#endif
	smagStdOutputFlush(prob, SMAG_ALLMASK);

  // Create a new instance of your minlp
  SMAG_MINLP* mysmagminlp = new SMAG_MINLP(prob);
  SmartPtr<TMINLP> smagminlp = mysmagminlp;

  IpoptInterface nlpSolver(smagminlp);
  
  
//	SmartPtr<Journal> smag_listjrnl=new SmagJournal(prob, SMAG_LISTMASK, "SMAGlisting", J_SUMMARY);
//	smag_listjrnl->SetPrintLevel(J_DBG, J_NONE);  	
//	if (!app->Jnlst()->AddJournal(smag_listjrnl))
//		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT listing output.\n");  
//
//  if (prob->logOption) {
//  	// calling this journal "console" lets IPOPT adjust its print_level according to the print_level parameter (if set) 
//  	SmartPtr<Journal> smag_logjrnl=new SmagJournal(prob, SMAG_LOGMASK, "console", J_ITERSUMMARY);
//		smag_logjrnl->SetPrintLevel(J_DBG, J_NONE);  	
//		if (!app->Jnlst()->AddJournal(smag_logjrnl))
//			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT logging output.\n");
//  }

//TODO message control not working yet
//	SmagMessageHandler msghandler(prob);
//	nlpSolver.passInMessageHandler(&msghandler);

	// Change some options
	nlpSolver.retrieve_options()->SetNumericValue("bound_relax_factor", 0);
	nlpSolver.retrieve_options()->SetNumericValue("nlp_lower_bound_inf", -prob->inf, false);
	nlpSolver.retrieve_options()->SetNumericValue("nlp_upper_bound_inf",  prob->inf, false);

//TODO	app->Options()->SetIntegerValue("max_iter", prob->gms.itnlim);
//TODO and more options like cutoff and optcr, optca...

	if (prob->gms.useopt)
		nlpSolver.readOptionFile(prob->gms.optFileName);

	nlpSolver.retrieve_options()->GetNumericValue("diverging_iterates_tol", mysmagminlp->div_iter_tol, "");
	// or should we also check the tolerance for acceptable points?  
	nlpSolver.retrieve_options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, ""); 
	nlpSolver.retrieve_options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, ""); 


  try {
		BonminCbcParam par;
		BonminBB bb;
		par(nlpSolver);

		bb(nlpSolver, par);//process parameter file using Ipopt and do branch and bound

//    std::string message;
    int model_status;
    int solver_status;
    switch (bb.mipStatus()) {
    	case BonminBB::FeasibleOptimal: {
	    	model_status=2; // local optimal; we could report optimal if the gap is closed
	    	solver_status=1; // normal completion
//	      message = "\nbonmin: Optimal";
	    } break;
    	case BonminBB::ProvenInfeasible: {
	    	model_status=4; // infeasible
	    	solver_status=1; // normal completion
//	      message = "\nbonmin: Infeasible problem";
	    } break;
    	case BonminBB::Feasible: {
	    	model_status=7; // intermediate nonoptimal
	    	solver_status=1; // normal completion
//  	    message = "\n Optimization not finished.";
    	} break;
    	case BonminBB::NoSolutionKnown: {
	    	model_status=6; // intermediate infeasible
	    	solver_status=1; // normal completion
//  	    message = "\n Optimization not finished.";
    	} break;
    	default : { // should not happen, since other mipStatus are not defined
	    	model_status=12; // error unknown
	    	solver_status=1; // normal completion
    	}
		}
		
//    std::cout
//    <<bb.bestObj()<<"\t"
//    <<bb.numNodes()<<"\t"
//    <<bb.iterationCount()<<"\t"
//    <<std::endl;

		double resUsed=0.;
		int domErrors=0;
		const double* rowMarg=nlpSolver.getRowPrice();
		//new double[smagRowCount(prob)];

	  smagReportSolX (prob, bb.iterationCount(), resUsed, domErrors, rowMarg, bb.bestSolution());
  }
  catch(IpoptInterface::UnsolvedError &E) {
    //There has been a failure to solve a problem with Ipopt.
    std::cerr<<"Ipopt has failed to solve a problem"<<std::endl;
	  smagReportSolBrief(prob, 10, 13);
  }
  catch(IpoptInterface::SimpleError &E) {
    std::cerr<<E.className()<<"::"<<E.methodName()
	     <<std::endl
	     <<E.message()<<std::endl;
	  smagReportSolBrief(prob, 13, 13);
  }
  catch(CoinError &E) {
    std::cerr<<E.className()<<"::"<<E.methodName()
	     <<std::endl
	     <<E.message()<<std::endl;
	  smagReportSolBrief(prob, 13, 13);
  }

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
