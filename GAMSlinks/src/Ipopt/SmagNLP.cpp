// Copyright (C) 2007-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Steve Dirkse, Stefan Vigerske

#include "SmagNLP.hpp"
#include "IpIpoptCalculatedQuantities.hpp"

#include "IpTNLPAdapter.hpp"
#include "IpOrigIpoptNLP.hpp"

#include <memory.h>

using namespace Ipopt;

// constructor
SMAG_NLP::SMAG_NLP (smagHandle_t prob)
: div_iter_tol(1E+20), scaled_conviol_tol(1E-8), unscaled_conviol_tol(1E-4),
  domviolations(0)
{
  this->prob = prob;
  negLambda = new double[smagRowCount(prob)];
  isMin = smagMinim (prob);		/* 1 for min, -1 for max */
	
	timelimit=prob->gms.reslim;
	domviollimit=prob->gms.domlim;
} // SMAG_NLP(prob)

// destructor
SMAG_NLP::~SMAG_NLP()
{
  delete[] negLambda;
}

// returns the size of the problem
bool SMAG_NLP::get_nlp_info (Index& n, Index& m, Index& nnz_jac_g,
	      Index& nnz_h_lag, IndexStyleEnum& index_style) {
  clockStart = smagGetCPUTime (prob);
  n = smagColCount (prob);
  m = smagRowCount (prob);
  nnz_jac_g = smagNZCount (prob); // Jacobian nonzeros
  nnz_h_lag = prob->hesData->lowTriNZ;
  index_style = TNLP::C_STYLE; // 0-based

  return true;
} // SMAG_NLP::get_nlp_info

// returns the variable bounds
bool SMAG_NLP::get_bounds_info (Index n, Number* x_l, Number* x_u,
		 Index m, Number* g_l, Number* g_u) {
  // here, the n and m we gave IPOPT in get_nlp_info are passed back to us.
  // If desired, we could assert to make sure they are what we think they are.
  assert(n == smagColCount (prob));
  assert(m == smagRowCount (prob));

  // if a variable or constraint has NO lower or upper bound, it is set to +/-prob->inf.
  // In the main program we told IPOPT to use this value to represent infinity.

  for (Index j = 0;  j < n;  j++) {
    x_l[j] = prob->colLB[j];
    x_u[j] = prob->colUB[j];
  }

  for (int i = 0;  i < m;  i++) {
    switch (prob->rowType[i]) {
    case SMAG_EQU_EQ:
      g_l[i] = g_u[i] = prob->rowRHS[i];
      break;
    case SMAG_EQU_LT:
      g_u[i] = prob->rowRHS[i];
      g_l[i] = -prob->inf;
      break;
    case SMAG_EQU_GT:
      g_l[i] = prob->rowRHS[i];
      g_u[i] = prob->inf;
      break;
    default:
      smagStdOutputPrint(prob, SMAG_ALLMASK, "Error: Unknown SMAG row type. Exiting ...\n");
      smagStdOutputFlush(prob, SMAG_ALLMASK);
      exit (EXIT_FAILURE);
		} /* switch (rowType) */
  }

  return true;
} // get_bounds_info

