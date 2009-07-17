// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Steve Dirkse, Stefan Vigerske

#include "GamsNLP.hpp"
#include "IpIpoptCalculatedQuantities.hpp"

#include "IpIpoptData.hpp"
#include "IpTNLPAdapter.hpp"
#include "IpOrigIpoptNLP.hpp"

// for memset
#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#endif
// for fabs
#ifdef HAVE_CMATH
#include <cmath>
#else
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#endif

#ifdef GAMS_BUILD
#include "gmomcc.h"
#else
#include "gmocc.h"
#endif

using namespace Ipopt;

GamsNLP::GamsNLP (gmoHandle_t gmo_)
: iRowStart(NULL), jCol(NULL), grad(NULL),
  div_iter_tol(1E+20), scaled_conviol_tol(1E-8), unscaled_conviol_tol(1E-4), domviolations(0)
{
  gmo = gmo_;
  assert(gmo);
	
	timelimit    = gmoResLim(gmo);
	domviollimit = gmoDomLim(gmo);
}

GamsNLP::~GamsNLP() {
	delete[] iRowStart;
	delete[] jCol;
	delete[] grad;
}

bool GamsNLP::get_nlp_info(Index& n, Index& m, Index& jac_nnz, Index& nnz_h_lag, TNLP::IndexStyleEnum &index_style) {
	n           = gmoN(gmo);
	m           = gmoM(gmo);
	jac_nnz     = gmoNZ(gmo);
	nnz_h_lag   = gmoHessLagNz(gmo);
	index_style = C_STYLE;
	
	return true;
}

bool GamsNLP::get_bounds_info (Index n, Number* x_l, Number* x_u, Index m, Number* g_l, Number* g_u) {
	assert(n == gmoN(gmo));
	assert(m == gmoM(gmo));
	
	gmoGetVarLower(gmo, x_l);
	gmoGetVarUpper(gmo, x_u);
	gmoGetRhs(gmo, g_u);

	for (int i = 0; i < m; ++i, ++g_u, ++g_l) {
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E:	//equality
				*g_l = *g_u;
				break;
			case equ_G: // greater-equal
				*g_l = *g_u;
				*g_u = gmoPinf(gmo);
				break;
			case equ_L:	// lower-equal
				*g_l = gmoMinf(gmo);
				break;
			case equ_N: // free row
				*g_l = gmoMinf(gmo);
				*g_u = gmoPinf(gmo);
				break;
			case equ_X: // external function TODO: supported now?
				gmoLogStat(gmo, "Error: External functions not supported yet.");
				return false;
				break;
			case equ_C: // conic function
				gmoLogStat(gmo, "Error: Conic constraints not supported.");
				return false;
				break;
			default:
				gmoLogStat(gmo, "Error: Unsuppored equation type.");
				return false;
		}
	}

	return true;
}

bool GamsNLP::get_starting_point(Index n, bool init_x, Number* x, bool init_z, Number* z_L, Number* z_U, Index m, bool init_lambda, Number* lambda) {
	assert(n == gmoN(gmo));
	assert(m == gmoM(gmo));

	if (init_lambda) {
		gmoGetEquM(gmo, lambda);
		for (Index j = m; j; --j, ++lambda)
			*lambda *= -1;
	}
	
	if (init_z) {
		gmoGetVarM(gmo, z_L);
		for (Index j = n; j; --j, ++z_L, ++z_U) {
			if (*z_L < 0) {
				*z_U = -*z_L;
				*z_L = 0;
			} else
				*z_U = 0;
		}
	}
	
	if (init_x) {
		gmoGetVarL(gmo, x);

		// check that we are not above or below tolerance for diverging iterates
  	for (Index j = 0;  j < n;  ++j) {
  		if (x[j] < -div_iter_tol) {
  			char buffer[255];
  			sprintf(buffer, "Initial value %e for variable %d below diverging iterates tolerance %e. Set initial value to %e.\n", x[j], j, -div_iter_tol, -.99*div_iter_tol);
  			gmoLogStatPChar(gmo, buffer);
  			x[j] = -.99*div_iter_tol;
  		} else if (x[j] > div_iter_tol) {
  			char buffer[255];
  			sprintf(buffer, "Initial value %e for variable %d above diverging iterates tolerance %e. Set initial value to %e.\n", x[j], j, div_iter_tol, .99*div_iter_tol);
  			gmoLogStatPChar(gmo, buffer);
  			x[j] = .99*div_iter_tol;
  		}
  	}
	}
	
  return true;
}

