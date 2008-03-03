// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Steve Dirkse, Stefan Vigerske

#if ! defined(__SMAGNLP_HPP__)
#define       __SMAGNLP_HPP__

#include "GAMSlinksConfig.h"

#include "IpTNLP.hpp"

//#include "IpIpoptApplication.hpp"

// smag.h will try to include stdio.h and stdarg.h
// so we include cstdio and cstdarg before if we know that we have them
#ifdef HAVE_CSTDIO
#include <cstdio>
#endif
#ifdef HAVE_CSTDARG
#include <cstdarg>
#endif
#include "smag.h"

using namespace Ipopt;

/** A TNLP for Ipopt that uses SMAG to interface the problem formulation.
 */
class SMAG_NLP : public TNLP {
public:
  /** Contructor.
   * @param prob The SMAG handle for the problem.
   */
  SMAG_NLP (smagHandle_t prob);

  /** Default destructor. */
  virtual ~SMAG_NLP();

  /** Method to return some info about the nlp */
  virtual bool get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                            Index& nnz_h_lag, IndexStyleEnum& index_style);

  /** Method to return the bounds for my problem */
  virtual bool get_bounds_info(Index n, Number* x_l, Number* x_u,
                               Index m, Number* g_l, Number* g_u);

  /** Method to return the starting point for the algorithm */
  virtual bool get_starting_point(Index n, bool init_x, Number* x,
                                  bool init_z, Number* z_L, Number* z_U,
                                  Index m, bool init_lambda,
                                  Number* lambda);
#if 0
  virtual bool get_scaling_parameters(Number &obj_scaling,
  		bool &use_x_scaling, Index n, Number *x_scaling,
  		bool &use_g_scaling, Index m, Number *g_scaling);
#endif
  /** Method to return the variables linearity. */
  virtual bool get_variables_linearity(Index n, LinearityType* var_types);

  /** Method to return the constraint linearity. */
  virtual bool get_constraints_linearity(Index m, LinearityType* const_types);

  /** Method to return the objective value */
  virtual bool eval_f(Index n, const Number* x, bool new_x, Number& obj_value);

  /** Method to return the gradient of the objective */
  virtual bool eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f);

  /** Method to return the constraint residuals */
  virtual bool eval_g(Index n, const Number* x, bool new_x, Index m, Number* g);

  /** Method to return:
   *   1) The structure of the jacobian (if "values" is NULL)
   *   2) The values of the jacobian (if "values" is not NULL)
   */
  virtual bool eval_jac_g(Index n, const Number* x, bool new_x,
                          Index m, Index nele_jac, Index* iRow, Index *jCol,
                          Number* values);

  /** Method to return:
   *   1) The structure of the hessian of the lagrangian (if "values" is NULL)
   *   2) The values of the hessian of the lagrangian (if "values" is not NULL)
   */
  virtual bool eval_h(Index n, const Number* x, bool new_x,
                      Number obj_factor, Index m, const Number* lambda,
                      bool new_lambda, Index nele_hess, Index* iRow,
                      Index* jCol, Number* values);

	/** Method called by the solver at each iteration.
	 * Checks the domain violation limit and time limit and stops the solver in case of limit exceedance.
	 */
	virtual bool intermediate_callback (AlgorithmMode mode, Index iter, Number obj_value, Number inf_pr, Number inf_du, Number mu, Number d_norm, Number regularization_size, Number alpha_du, Number alpha_pr, Index ls_trials, const IpoptData *ip_data, IpoptCalculatedQuantities *ip_cq);

  /** This method is called when the algorithm is complete so the TNLP can store/write the solution.
   */
  virtual void finalize_solution(SolverReturn status,
		    Index n, const Number* x, const Number* z_L,
		    const Number* z_U,
		    Index m, const Number* g, const Number* lambda,
		    Number obj_value,
		    const IpoptData* data, IpoptCalculatedQuantities* cq);

	double div_iter_tol;
	double scaled_conviol_tol;
	double unscaled_conviol_tol;
	
//	IpoptApplication* ipopt_app;
	
private:
  smagHandle_t prob;
  double clockStart;
  double *negLambda;
  double isMin;
	double timelimit;
	int domviollimit;
	long int domviolations;

  /* Methods to block default compiler methods.
   * The compiler automatically generates the following three methods.
   *  Since the default compiler implementation is generally not what
   *  you want (for all but the most simple classes), we usually 
   *  put the declarations of these methods in the private section
   *  and never implement them. This prevents the compiler from
   *  implementing an incorrect "default" behavior without us
   *  knowing. (See Scott Meyers book, "Effective C++")
   */
	SMAG_NLP();
  SMAG_NLP(const SMAG_NLP&);
  SMAG_NLP& operator=(const SMAG_NLP&);
};


#endif /* if ! defined(__SMAGNLP_HPP__) */