// returns the initial point for the problem
bool SMAG_NLP::get_starting_point (Index n, bool init_x, Number* x,
		    bool init_z, Number* z_L, Number* z_U, Index m,
		    bool init_lambda, Number* lambda) {
	if (init_lambda) {
		for (Index j=0; j<m; ++j)
			lambda[j] = -prob->rowPi[j];
	}
	if (init_z) {
		for (Index j=0; j<n; ++j) {
			if (prob->colRC[j]*isMin>0) {
				z_L[j] = isMin*prob->colRC[j];
				z_U[j] = 0;
			} else {
				z_U[j] = -isMin*prob->colRC[j];
				z_L[j] = 0;
			}				
		}
	}
	if (init_x) {
  	for (Index j = 0;  j < n;  ++j) {
  		if (prob->colLev[j]<-div_iter_tol) {
  			x[j]=-.99*div_iter_tol;
  			char buffer[255];
  			sprintf(buffer, "Initial value %e for variable %d below diverging iterates tolerance %e. Set initial value to %e.\n", prob->colLev[j], j, -div_iter_tol, -.99*div_iter_tol);
  			smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
  		} else if (prob->colLev[j]>div_iter_tol) {
  			x[j]=.99*div_iter_tol;
  			char buffer[255];
  			sprintf(buffer, "Initial value %e for variable %d above diverging iterates tolerance %e. Set initial value to %e.\n", prob->colLev[j], j, div_iter_tol, .99*div_iter_tol);
  			smagStdOutputPrint(prob, SMAG_LOGMASK, buffer);
  		} else {
		    x[j] = prob->colLev[j];
			}
  	}
	}
  return true;
} // get_starting_point
#if 0
bool SMAG_NLP::get_scaling_parameters(Number &obj_scaling,
		bool &use_x_scaling, Index n, Number *x_scaling,
		bool &use_g_scaling, Index m, Number *g_scaling) {
	if (!prob->gms.iscopt) { // scale option set? 
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Ipopt asks for user scaling parameters, but GAMS scaling option is not set.\nTry adding <model>.scaleopt = 1; in your GAMS model.\n");
		return false;
	}
	
	bool got_scale_by_zero = (prob->objScale==0.);
	obj_scaling = 1./prob->objScale;
	
	bool all_one=true;
	for (Index i=0; (!got_scale_by_zero) && i<n; ++i) {
		x_scaling[i] = 1./prob->colScale[i];
		all_one &= (prob->colScale[i]==1.);
	}
	use_x_scaling=!all_one;
	
	all_one=false;
	for (Index j=0; (!got_scale_by_zero) && j<m; ++j) {
		g_scaling[j] = 1./prob->rowScale[j];
		all_one &= (prob->rowScale[j]==1.);
	}
	use_g_scaling=!all_one;
	
	if (got_scale_by_zero) {
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Found scale attribute 0. for a variable or constraint. User-scaling turned off.\n");
		return false;
	}
	
	return true;
}
#endif

// returns the variables linearity
bool SMAG_NLP::get_variables_linearity(Index n, LinearityType* var_types) {
	for (Index i=0; i<n; ++i)
		var_types[i]=LINEAR;
	if (smagColCountNL(prob)==0) return true; // no nonlinearities
	
  for (smagObjGradRec_t* og = prob->objGrad;  og;  og = og->next)
  	if (og->degree>1) var_types[og->j]=NON_LINEAR;
  if (smagRowCountNL(prob)==0) return true; // only objective is nonlinear
  
	for (Index i = 0;  i < smagRowCount(prob);  ++i)
		for (smagConGradRec_t* cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next)
			if (cGrad->degree>1) var_types[cGrad->j]=NON_LINEAR;     
	
	return true;
}

// returns the constraint linearity
bool SMAG_NLP::get_constraints_linearity(Index m, LinearityType* const_types) {
	if (!prob->snlData.numInstr) // no nonlinearities
		for (Index i=0; i<m; ++i)
			const_types[i]=LINEAR;
	else
		for (Index i=0; i<m; ++i)
			const_types[i]=prob->snlData.numInstr[i] ? NON_LINEAR : LINEAR;
	
	return true;
}

// returns the value of the objective function
bool SMAG_NLP::eval_f (Index n, const Number* x, bool new_x, Number& obj_value) {
  int nerror = smagEvalObjFunc (prob, x, &obj_value);
  obj_value *= isMin;
  /* Error handling */
  if (nerror < 0) {
		char buffer[255];  	
  	sprintf(buffer, "Error detected in SMAG evaluation!\nnerror = %d\nExiting from subroutine - eval_f\n", nerror); 
		smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		smagStdOutputFlush(prob, SMAG_ALLMASK);
    exit(EXIT_FAILURE);
  } if (nerror > 0) {
		++domviolations;
		return false;
	}

	return true;
} // eval_f

// return the gradient of the objective function grad_{x} f(x)
bool SMAG_NLP::eval_grad_f (Index n, const Number* x, bool new_x, Number* grad_f) {
  double objVal;

  int nerror = smagEvalObjGrad (prob, x, &objVal);
  if (nerror < 0) {
		char buffer[255];  	
  	sprintf(buffer, "Error detected in SMAG evaluation!\nnerror = %d\nExiting from subroutine - eval_grad_f\n", nerror); 
		smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		smagStdOutputFlush(prob, SMAG_ALLMASK);
    exit(EXIT_FAILURE);
  } else if (nerror > 0) {
		++domviolations;
		return false;
	}

  /* does grad_f come in with some zeros already?? */
  memset (grad_f, 0, n*sizeof(double));
  for (smagObjGradRec_t* og = prob->objGrad;  og;  og = og->next)
    grad_f[og->j] = og->dfdj * isMin;

  return true;
} // eval_grad_f