bool GamsNLP::get_variables_linearity(Index n, LinearityType* var_types) {
	assert(n == gmoN(gmo));
	
	if (gmoNLNZ(gmo) == 0 && gmoObjNLNZ(gmo) == 0) { // problem is linear
		memset(var_types, LINEAR, n*sizeof(LinearityType));
		return true;
	}
	
	int jnz, jnlnz, jobjnz;
	for (int i = 0; i < n; ++i) {
		gmoGetColStat(gmo, i, &jnz, &jnlnz, &jobjnz);
		if (jnlnz || (jobjnz == 1)) // jobjnz is -1 if linear in obj, +1 if nonlinear in obj, and 0 if not there
			var_types[i] = NON_LINEAR;
		else
			var_types[i] = LINEAR;
	}
	
	return true;
}

// returns the constraint linearity
bool GamsNLP::get_constraints_linearity(Index m, LinearityType* const_types) {
	assert(m == gmoM(gmo));
	
	if (gmoNLNZ(gmo) == 0) { // all constraints are linear
		memset(const_types, LINEAR, m*sizeof(LinearityType));
		return true;
	}

	for (Index i = 0; i < m; ++i)
		const_types[i] = gmoNLfunc(gmo, i) ? NON_LINEAR : LINEAR;
	
	return true;
}

bool GamsNLP::eval_f(Index n, const Number* x, bool new_x, Number& obj_value) {
	assert(n == gmoN(gmo));

	if (new_x)
		gmoEvalNewPoint(gmo, x);
	int nerror = gmoEvalObjFunc(gmo, x, &obj_value);

  if (nerror < 0) {
		char buffer[255];
  	sprintf(buffer, "Error detected in evaluation of objective function!\nnerror = %d\nExiting from subroutine - eval_f\n", nerror);
		gmoLogStatPChar(gmo, buffer);
    throw -1;
  }
  if (nerror > 0) {
		++domviolations;
		return false;
	}

	return true;
}

bool GamsNLP::eval_grad_f (Index n, const Number* x, bool new_x, Number* grad_f) {
	assert(n == gmoN(gmo));
	
	if (new_x)
		gmoEvalNewPoint(gmo, x);

  memset(grad_f, 0, n*sizeof(double));
	double val;
 	double gx;
	int nerror = gmoEvalObjGrad(gmo, x, &val, grad_f, &gx);

  if (nerror < 0) {
		char buffer[255];
  	sprintf(buffer, "Error detected in GAMS evaluation of objective gradient!\nnerror = %d\nExiting from subroutine - eval_grad_f\n", nerror); 
		gmoLogStatPChar(gmo, buffer);
    throw -1;
  }
  if (nerror > 0) {
		++domviolations;
		return false;
	}
	
  return true;
}

bool GamsNLP::eval_g(Index n, const Number *x, bool new_x, Index m, Number *g) {
	assert(n == gmoN(gmo));
	assert(m == gmoM(gmo));

	if (new_x)
		gmoEvalNewPoint(gmo, x);

  int nerror;
  for (int i = 0; i < m; ++i) {
		nerror = gmoEvalFunc(gmo, i, x, &g[i]);
	  if (nerror < 0) {
			char buffer[255];  	
	  	sprintf(buffer, "Error detected in evaluation of constraint %d!\nnerror = %d\nExiting from subroutine - eval_g\n", i, nerror); 
			gmoLogStatPChar(gmo, buffer);
	    throw -1;
	  }
	  if (nerror > 0) {
			++domviolations;
			return false;
		}
  }

  return true;
}

