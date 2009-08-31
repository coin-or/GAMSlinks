// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#include "GamsMINLP.hpp"
#include "IpIpoptCalculatedQuantities.hpp"

#include "gmomcc.h"
#include "gevmcc.h"

using namespace Ipopt;
using namespace Bonmin;

// constructor
GamsMINLP::	GamsMINLP(struct gmoRec* gmo_)
: gmo(gmo_), gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL), in_couenne(false), hess_iRow(NULL), hess_jCol(NULL)
{
	assert(gmo);
	switch (gmoSense(gmo)) {
		case Obj_Min: isMin =  1.; break;
		case Obj_Max: isMin = -1.; break;
		default:
			gevLogStat(gev, "Error: Unsupported objective sense!\n");
			exit(EXIT_FAILURE);
	}

	nlp = new GamsNLP(gmo);

  setupPrioritiesSOS();
}

GamsMINLP::~GamsMINLP() {
	delete[] hess_iRow;
	delete[] hess_jCol;
}

void GamsMINLP::setupPrioritiesSOS() {
	// range of priority values
	double minprior = gmoMinf(gmo);
	double maxprior = gmoPinf(gmo);
	// take care of integer variables branching priorities
	if (gmoPriorOpt(gmo)) {
		// first check which range of priorities is given
		for (int i = 0; i < gmoN(gmo); ++i) {
			if (gmoGetVarTypeOne(gmo, i) == var_X) continue; // gams forbids branching priorities for continuous variables
			if (gmoGetVarPriorOne(gmo,i) < minprior) minprior = gmoGetVarPriorOne(gmo,i);
			if (gmoGetVarPriorOne(gmo,i) > maxprior) maxprior = gmoGetVarPriorOne(gmo,i);
		}
		if (minprior!=maxprior) {
			branchinginfo.size = gmoN(gmo);
			branchinginfo.priorities=new int[branchinginfo.size];
			for (int i = 0; i < branchinginfo.size; ++i) {
				if (gmoGetVarTypeOne(gmo, i) == var_X) //TODO are priorities now also allowed for continuous vars?
					branchinginfo.priorities[i] = 0;
				else
					// we map gams priorities into the range {1,..,1000}
					// CBC: 1000 is standard priority and 1 is highest priority
					// GAMS: 1 is standard priority for discrete variables, and as smaller the value as higher the priority
					branchinginfo.priorities[i] = 1+(int)(999*(gmoGetVarPriorOne(gmo,i)-minprior)/(maxprior-minprior));
			}
		}
	}

	// Tell solver which variables belong to SOS of type 1 or 2
	int numSos1, numSos2, nzSos, numSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	numSos = numSos1 + numSos2;

	sosinfo.num   = numSos; // number of sos
	sosinfo.numNz = nzSos; // number of variables in sos
	if (!sosinfo.num || !sosinfo.numNz) {
		sosinfo.num   = 0;
		sosinfo.numNz = 0;
		return;
	}

	sosinfo.indices    = new int[sosinfo.numNz];
	sosinfo.starts     = new int[sosinfo.num+1];
	sosinfo.weights    = new double[sosinfo.numNz];
	sosinfo.priorities = new int[sosinfo.num];
	sosinfo.types      = new char[sosinfo.num];
	int* sostype       = new int[numSos];

	gmoGetSosConstraints(gmo, sostype, sosinfo.starts, sosinfo.indices, sosinfo.weights);

	for (int i = 0; i < numSos; ++i) {
		sosinfo.types[i] = (char)sostype[i];
		sosinfo.priorities[i] = gmoN(gmo) - (sosinfo.starts[i+1] - sosinfo.starts[i]); // branch on long sets first
	}

	delete[] sostype;
}

bool GamsMINLP::get_nlp_info(Index& n, Index& m, Index& nnz_jac_g, Index& nnz_h_lag, TNLP::IndexStyleEnum& index_style) {
	return nlp->get_nlp_info(n, m, nnz_jac_g, nnz_h_lag, index_style);
}