// return the value of the constraints: g(x)
bool SMAG_NLP::eval_g (Index n, const Number *x, bool new_x, Index m, Number *g) {
  int nerror = smagEvalConFunc (prob, x, g);

  /* Error handling */
  if ( nerror < 0 ) {
		char buffer[255];  	
  	sprintf(buffer, "Error detected in SMAG evaluation!\nnerror = %d\nExiting from subroutine - eval_g\n", nerror); 
		smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
		smagStdOutputFlush(prob, SMAG_ALLMASK);
    exit(EXIT_FAILURE);
  } else if (nerror > 0) {
		++domviolations;
		return false;
	}

  return true;
} // eval_g

// return the structure or values of the jacobian
bool SMAG_NLP::eval_jac_g (Index n, const Number *x, bool new_x,
	    Index m, Index nele_jac, Index *iRow, Index *jCol, Number *values) {
  if (values == NULL) {
    assert(NULL==x);
    assert(NULL!=iRow);
    assert(NULL!=jCol);
    // return the structure of the jacobian
    int k = 0;
    for (Index i = 0;  i < m;  ++i) {
      for (smagConGradRec_t* cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next) {
				iRow[k] = i;
				jCol[k] = cGrad->j;
				++k;
      }
    }
    assert(k==smagNZCount(prob));
  } else {
    assert(NULL!=x);
    assert(NULL==iRow);
    assert(NULL==jCol);

    int nerror = smagEvalConGrad (prob, x);
    /* Error handling */
    if (nerror < 0) {
			char buffer[255];  	
	  	sprintf(buffer, "Error detected in SMAG evaluation!\nnerror = %d\nExiting from subroutine - eval_jac_g\n", nerror); 
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputFlush(prob, SMAG_ALLMASK);
	    exit(EXIT_FAILURE);
    } else if (nerror > 0) {
			++domviolations;
      return false;
    }
    int k = 0;
    for (Index i = 0;  i < m;  i++) {
      for (smagConGradRec_t* cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next) {
				values[k++] = cGrad->dcdj;
      }
    }
    assert(k==smagNZCount(prob));
  }

  return true;
} // eval_jac_g

//return the structure or values of the hessian
bool SMAG_NLP::eval_h (Index n, const Number *x, bool new_x,
	Number obj_factor, Index m, const Number *lambda, bool new_lambda,
	Index nele_hess, Index *iRow, Index *jCol, Number *values) {
  if (values == NULL) {
    // return the structure. This is a symmetric matrix, fill the lower left triangle only.
    assert(NULL==x);
    assert(NULL==lambda);
    assert(NULL!=iRow);
    assert(NULL!=jCol);
		int kk = 0;
		int k, kLast;
    for (Index j = 0;  j < n;  j++) {
      for (k = prob->hesData->colPtr[j]-1, kLast = prob->hesData->colPtr[j+1]-1;  k < kLast;  k++) {
				iRow[kk] = prob->hesData->rowIdx[k] - 1;
				jCol[kk] = j;
				kk++;
      }
    }
    assert(prob->hesData->lowTriNZ==kk);
  }
  else {
    // return the values. This is a symmetric matrix, fill the lower left triangle only.
    assert(NULL!=x);
    assert(NULL!=lambda);
    assert(NULL==iRow);
    assert(NULL==jCol);

		for (Index j=0; j<m; ++j)
			negLambda[j]=-lambda[j];

		int nerror;
		smagEvalLagHess (prob, x, isMin*obj_factor, negLambda, values, nele_hess, &nerror);
    if (nerror < 0) {
			char buffer[255];  	
	  	sprintf(buffer, "Error detected in SMAG evaluation!\nnerror = %d\nExiting from subroutine - eval_h\n", nerror); 
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			smagStdOutputFlush(prob, SMAG_ALLMASK);
	    exit(EXIT_FAILURE);
    } else if (nerror > 0) {
			++domviolations;
      return false;
    }
  }

  return true;
} // eval_h

bool SMAG_NLP::intermediate_callback (AlgorithmMode mode, Index iter, Number obj_value, Number inf_pr, Number inf_du, Number mu, Number d_norm, Number regularization_size, Number alpha_du, Number alpha_pr, Index ls_trials, const IpoptData *ip_data, IpoptCalculatedQuantities *ip_cq) {
	if (timelimit && smagGetCPUTime(prob)-clockStart>timelimit) return false;
	if (domviollimit && domviolations>=domviollimit) return false;
	return true;
}

