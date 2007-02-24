// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

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

// in case that we have to solve an NLP only
#include "IpIpoptApplication.hpp"
#include "SmagNLP.hpp"
#include "SmagJournal.hpp"

using namespace Ipopt;

void solve_minlp(smagHandle_t);
void solve_nlp(smagHandle_t);
void write_solution(smagHandle_t prob, IpoptInterface& nlpSolver, int model_status, int solver_status, double resuse, int domviol, int nodeuse);
void write_solution_nodual(smagHandle_t prob, IpoptInterface& nlpSolver, int model_status, int solver_status, double resuse, int domviol, int nodeuse);

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
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/CoinBonmin MINLP Solver (Bonmin Library 0.1.1)\nwritten by P. Bonami\n");
#else
	smagStdOutputPrint(prob, SMAG_ALLMASK, "\nGAMS/Bonmin MINLP Solver (Bonmin Library 0.1.1)\nwritten by P. Bonami\n");
#endif
	smagStdOutputFlush(prob, SMAG_ALLMASK);

	// call Bonmin if there are any discrete variables; call Ipopt otherwise
	if (prob->gms.ndisc)
		solve_minlp(prob);
	else
		solve_nlp(prob);

	smagStdOutputStop(prob, buffer, sizeof(buffer));
	smagClose(prob);

  return EXIT_SUCCESS;
} // main

/** Solves a MINLP via Bonmin.
 */
void solve_minlp(smagHandle_t prob) {
  SMAG_MINLP* mysmagminlp = new SMAG_MINLP(prob);
  SmartPtr<TMINLP> smagminlp = mysmagminlp;
  
//  SmagMessageHandler messagehandler(prob);

//  IpoptInterface nlpSolver(smagminlp, &messagehandler);
  IpoptInterface nlpSolver(smagminlp);
	BonminCbcParam par;
	par.fout=prob->fpLog;

	// Change some options
	nlpSolver.retrieve_options()->SetNumericValue("bound_relax_factor", 0);
	nlpSolver.retrieve_options()->SetNumericValue("nlp_lower_bound_inf", -prob->inf, false);
	nlpSolver.retrieve_options()->SetNumericValue("nlp_upper_bound_inf",  prob->inf, false);
	if (prob->gms.icutof)
		nlpSolver.retrieve_options()->SetNumericValue("bonmin.cutoff", prob->gms.cutoff);
	nlpSolver.retrieve_options()->SetNumericValue("bonmin.allowable_gap", prob->gms.optca);
	nlpSolver.retrieve_options()->SetNumericValue("bonmin.allowable_fraction_gap", prob->gms.optcr);
	if (prob->gms.nodlim)
		nlpSolver.retrieve_options()->SetIntegerValue("bonmin.node_limit", prob->gms.nodlim);
	else
		nlpSolver.retrieve_options()->SetIntegerValue("bonmin.node_limit", prob->gms.itnlim);
	nlpSolver.retrieve_options()->SetNumericValue("bonmin.time_limit", prob->gms.reslim);

	if (prob->gms.useopt)
		nlpSolver.readOptionFile(prob->gms.optFileName);

	nlpSolver.retrieve_options()->GetNumericValue("diverging_iterates_tol", mysmagminlp->div_iter_tol, "");
	// or should we also check the tolerance for acceptable points?  
	nlpSolver.retrieve_options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, ""); 
	nlpSolver.retrieve_options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, ""); 

  try {
		par(nlpSolver); // process option file
		BonminBB bb;

		double clockStart = smagGetCPUTime (prob);
		bb(nlpSolver, par); //process parameter file using Ipopt and do branch and bound

    int model_status;
    int solver_status;
    int resolve_nlp=false;
    switch (bb.mipStatus()) {
    	case BonminBB::FeasibleOptimal: {
	    	if (bb.bestSolution()) {
	    		resolve_nlp=true;
//	    		model_status=2; // local optimal; we could report optimal if the gap is closed
	    		model_status=1; // optimal; TODO: the gap should be closed!!!
	    	} else { // this should not happen
	    		model_status=13; // error - no solution
	    	}
	    	solver_status=1; // normal completion
	    } break;
    	case BonminBB::ProvenInfeasible: {
	    	model_status=19; // infeasible - no solution
	    	solver_status=1; // normal completion
	    } break;
    	case BonminBB::Feasible: {
	    	if (bb.bestSolution()) {
	    		resolve_nlp=true;
		    	model_status=7; // intermediate nonoptimal
	    	} else { // this should not happen
	    		model_status=13; // error - no solution
	    	}
	    	solver_status=1; // normal completion
    	} break;
    	case BonminBB::NoSolutionKnown: {
    		if (bb.bestSolution()) { // probably this will not happen
    			model_status=6; // intermediate infeasible
    			resolve_nlp=true;
    		}	else {
    			model_status=14; // no solution returned
    		}		    
	    	solver_status=1; // normal completion
    	} break;
    	default : { // should not happen, since other mipStatus is not defined
	    	model_status=12; // error unknown
	    	solver_status=1; // normal completion
    	}
		}

		if (resolve_nlp) {
			bool has_free_var=false;
			for (Index i=0; i<smagColCount(prob); ++i)
				if (prob->colType[i]!=SMAG_VAR_CONT)
					nlpSolver.setColBounds(i, bb.bestSolution()[i], bb.bestSolution()[i]);
				else if ((!has_free_var) && nlpSolver.getColUpper()[i]-nlpSolver.getColLower()[i]>1e-5)
					has_free_var=true; 
			nlpSolver.setColSolution(bb.bestSolution());

			if (!has_free_var) {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "\nAll variables are discrete. Dual variables for fixed problem will be not available.\n");
				nlpSolver.initialSolve(); // this will only evaluate the constraints, so we get correct row levels
				write_solution_nodual(prob, nlpSolver, model_status, solver_status, smagGetCPUTime(prob)-clockStart, mysmagminlp->domviolations, bb.numNodes());
			} else { 
				smagStdOutputPrint(prob, SMAG_LOGMASK, "\nResolve with fixed discrete variables to get dual values.\n");
				nlpSolver.initialSolve();
				if (nlpSolver.isProvenOptimal()) 
					write_solution(prob, nlpSolver, model_status, solver_status, smagGetCPUTime(prob)-clockStart, mysmagminlp->domviolations, bb.numNodes());
				else
					smagStdOutputPrint(prob, SMAG_LOGMASK, "Problems solving fixed problem.\n");
			}
		} else {
			smagReportSolBrief(prob, model_status, solver_status);
		}
  }
  catch(IpoptInterface::UnsolvedError &E) {
    //There has been a failure to solve a problem with Ipopt.
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Ipopt has failed to solve a problem.\n");
	  smagReportSolBrief(prob, 13, 10);
  }
  catch(CoinError &E) {
  	char buf[1024];
  	snprintf(buf, 1024, "%s::%s\n%s\n", E.className().c_str(), E.methodName().c_str(), E.message().c_str());
		smagStdOutputPrint(prob, SMAG_ALLMASK, buf);
	  smagReportSolBrief(prob, 13, 13);
  }
} // solve_minlp()