// returns the variable bounds
bool GamsMINLP::get_bounds_info (Index n, Number* x_l, Number* x_u, Index m, Number* g_l, Number* g_u) {
	return nlp->get_bounds_info(n, x_l, x_u, m, g_l, g_u);
}

bool GamsMINLP::get_variables_types(Index n, VariableType* var_types) {
	for (Index i = 0; i < n; ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				var_types[i] = CONTINUOUS;
				break;
			case var_B:
				var_types[i] = BINARY;
				break;
			case var_I:
				var_types[i] = INTEGER;
				break;
			case var_S1:
			case var_S2:
				var_types[i] = CONTINUOUS;
				break;
			case var_SC:
			case var_SI:
			default: {
				gevLogStat(gev, "Error: Semicontinuous and semiinteger variables not supported by Bonmin.\n");
			  return false;
			}
		}
	}

	return true;
}

bool GamsMINLP::get_variables_linearity(Index n, Ipopt::TNLP::LinearityType* var_linearity) {
	return nlp->get_variables_linearity(n, var_linearity);
}

bool GamsMINLP::get_constraints_linearity(Index m, Ipopt::TNLP::LinearityType* const_types) {
	return nlp->get_constraints_linearity(m, const_types);
}

bool GamsMINLP::get_starting_point(Index n, bool init_x, Number* x, bool init_z, Number* z_L, Number* z_U, Index m, bool init_lambda, Number* lambda) {
	return nlp->get_starting_point(n, init_x, x, init_z, z_L, z_U, m, init_lambda, lambda);
}

bool GamsMINLP::eval_f(Index n, const Number* x, bool new_x, Number& obj_value) {
	if (!nlp->eval_f(n, x, new_x, obj_value))
		return false;
	obj_value *= isMin;
	return true;
}

bool GamsMINLP::eval_grad_f (Index n, const Number* x, bool new_x, Number* grad_f) {
	if (!nlp->eval_grad_f(n, x, new_x, grad_f))
		return false;

	if (gmoSense(gmo) == Obj_Max)
		for (int i = n; i ; --i)
			(*grad_f++) *= -1;

  return true;
}

bool GamsMINLP::eval_g(Index n, const Number *x, bool new_x, Index m, Number *g) {
	return nlp->eval_g(n, x, new_x, m, g);
}

bool GamsMINLP::eval_jac_g (Index n, const Number *x, bool new_x, Index m, Index nele_jac, Index *iRow, Index *jCol, Number *values) {
	return nlp->eval_jac_g(n, x, new_x, m, nele_jac, iRow, jCol, values);
}

bool GamsMINLP::eval_gi(Index n, const Number* x, bool new_x, Index i, Number& gi) {
	assert(n == gmoN(gmo));

	if (new_x)
		gmoEvalNewPoint(gmo, x);

	int nerror = gmoEvalFunc(gmo, i, x, &gi);

  if (nerror < 0) {
		char buffer[255];
  	sprintf(buffer, "Error detected in evaluation of constraint %d!\nnerror = %d\nExiting from subroutine - eval_g\n", i, nerror);
		gevLogStatPChar(gev, buffer);
    throw -1;
  }
  if (nerror > 0) {
		++nlp->domviolations;
		return false;
	}

	return true;
}

bool GamsMINLP::eval_grad_gi(Index n, const Number* x, bool new_x, Index i, Index& nele_grad_gi, Index* jCol, Number* values) {
	assert(n == gmoN(gmo));
	assert(i < gmoM(gmo));

	if (values == NULL) {
    assert(NULL == x);
    assert(NULL != jCol);
    assert(NULL != nlp->jCol);
    assert(NULL != nlp->iRowStart);

		nele_grad_gi = nlp->iRowStart[i+1] - nlp->iRowStart[i];
		memcpy(jCol, nlp->jCol + nlp->iRowStart[i], nele_grad_gi * sizeof(int));

  } else {
    assert(NULL != x);
    assert(NULL == jCol);
    assert(NULL != nlp->grad);

  	if (new_x)
  		gmoEvalNewPoint(gmo, x);

  	double val;
  	double gx;

  	int nerror = gmoEvalGrad(gmo, i, x, &val, nlp->grad, &gx);

    if (nerror < 0) {
			char buffer[255];
	  	sprintf(buffer, "Error detected in evaluation of gradient of constraint %d!\nnerror = %d\nExiting from subroutine - eval_grad_gi\n", i, nerror);
			gevLogStatPChar(gev, buffer);
	    throw -1;
    }
    if (nerror > 0) {
			++nlp->domviolations;
      return false;
    }
    int next = nlp->iRowStart[i+1];
    for (int k = nlp->iRowStart[i]; k < next; ++k, ++values)
    	*values = nlp->grad[nlp->jCol[k]];
  }

  return true;
}