void SMAG_NLP::finalize_solution (SolverReturn status, Index n, const Number *x, const Number *z_L, const Number *z_U,
		   Index m, const Number *g, const Number *lambda, Number obj_value, const IpoptData* data, IpoptCalculatedQuantities* cq) {
	int model_status;
	int solver_status;
	bool write_solution=false;
	
  switch (status) {
	  case SUCCESS:
	  case STOP_AT_ACCEPTABLE_POINT:
	  	model_status=smagColCountNL(prob) ? 2 : 1; // local optimal for nlps, optimal for lps
	  	solver_status=1;
	  	write_solution=true; 
			break;
	  case LOCAL_INFEASIBILITY: 
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Local infeasible!!\n");
			model_status=smagColCountNL(prob) ? 5 : 4; // local infeasible for nlps, infeasible for lps
			solver_status=1;
			write_solution=true;
	    break;
	  case DIVERGING_ITERATES:
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Diverging iterates: we'll guess unbounded!!\n");
			model_status=3;
			solver_status=1;
			write_solution=true;
	    break;
		case STOP_AT_TINY_STEP:
	  case RESTORATION_FAILURE:
			if (cq->curr_nlp_constraint_violation(NORM_MAX) < scaled_conviol_tol && cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < unscaled_conviol_tol) {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Restoration failed or stop at tiny step: we don't know about optimality, but we have feasibility!!\n");
				model_status=7; // intermediate nonoptimal
			} else {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Restoration failed or stop at tiny step: point is not feasibile!!\n");
				model_status=6; // intermediate infeasible
			}
			solver_status=4; // terminated by solver (normal completion not allowed by GAMS philosophy here: its not normal when it stops with an intermediate point)
			write_solution=true;
	    break;
	  case MAXITER_EXCEEDED:
			if (cq->curr_nlp_constraint_violation(NORM_MAX) < scaled_conviol_tol && cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < unscaled_conviol_tol) {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Iteration limit exceeded!! Point is feasible.\n");
				model_status=7; // intermediate nonoptimal
			} else {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Iteration limit exceeded!! Point is not feasible.\n");
				model_status=6; // intermediate infeasible
			}
			solver_status=2;
			write_solution=true;
			break;
		case USER_REQUESTED_STOP:
			if (domviollimit && domviolations>=domviollimit) {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Domain violation limit exceeded!!\n");
				model_status=6; // intermediate infeasible
				solver_status=5;
			} else {
				if (cq->curr_nlp_constraint_violation(NORM_MAX) < scaled_conviol_tol && cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < unscaled_conviol_tol) {
					smagStdOutputPrint(prob, SMAG_LOGMASK, "Time limit exceeded!! Point is feasible.\n");
					model_status=7; // intermediate nonoptimal
				} else {
					smagStdOutputPrint(prob, SMAG_LOGMASK, "Time limit exceeded!! Point is not feasible.\n");
					model_status=6; // intermediate infeasible
				}
				solver_status=3;
			}
			write_solution=true;
			break;
		case ERROR_IN_STEP_COMPUTATION:
		case TOO_FEW_DEGREES_OF_FREEDOM:
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Error in step compuation or too few degrees of freedom!!\n");
			model_status=13;
			solver_status=10;
			break;
		case INVALID_NUMBER_DETECTED:
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Invalid number detected!!\n");
			model_status=13;
			solver_status=13;
			break;
		case INTERNAL_ERROR:
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Internal error!!\n");
			model_status=13;
			solver_status=11;
			break;
	  default:
	  	char buffer[255];
	  	sprintf(buffer, "OUCH: unhandled SolverReturn of %d\n", status);
			smagStdOutputPrint(prob, SMAG_ALLMASK, buffer);
			model_status=12;
			solver_status=13;
  } // switch

  if (write_solution) {
  	double* scaled_viol=NULL;
  	double* compl_xL = NULL;
  	double* compl_xU = NULL;
  	double* compl_gL = NULL;
  	double* compl_gU = NULL;
  	//  	1. Use IpoptCalculatedQuantities::GetIpoptNLP() to get a pointer to an Ipopt::IpoptNLP.
  	//  	2. Cast this Ipopt::IpoptNLP to an Ipopt::OrigIpoptNLP
  	//  	3. Use Ipopt::OrigIpoptNLP::nlp() to get the Ipopt::NLP
  	//  	4. This Ipopt::NLP is actually the TNLPAdapter
  	TNLPAdapter* tnlp_adapter=NULL;
  	OrigIpoptNLP* orignlp=NULL;
  	if (cq)
  		orignlp=dynamic_cast<OrigIpoptNLP*>(GetRawPtr(cq->GetIpoptNLP()));
  	if (orignlp)
  		tnlp_adapter=dynamic_cast<TNLPAdapter*>(GetRawPtr(orignlp->nlp()));
  		
  	if (tnlp_adapter) {
  		scaled_viol = new double[smagRowCount(prob)];
  		tnlp_adapter->ResortG(*cq->curr_c(), *cq->curr_d_minus_s(), scaled_viol);
  		
    	SmartPtr<Vector> dummy=cq->curr_c()->MakeNew();
    	dummy->Set(0.);
    	compl_xL = new double[smagColCount(prob)];
    	compl_xU = new double[smagColCount(prob)];
    	compl_gL = new double[smagRowCount(prob)];
    	compl_gU = new double[smagRowCount(prob)];

    	if (cq->curr_compl_x_L()->Dim())
    		tnlp_adapter->ResortX(*cq->curr_compl_x_L(), compl_xL);
    	else
    	  memset(compl_xL, 0, n*sizeof(double));
    	if (cq->curr_compl_x_U()->Dim())
    		tnlp_adapter->ResortX(*cq->curr_compl_x_U(), compl_xU);
    	else
    	  memset(compl_xU, 0, n*sizeof(double));
    	if (cq->curr_compl_s_L()->Dim())
    		tnlp_adapter->ResortG(*dummy, *cq->curr_compl_s_L(), compl_gL);
    	else
    	  memset(compl_gL, 0, m*sizeof(double));
    	if (cq->curr_compl_s_U()->Dim())
    		tnlp_adapter->ResortG(*dummy, *cq->curr_compl_s_U(), compl_gU);
    	else
    		memset(compl_gU, 0, m*sizeof(double));

//    	for (Index i=0; i<smagRowCount(prob); ++i)
//    		std::cout << "row " << i << " infeas.: " << scaled_viol[i] << std::endl;
//    	for (Index i=0; i<smagColCount(prob); ++i)
//    		std::cout << "col " << i << " compl.: " << compl_xL[i] << '\t' << compl_xU[i] << std::endl;
//    	for (Index i=0; i<smagRowCount(prob); ++i)
//    		std::cout << "row " << i << " compl.: " << compl_gL[i] << '\t' << compl_gU[i] << std::endl;
  	}

		unsigned char* colBasStat=new unsigned char[n];
		unsigned char* colIndic=new unsigned char[n];
		double* colMarg=new double[n];
		for (Index i=0; i<n; ++i) {
			colBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
			if (prob->colLB[i]!=prob->colUB[i] && compl_xL && (fabs(compl_xL[i])>scaled_conviol_tol || fabs(compl_xU[i])>scaled_conviol_tol))
				colIndic[i]=SMAG_RCINDIC_NONOPT;
			else
				colIndic[i]=SMAG_RCINDIC_OK;
			// if, e.g., x_i has no lower bound, then the dual z_L[i] is -infinity
			colMarg[i]=0;
			if (z_L[i]>-prob->inf) colMarg[i]+=isMin*z_L[i];
			if (z_U[i]<prob->inf) colMarg[i]-=isMin*z_U[i];
		}
		unsigned char* rowBasStat=new unsigned char[m];
		unsigned char* rowIndic=new unsigned char[m];
    for (Index i = 0;  i < m;  i++) {
			rowBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
			if (scaled_viol && fabs(scaled_viol[i]) > scaled_conviol_tol)
				rowIndic[i]=SMAG_RCINDIC_INFEAS;
			else if (compl_gL && (fabs(compl_gL[i]) > scaled_conviol_tol || fabs(compl_gU[i]) > scaled_conviol_tol))
				rowIndic[i]=SMAG_RCINDIC_NONOPT;
			else
				rowIndic[i]=SMAG_RCINDIC_OK;
      negLambda[i] = -lambda[i] * isMin;
    }
    smagSetObjEst(prob, obj_value*isMin);
		smagReportSolFull(prob, model_status, solver_status,
			data ? data->iter_count() : SMAG_INT_NA, smagGetCPUTime(prob)-clockStart, obj_value*isMin, domviolations,
			g, negLambda, rowBasStat, rowIndic,
			x, colMarg, colBasStat, colIndic);

		delete[] scaled_viol;
  	delete[] compl_xL;
  	delete[] compl_xU;
  	delete[] compl_gL;
  	delete[] compl_gU;
		delete[] colBasStat;
		delete[] colIndic;
		delete[] colMarg;
		delete[] rowBasStat;
		delete[] rowIndic;
  } else {
		smagReportSolStats (prob, model_status, solver_status, SMAG_INT_NA, smagGetCPUTime(prob)-clockStart, SMAG_DBL_NA, domviolations);
  }
  

} // finalize_solution
