// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Steve Dirkse, Stefan Vigerske

#if ! defined(__SMAGMINLP_HPP__)
#define       __SMAGMINLP_HPP__

#include "GAMSlinksConfig.h"

#include "BonTMINLP.hpp"

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
using namespace Bonmin;

/** A TNLP for Ipopt that uses SMAG to interface the problem formulation.
 */
class SMAG_MINLP : public TMINLP {
public:
  /** Contructor.
   * @param prob The SMAG handle for the problem.
   */
  SMAG_MINLP (smagHandle_t prob);

  /** Default destructor. */
  virtual ~SMAG_MINLP();

  /** Method to return some info about the nlp */
  virtual bool get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                            Index& nnz_h_lag, TNLP::IndexStyleEnum& index_style);

  /** Method to return the bounds for my problem */
  virtual bool get_bounds_info(Index n, Number* x_l, Number* x_u,
                               Index m, Number* g_l, Number* g_u);

  /** Pass the type of the variables (INTEGER, BINARY, CONTINUOUS) to the optimizer.
	 * @param n size of var_types (has to be equal to the number of variables in the problem).
	 * @param var_types types of the variables (has to be filled by function).
  */
  virtual bool get_variables_types(Index n, VariableType* var_types);

  /** Pass the type of the variables linearity (LINEAR, NON_LINEAR) to the optimizer.
   * @param n size of var_linearity (has to be equal to the number of variables in the problem).
   * @param var_linearity linearity of the variables (has to be filled by function).
  */
  virtual bool get_variables_linearity(Index n, Ipopt::TNLP::LinearityType* var_linearity);

  /** Pass the type of the constraints (LINEAR, NON_LINEAR) to the optimizer.
   * @param m size of const_types (has to be equal to the number of constraints in the problem).
   * @param const_types types of the constraints (has to be filled by function).
  */
  virtual bool get_constraints_linearity(Index m, Ipopt::TNLP::LinearityType* const_types);


  /** Method to return the starting point for the algorithm */
  virtual bool get_starting_point(Index n, bool init_x, Number* x,
                                  bool init_z, Number* z_L, Number* z_U,
                                  Index m, bool init_lambda,
                                  Number* lambda);

  virtual bool get_scaling_parameters(Number &obj_scaling,
  		bool &use_x_scaling, Index n, Number *x_scaling,
  		bool &use_g_scaling, Index m, Number *g_scaling);

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

	/** Compute the value of a single constraint.
	 * @param n the number of variables
	 * @param x the point to evaluate
	 * @param new_x whether x is a new point
	 * @param i the constraint number (starting counting from 0)
	 * @param gi to store the value of g_i at x
	 */
	virtual bool eval_gi(Index n, const Number* x, bool new_x, Index i, Number& gi);
	
	/** Compute the structure or values of the gradient for one constraint.
	 * Things are like with eval_jac_g.
	 * @param n the number of variables
	 * @param x the point to compute the gradient for
	 * @param new_x whether x is a new point
	 * @param i the constraint number (starting counting from 0)
	 * @param nele_grad_gi the number of nonzero elements in the gradient of g_i
	 * @param jCol the indices of the nonzero columns
	 * @param values the values for the nonzero columns
	 */
	virtual bool eval_grad_gi(Index n, const Number* x, bool new_x,
		Index i, Index& nele_grad_gi, Index* jCol, Number* values);

  /** Method to return:
   *   1) The structure of the hessian of the lagrangian (if "values" is NULL)
   *   2) The values of the hessian of the lagrangian (if "values" is not NULL)
   */
  virtual bool eval_h(Index n, const Number* x, bool new_x,
                      Number obj_factor, Index m, const Number* lambda,
                      bool new_lambda, Index nele_hess, Index* iRow,
                      Index* jCol, Number* values);

	virtual void finalize_solution(TMINLP::SolverReturn status,Index n, const Number* x, Number obj_value);

	/** Provides information about SOS constraints.
	 */
  virtual const SosInfo* sosConstraints() const;
  
	/** Provides information about branching priorities. 
	 */
  virtual const BranchingInfo* branchingInfo() const;

	double div_iter_tol;
	long int domviolations;
	double clock_start; // time when solving starts
	int model_status, solver_status;
private:
  smagHandle_t prob;
  double *negLambda;
  double isMin;
//	long int domviollimit;

	SosInfo sosinfo;
	BranchingInfo branchinginfo;

  /* Methods to block default compiler methods.
   * The compiler automatically generates the following three methods.
   *  Since the default compiler implementation is generally not what
   *  you want (for all but the most simple classes), we usually 
   *  put the declarations of these methods in the private section
   *  and never implement them. This prevents the compiler from
   *  implementing an incorrect "default" behavior without us
   *  knowing. (See Scott Meyers book, "Effective C++")
   */
	SMAG_MINLP();
  SMAG_MINLP(const SMAG_MINLP&);
  SMAG_MINLP& operator=(const SMAG_MINLP&);

  /** Internal routine to initialize sosinfo and branchinginfo.
   */
  void setupPrioritiesSOS();
};


#endif /* if ! defined(__SMAGMINLP_HPP__) */
