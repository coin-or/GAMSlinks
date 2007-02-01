// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Steve Dirkse, Stefan Vigerske

#include "SmagNLP.hpp"

using namespace Ipopt;

// constructor
SMAG_NLP::SMAG_NLP (smagHandle_t prob)
: domviolations(0)
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
  	for (Index j = 0;  j < n;  ++j)
	    x[j] = prob->colLev[j];
	}
  return true;
} // get_starting_point

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
		   Index m, const Number *g, const Number *lambda, Number obj_value) {
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
			//TODO: can Ipopt tell me if current point is feasible?
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Restoration failed or stop at tiny step: we don't know about optimality or feasibility!!\n");
			model_status=6;
			solver_status=4;
			write_solution=true;
	    break;
	  case MAXITER_EXCEEDED:
			smagStdOutputPrint(prob, SMAG_LOGMASK, "Iteration limit exceeded!!\n");
			model_status=6;
			solver_status=2;
			write_solution=true;
			break;
		case USER_REQUESTED_STOP:
			if (domviollimit && domviolations>=domviollimit) {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Domain violation limit exceeded!!\n");
				model_status=6;
				solver_status=5;
			} else {
				smagStdOutputPrint(prob, SMAG_LOGMASK, "Time limit exceeded!!\n");
				model_status=6;
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

//TODO: set iteration number
  if (write_solution) {
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
//		double* rowLev=new double[m];
    for (Index i = 0;  i < m;  i++) {
			rowBasStat[i]=SMAG_BASSTAT_SUPERBASIC;
			rowIndic[i]=SMAG_RCINDIC_OK;
      negLambda[i] = -lambda[i] * isMin;
//			if (prob->rowType[i]==SMAG_EQU_EQ) {
//				rowLev[i]=g[i]+prob->rowRHS[i]; //TODO: why I have todo this?
//			} else {
//				rowLev[i]=g[i];
//			}
    }
		smagReportSolFull(prob, model_status, solver_status,
			SMAG_INT_NA, smagGetCPUTime(prob)-clockStart, obj_value*isMin, domviolations,
			/*rowLev*/ g, negLambda, rowBasStat, rowIndic,
			x, colMarg, colBasStat, colIndic);
			
		delete[] colBasStat;
		delete[] colIndic;
		delete[] colMarg;
		delete[] rowBasStat;
		delete[] rowIndic;
//		delete[] rowLev;
  } else {
		smagReportSolStats (prob, model_status, solver_status, SMAG_INT_NA, smagGetCPUTime(prob)-clockStart, SMAG_DBL_NA, domviolations);
  }
  

} // finalize_solution