bool GamsNLP::eval_jac_g (Index n, const Number *x, bool new_x, Index m, Index nele_jac, Index *iRow, Index *jCol, Number *values) {
	assert(n == gmoN(gmo));
	assert(m == gmoM(gmo));
	assert(nele_jac == gmoNZ(gmo));
	
  if (values == NULL) { // return the structure of the jacobian
    assert(NULL == x);
    assert(NULL != iRow);
    assert(NULL != jCol);

    delete[] iRowStart;
    delete[] this->jCol;
    iRowStart         = new int[m+1];
    this->jCol        = new int[nele_jac];
    double* jacval    = new double[nele_jac];
    int*    nlflag    = new int[nele_jac];
    gmoGetMatrixRow(gmo, iRowStart, this->jCol, jacval, nlflag);

    assert(iRowStart[m] == nele_jac);
    for (Index i = 0;  i < m;  ++i)
    	for (int j = iRowStart[i]; j < iRowStart[i+1]; ++j)
    		iRow[j] = i;
    memcpy(jCol, this->jCol, nele_jac * sizeof(int));
    
    delete[] jacval;
    delete[] nlflag;
    
    delete[] grad;
    grad = new double[n];

  } else { // return the values of the jacobian
    assert(NULL != x);
    assert(NULL == iRow);
    assert(NULL == jCol);
    assert(NULL != iRowStart);
    assert(NULL != this->jCol);
  	assert(NULL != grad);
  	
  	if (new_x)
  		gmoEvalNewPoint(gmo, x);
  	
  	double val;
  	double gx;
  	int nerror;
    int k = 0;
    int next;
    
  	for (int rownr = 0; rownr < m; ++rownr) {
  		nerror = gmoEvalGrad(gmo, rownr, x, &val, grad, &gx);
      if (nerror < 0) {
  			char buffer[255];  	
  	  	sprintf(buffer, "Error detected in evaluation of gradient for constraint %d!\nnerror = %d\nExiting from subroutine - eval_jac_g\n", rownr, nerror); 
  			gmoLogStatPChar(gmo, buffer);
  			throw -1;
      }
      if (nerror > 0) {
  			++domviolations;
        return false;
      }
      assert(k == iRowStart[rownr]);
      next = iRowStart[rownr+1];
      for (; k < next; ++k)
      	values[k] = grad[this->jCol[k]];
      
//      for (; k < nele_jac && this->iRow[k] == rownr; ++k)
//      	values[k] = grad[this->jCol[k]];
    }
    assert(k == nele_jac);
  }

  return true;
}

bool GamsNLP::eval_h(Index n, const Number *x, bool new_x, Number obj_factor, Index m, const Number *lambda, bool new_lambda, Index nele_hess, Index *iRow, Index *jCol, Number *values) {
	assert(n == gmoN(gmo));
	assert(m == gmoM(gmo));
	assert(nele_hess == gmoHessLagNz(gmo));

  if (values == NULL) { // return the structure. This is a symmetric matrix, fill the lower left triangle only.
    assert(NULL == x);
    assert(NULL == lambda);
    assert(NULL != iRow);
    assert(NULL != jCol);

    gmoHessLagStruct(gmo, iRow, jCol);
        
  } else { // return the values. This is a symmetric matrix, fill the lower left triangle only.
    assert(NULL != x);
    assert(NULL != lambda);
    assert(NULL == iRow);
    assert(NULL == jCol);

  	if (new_x)
  		gmoEvalNewPoint(gmo, x);

  	// for GAMS lambda would need to be multiplied by -1, we do this via the constraint weight
    int nerror = gmoHessLagValue(gmo, const_cast<double*>(x), const_cast<double*>(lambda), values, obj_factor, -1.);
    if (nerror < 0) {
			char buffer[255];  	
	  	sprintf(buffer, "Error detected in evaluation of Hessian!\nnerror = %d\nExiting from subroutine - eval_h\n", nerror);
			gmoLogStatPChar(gmo, buffer);
			throw -1;
    } else if (nerror > 0) {
			++domviolations;
      return false;
    }
  }

  return true;
}

bool GamsNLP::intermediate_callback(AlgorithmMode mode, Index iter, Number obj_value, Number inf_pr, Number inf_du, Number mu, Number d_norm, Number regularization_size, Number alpha_du, Number alpha_pr, Index ls_trials, const IpoptData *ip_data, IpoptCalculatedQuantities *ip_cq) {
	if (timelimit && gmoTimeDiffStart(gmo) - clockStart > timelimit) return false;
	if (domviollimit && domviolations >= domviollimit) return false;
	return true;
}