bool GamsMINLP::eval_h (Index n, const Number *x, bool new_x, Number obj_factor, Index m, const Number *lambda, bool new_lambda, Index nele_hess, Index *iRow, Index *jCol, Number *values) {
	if (values == NULL)
	{ // workaround for GMO bug: gmoLagHessStruct fails on 2nd call
		if (!hess_iRow) { // first call
			hess_iRow = new int[nele_hess];
			hess_jCol = new int[nele_hess];
	    gmoHessLagStruct(gmo, hess_iRow, hess_jCol);
		}
    memcpy(iRow, hess_iRow, nele_hess*sizeof(int));
    memcpy(jCol, hess_jCol, nele_hess*sizeof(int));
    return true;
	}
	return nlp->eval_h(n, x, new_x, isMin*obj_factor, m, lambda, new_lambda, nele_hess, iRow, jCol, values);
}

const TMINLP::SosInfo* GamsMINLP::sosConstraints() const {
	return sosinfo.num ? &sosinfo : NULL;
}

const TMINLP::BranchingInfo* GamsMINLP::branchingInfo() const {
	if (gmoPriorOpt(gmo))
		return &branchinginfo;
	return NULL;
}

void GamsMINLP::finalize_solution(TMINLP::SolverReturn status, Index n, const Number* x, Number obj_value) {
  solver_status = SolveStat_Normal;
  switch (status) {
  	case TMINLP::SUCCESS: {
    	if (x) {
    		if (gevGetDblOpt(gev, gevOptCA) == 0 && gevGetDblOpt(gev, gevOptCR) == 0 && (in_couenne || gmoNLNZ(gmo) == 0)) // report optimal if optcr=optca=0 and we are running in couenne or model is a mip
    			model_status = ModelStat_OptimalGlobal; // optimal
    		else
    			model_status = ModelStat_OptimalLocal; // integer feasible solution
    	} else { // this should not happen
    		model_status = ModelStat_ErrorNoSolution; // error - no solution
    	}
    } break;
  	case TMINLP::INFEASIBLE: {
    	model_status = ModelStat_InfeasibleNoSolution; // infeasible - no solution
    } break;
#ifndef GAMS_BUILD
  	case TMINLP::CONTINUOUS_UNBOUNDED: {
  		model_status = ModelStat_UnboundedNoSolution; // unbounded - no solution
  	} break;
#endif
  	case TMINLP::LIMIT_EXCEEDED: {
			if (gevTimeDiffStart(gev) - nlp->clockStart >= gevGetDblOpt(gev, gevResLim)) {
				solver_status = SolveStat_Resource;
				gevLogStat(gev, "Time limit exceeded.\n");
			} else { // should be iteration limit = node limit
				solver_status = SolveStat_Iteration;
				gevLogStat(gev, "Node limit exceeded.\n");
			}
    	if (x) {
	    	model_status = ModelStat_Integer; // integer feasible solution
    	} else {
    		model_status = ModelStat_NoSolutionReturned; // no solution returned
    	}
  	} break;
  	case TMINLP::MINLP_ERROR: {
  		solver_status = SolveStat_SolverErr;
  		if (x) {
  			model_status = ModelStat_InfeasibleIntermed; // intermediate infeasible
  		}	else {
 				model_status = ModelStat_NoSolutionReturned; // no solution returned
  		}
  	} break;
  	default : { // should not happen, since other mipStatus is not defined
    	model_status  = ModelStat_ErrorUnknown; // error unknown
    	solver_status = SolveStat_InternalErr; // error internal solver error
  	}
	}
}