/** Processes Ipopt solution and calls method to report the solution.
 */
void write_solution(smagHandle_t prob, IpoptInterface& nlpSolver, int model_status, int solver_status, double resuse, int domviol, int nodeuse) {
	int n=smagColCount(prob);
	int m=smagRowCount(prob);
	int isMin=smagMinim(prob);
	
	const double* lambda=nlpSolver.getRowPrice();
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
		nodeuse, resuse, nlpSolver.getObjValue()*isMin, domviol,
		nlpSolver.getRowActivity(), negLambda, rowBasStat, rowIndic,
		nlpSolver.getColSolution(), colMarg, colBasStat, colIndic);
		
	delete[] colBasStat;
	delete[] colIndic;
	delete[] colMarg;
	delete[] rowBasStat;
	delete[] rowIndic;
	delete[] negLambda;

//	const double* rowMarg=new double[smagRowCount(prob)];
//  smagReportSolX(prob, nodeuse, resuse, domviol, rowMarg, nlpSolver.getColSolution());
	
} // write_solution

void write_solution_nodual(smagHandle_t prob, IpoptInterface& nlpSolver, int model_status, int solver_status, double resuse, int domviol, int nodeuse) {
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
		nodeuse, resuse, nlpSolver.getObjValue()*smagMinim(prob), domviol,
		nlpSolver.getRowActivity(), prob->rowPi, rowBasStat, rowIndic,
		nlpSolver.getColSolution(), prob->colRC, colBasStat, colIndic);
		
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
  
	SmartPtr<Journal> smag_listjrnl=new SmagJournal(prob, SMAG_LISTMASK, "SMAGlisting", J_SUMMARY);
	smag_listjrnl->SetPrintLevel(J_DBG, J_NONE);  	
	if (!app->Jnlst()->AddJournal(smag_listjrnl))
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT listing output.\n");  

  if (prob->logOption) {
  	// calling this journal "console" lets IPOPT adjust its print_level according to the print_level parameter (if set) 
  	SmartPtr<Journal> smag_logjrnl=new SmagJournal(prob, SMAG_LOGMASK, "console", J_ITERSUMMARY);
		smag_logjrnl->SetPrintLevel(J_DBG, J_NONE);  	
		if (!app->Jnlst()->AddJournal(smag_logjrnl))
			smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to register SmagJournal for IPOPT logging output.\n");
  }

	// register Bonmin options so that Ipopt does not stumble over bonmin options
	IpoptInterface().register_ALL_options(app->RegOptions());

	// Change some options
  app->Options()->SetNumericValue("bound_relax_factor", 0);
	app->Options()->SetIntegerValue("max_iter", prob->gms.itnlim);
  app->Options()->SetStringValue("mu_strategy", "adaptive");
 	app->Options()->SetNumericValue("nlp_lower_bound_inf", -prob->inf, false);
 	app->Options()->SetNumericValue("nlp_upper_bound_inf",  prob->inf, false);

	if (prob->gms.useopt)
		app->Initialize(prob->gms.optFileName);
	else
		app->Initialize("");
		
	app->Options()->GetNumericValue("diverging_iterates_tol", mysmagnlp->div_iter_tol, "");
	// or should we also check the tolerance for acceptable points?  
	app->Options()->GetNumericValue("tol", mysmagnlp->scaled_conviol_tol, ""); 
	app->Options()->GetNumericValue("constr_viol_tol", mysmagnlp->unscaled_conviol_tol, ""); 
	
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