void GamsNLP::finalize_solution(SolverReturn status, Index n, const Number *x, const Number *z_L, const Number *z_U, Index m, const Number *g, const Number *lambda, Number obj_value, const IpoptData* data, IpoptCalculatedQuantities* cq) {
	assert(n == gmoN(gmo));
	assert(m == gmoM(gmo));

	bool write_solution = false;
	
  switch (status) {
	  case SUCCESS:
	  case STOP_AT_ACCEPTABLE_POINT:
			gmoModelStatSet(gmo, gmoNLNZ(gmo) ? ModelStat_OptimalLocal : ModelStat_OptimalGlobal);
			gmoSolveStatSet(gmo, SolveStat_Normal);
	  	write_solution = true; 
			break;
	  case LOCAL_INFEASIBILITY:
			gmoModelStatSet(gmo, gmoNLNZ(gmo) ? ModelStat_InfeasibleLocal : ModelStat_InfeasibleGlobal);
			gmoSolveStatSet(gmo, SolveStat_Normal);
			write_solution = true;
	    break;
	  case DIVERGING_ITERATES:
			gmoLogStat(gmo, "Diverging iterates: we'll guess unbounded!");
			gmoModelStatSet(gmo, ModelStat_Unbounded);
			gmoSolveStatSet(gmo, SolveStat_Normal);
			write_solution = true;
	    break;
		case STOP_AT_TINY_STEP:
	  case RESTORATION_FAILURE:
			if (cq->curr_nlp_constraint_violation(NORM_MAX) < scaled_conviol_tol && cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < unscaled_conviol_tol) {
				gmoLog(gmo, "Having feasible solution.\n");
				gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
			} else {
				gmoLog(gmo, "Current point is not feasible.");
				gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
			}
			gmoSolveStatSet(gmo, SolveStat_Solver); // terminated by solver (normal completion not allowed by GAMS philosophy here: its not normal when it stops with an intermediate point)
			write_solution = true;
	    break;
	  case MAXITER_EXCEEDED:
			if (cq->curr_nlp_constraint_violation(NORM_MAX) < scaled_conviol_tol && cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < unscaled_conviol_tol) {
				gmoLog(gmo, "Having feasible solution.\n");
				gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
			} else {
				gmoLog(gmo, "Current point is not feasible.");
				gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
			}
			gmoSolveStatSet(gmo, SolveStat_Iteration);
			write_solution=true;
			break;
		case USER_REQUESTED_STOP:
			if (domviollimit && domviolations >= domviollimit) {
				gmoLogStat(gmo, "Domain violation limit exceeded!");
				gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
				gmoSolveStatSet(gmo, SolveStat_EvalError);
			} else {
				if (cq->curr_nlp_constraint_violation(NORM_MAX) < scaled_conviol_tol && cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < unscaled_conviol_tol) {
					gmoLogStat(gmo, "Time limit exceeded! Point is feasible.");
					gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
				} else {
					gmoLogStat(gmo, "Time limit exceeded! Point is not feasible.");
					gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
				}
				gmoSolveStatSet(gmo, SolveStat_Resource);
			}
			write_solution=true;
			break;
		case ERROR_IN_STEP_COMPUTATION:
		case TOO_FEW_DEGREES_OF_FREEDOM:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SolverErr);
			break;
		case INVALID_NUMBER_DETECTED:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
			break;
		case INTERNAL_ERROR:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_InternalErr);
			break;
		case OUT_OF_MEMORY:
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoSolveStatSet(gmo, SolveStat_InternalErr);
			break;
	  default:
	  	char buffer[255];
	  	sprintf(buffer, "OUCH: unhandled SolverReturn of %d\n", status);
			gmoLogStatPChar(gmo, buffer);
			gmoModelStatSet(gmo, ModelStat_ErrorUnknown);
			gmoSolveStatSet(gmo, SolveStat_SystemErr);
  }

  if (data)
  	gmoSetHeadnTail(gmo, Hiterused, data->iter_count());
	gmoSetHeadnTail(gmo, HresUsed, gmoTimeDiffStart(gmo) - clockStart);
	gmoSetHeadnTail(gmo, HdomUsed, domviolations);

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
  	TNLPAdapter* tnlp_adapter = NULL;
  	OrigIpoptNLP* orignlp = NULL;
  	if (cq)
  		orignlp = dynamic_cast<OrigIpoptNLP*>(GetRawPtr(cq->GetIpoptNLP()));
  	if (orignlp)
  		tnlp_adapter = dynamic_cast<TNLPAdapter*>(GetRawPtr(orignlp->nlp()));
  		
  	if (tnlp_adapter) {
  		scaled_viol = new double[m];
  		tnlp_adapter->ResortG(*cq->curr_c(), *cq->curr_d_minus_s(), scaled_viol);
  		
    	SmartPtr<Vector> dummy = cq->curr_c()->MakeNew();
    	dummy->Set(0.);
    	compl_xL = new double[n];
    	compl_xU = new double[n];
    	compl_gL = new double[m];
    	compl_gU = new double[m];

  	  memset(compl_xL, 0, n*sizeof(double));
  	  memset(compl_xU, 0, n*sizeof(double));
  	  memset(compl_gL, 0, m*sizeof(double));
  	  memset(compl_gU, 0, m*sizeof(double));

  	  if (cq->curr_compl_x_L()->Dim() && cq->curr_compl_x_U()->Dim())
  	  	tnlp_adapter->ResortBnds(*cq->curr_compl_x_L(), compl_xL, *cq->curr_compl_x_U(), compl_xU);
    	if (cq->curr_compl_s_L()->Dim())
    		tnlp_adapter->ResortG(*dummy, *cq->curr_compl_s_L(), compl_gL);
    	if (cq->curr_compl_s_U()->Dim())
    		tnlp_adapter->ResortG(*dummy, *cq->curr_compl_s_U(), compl_gU);

//    	for (Index i=0; i<smagRowCount(prob); ++i)
//    		std::cout << "row " << i << " infeas.: " << scaled_viol[i] << std::endl;
//    	for (Index i=0; i<smagColCount(prob); ++i)
//    		std::cout << "col " << i << " compl.: " << compl_xL[i] << '\t' << compl_xU[i] << std::endl;
//    	for (Index i=0; i<smagRowCount(prob); ++i)
//    		std::cout << "row " << i << " compl.: " << compl_gL[i] << '\t' << compl_gU[i] << std::endl;
  	}

		int*    colBasStat = new int[n];
		int*    colIndic   = new int[n];
		double* colMarg    = new double[n];
		for (Index i = 0; i < n; ++i) {
			colBasStat[i] = Bstat_Super;
			if (gmoGetVarLowerOne(gmo, i) != gmoGetVarUpperOne(gmo, i) && compl_xL && (fabs(compl_xL[i]) > scaled_conviol_tol || fabs(compl_xU[i]) > scaled_conviol_tol))
				colIndic[i] = Cstat_NonOpt;
			else
				colIndic[i] = Cstat_OK;
			// if, e.g., x_i has no lower bound, then the dual z_L[i] is -infinity
			colMarg[i] = 0;
			if (z_L[i] > gmoMinf(gmo)) colMarg[i] += z_L[i];
			if (z_U[i] < gmoPinf(gmo)) colMarg[i] -= z_U[i];
		}
		
		int* rowBasStat   = new int[m];
		int* rowIndic     = new int[m];
    double* negLambda = new double[m];
    for (Index i = 0;  i < m;  i++) {
			rowBasStat[i] = Bstat_Super;
			if (scaled_viol && fabs(scaled_viol[i]) > scaled_conviol_tol)
				rowIndic[i] = Cstat_Infeas;
			else if (compl_gL && (fabs(compl_gL[i]) > scaled_conviol_tol || fabs(compl_gU[i]) > scaled_conviol_tol))
				rowIndic[i] = Cstat_NonOpt;
			else
				rowIndic[i] = Cstat_OK;
    	negLambda[i] = -lambda[i];
    }
        
  	gmoSetSolution8(gmo, x, colMarg, negLambda, g, colBasStat, colIndic, rowBasStat, rowIndic);
  	gmoSetHeadnTail(gmo, HobjVal, obj_value);

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
		delete[] negLambda;
	}

}
