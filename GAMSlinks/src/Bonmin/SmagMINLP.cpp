// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Steve Dirkse, Stefan Vigerske

#include "SmagMINLP.hpp"
#include "IpIpoptCalculatedQuantities.hpp"

using namespace Ipopt;

// constructor
SMAG_MINLP::SMAG_MINLP (smagHandle_t prob)
: div_iter_tol(1E+20), scaled_conviol_tol(1E-8), unscaled_conviol_tol(1E-4),
  domviolations(0)
{
  this->prob = prob;
  negLambda = new double[smagRowCount(prob)];
  isMin = smagMinim (prob);		/* 1 for min, -1 for max */
	
	domviollimit=prob->gms.domlim;
} // SMAG_MINLP(prob)

// destructor
SMAG_MINLP::~SMAG_MINLP()
{
  delete[] negLambda;
}

// returns the size of the problem
bool SMAG_MINLP::get_nlp_info (Index& n, Index& m, Index& nnz_jac_g,
	      Index& nnz_h_lag, TNLP::IndexStyleEnum& index_style) {
  n = smagColCount (prob);
  m = smagRowCount (prob);
  nnz_jac_g = smagNZCount (prob); // Jacobian nonzeros
  nnz_h_lag = prob->hesData->lowTriNZ;
  index_style = TNLP::C_STYLE; 

  return true;
} // SMAG_MINLP::get_nlp_info

// returns the variable bounds
bool SMAG_MINLP::get_bounds_info (Index n, Number* x_l, Number* x_u,
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

#include <vector>
#include <list>
bool SMAG_MINLP::get_var_types(Index n, VariableType* var_types) {
	sosinfo.num=prob->gms.nosos1+prob->gms.nosos2; // number of sos
	sosinfo.types=new char[sosinfo.num]; // types of sos
	sosinfo.numNz=prob->gms.nsos1+prob->gms.nsos2; // number of variables in sos
	// collects for each sos the variables which are in there
	std::vector<std::list<int> > sosvar(sosinfo.num);  

	for (Index i=0; i<n; ++i) {
		switch (prob->colType[i]) {
			case SMAG_VAR_CONT:
				var_types[i]=CONTINUOUS;
				break;
			case SMAG_VAR_BINARY:
				var_types[i]=BINARY;
				break;
			case SMAG_VAR_INTEGER:
				var_types[i]=INTEGER;
				break;
			case SMAG_VAR_SOS1:
				sosvar[0].push_back(i);
				sosinfo.types[0]=1;
				var_types[i]=CONTINUOUS;
//				break;
			case SMAG_VAR_SOS2:
				sosvar[0].push_back(i);
				sosinfo.types[0]=2;
				var_types[i]=CONTINUOUS;
//				break;
			case SMAG_VAR_SEMICONT:
			case SMAG_VAR_SEMIINT:
			default: {
				char msg[255];
				sprintf(msg, "Error: Column type %d for variable %d unknown or not supported. Exiting ...\n", prob->colType[i], i);
				smagStdOutputPrint(prob, SMAG_ALLMASK, msg); 			
	      smagStdOutputFlush(prob, SMAG_ALLMASK);
  	    exit (EXIT_FAILURE);
			}			
		}
	}
	
	sosinfo.indices=new int[sosinfo.numNz];
	sosinfo.weights=new double[sosinfo.numNz];
	sosinfo.starts=new int[sosinfo.num+1];
	int k=0;
	for (unsigned int i=0; i<sosvar.size(); ++i) {
		sosinfo.starts[i]=k;
//		std::clog << i << ' ' << (int)sosinfo.types[i] << ':';
		for (std::list<int>::iterator it(sosvar[i].begin()); it!=sosvar[i].end(); ++it, ++k) {
			sosinfo.indices[k]=*it;
			sosinfo.weights[k]=k-sosinfo.starts[i];
//			std::clog << *it << ' ';		
		}
//		std::clog << std::endl;
	}
	sosinfo.starts[sosvar.size()]=k;
	
	
	return true;	
} // get_var_types
  
bool SMAG_MINLP::get_constraints_types(Index m, ConstraintType* const_types) {
	if (!prob->snlData.numInstr) // no nonlinearities
		for (Index i=0; i<m; ++i)
			const_types[i]=LINEAR;
	else
		for (Index i=0; i<m; ++i)
			const_types[i]=prob->snlData.numInstr[i] ? NON_LINEAR : LINEAR;
	
	return true;
} // get_constraints_types

// returns the initial point for the problem
bool SMAG_MINLP::get_starting_point (Index n, bool init_x, Number* x,
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

// returns the value of the objective function
bool SMAG_MINLP::eval_f (Index n, const Number* x, bool new_x, Number& obj_value) {
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
bool SMAG_MINLP::eval_grad_f (Index n, const Number* x, bool new_x, Number* grad_f) {
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
bool SMAG_MINLP::eval_g (Index n, const Number *x, bool new_x, Index m, Number *g) {
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
bool SMAG_MINLP::eval_jac_g (Index n, const Number *x, bool new_x,
	    Index m, Index nele_jac, Index *iRow, Index *jCol, Number *values) {
  if (values == NULL) {
    assert(NULL==x);
    assert(NULL!=iRow);
    assert(NULL!=jCol);
    smagConGradRec_t* cGrad;
    // return the structure of the jacobian
   	for (Index i=0; i<m; ++i)
			for (cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next) {
				*(iRow++) = i;
				*(jCol++) = cGrad->j;								
			}
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
    for (Index i = 0;  i < m;  i++)
      for (smagConGradRec_t* cGrad = prob->conGrad[i];  cGrad;  cGrad = cGrad->next)
				*(values++) = cGrad->dcdj;
  }

  return true;
} // eval_jac_g

//return the structure or values of the hessian
bool SMAG_MINLP::eval_h (Index n, const Number *x, bool new_x,
	Number obj_factor, Index m, const Number *lambda, bool new_lambda,
	Index nele_hess, Index *iRow, Index *jCol, Number *values) {
  if (values == NULL) {
    // return the structure. This is a symmetric matrix, fill the lower left triangle only.
    assert(NULL==x);
    assert(NULL==lambda);
    assert(NULL!=iRow);
    assert(NULL!=jCol);
		int k, kLast;
    for (Index j = 0;  j < n;  j++) {
      for (k = prob->hesData->colPtr[j]-1, kLast = prob->hesData->colPtr[j+1]-1;  k < kLast;  k++) {
				*(iRow++) = prob->hesData->rowIdx[k] - 1;
				*(jCol++) = j;
      }
    }
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

bool SMAG_MINLP::intermediate_callback (AlgorithmMode mode, Index iter, Number obj_value, Number inf_pr, Number inf_du, Number mu, Number d_norm, Number regularization_size, Number alpha_du, Number alpha_pr, Index ls_trials, const IpoptData *ip_data, IpoptCalculatedQuantities *ip_cq) {
	last_iterationnumber=iter;
	last_scaled_conviol = ip_cq->curr_nlp_constraint_violation(NORM_MAX);	
	last_unscaled_conviol = ip_cq->unscaled_curr_nlp_constraint_violation(NORM_MAX);	
	if (domviollimit && domviolations>=domviollimit) return false;
	return true;
}

const TMINLP::SosInfo* SMAG_MINLP::sosConstraints() const {
	return NULL; // smag does not support sos yet
	if (!sosinfo.num) return NULL;	
	return &sosinfo;
}
