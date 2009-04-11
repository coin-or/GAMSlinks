// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSMINLP_HPP_
#define GAMSMINLP_HPP_

#include "GAMSlinksConfig.h"

#include "GamsNLP.hpp"
class GamsBonmin;

struct gmoRec;

#include "BonTMINLP.hpp"

/** A TMINLP for Bonmin that uses GMO to interface the problem formulation.
 */
class GamsMINLP : public Bonmin::TMINLP {
	friend class GamsBonmin;
private:
	struct gmoRec* gmo;
  double isMin;

	Bonmin::TMINLP::SosInfo sosinfo;
	Bonmin::TMINLP::BranchingInfo branchinginfo;
	
	SmartPtr<GamsNLP> nlp;
	
	int* hess_iRow;
	int* hess_jCol;

  /** Internal routine to initialize sosinfo and branchinginfo.
   */
  void setupPrioritiesSOS();
  
public:
	int model_status;
	int solver_status;
	
	GamsMINLP(struct gmoRec* gmo);

  ~GamsMINLP();

  /** Method to return some info about the nlp */
  bool get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                            Index& nnz_h_lag, TNLP::IndexStyleEnum& index_style);

  /** Method to return the bounds for my problem */
  bool get_bounds_info(Index n, Number* x_l, Number* x_u,
                               Index m, Number* g_l, Number* g_u);

  /** Pass the type of the variables (INTEGER, BINARY, CONTINUOUS) to the optimizer.
	 * @param n size of var_types (has to be equal to the number of variables in the problem).
	 * @param var_types types of the variables (has to be filled by function).
  */
  bool get_variables_types(Ipopt::Index n, Bonmin::TMINLP::VariableType* var_types);

  /** Pass the type of the variables linearity (LINEAR, NON_LINEAR) to the optimizer.
   * @param n size of var_linearity (has to be equal to the number of variables in the problem).
   * @param var_linearity linearity of the variables (has to be filled by function).
  */
  bool get_variables_linearity(Ipopt::Index n, Ipopt::TNLP::LinearityType* var_types);

  /** Pass the type of the constraints (LINEAR, NON_LINEAR) to the optimizer.
   * @param m size of const_types (has to be equal to the number of constraints in the problem).
   * @param const_types types of the constraints (has to be filled by function).
  */
  bool get_constraints_linearity(Index m, Ipopt::TNLP::LinearityType* const_types);

  /** Method to return the starting point for the algorithm */
  bool get_starting_point(Ipopt::Index n, bool init_x, Ipopt::Number* x,
  		bool init_z, Ipopt::Number* z_L, Ipopt::Number* z_U,
  		Ipopt::Index m, bool init_lambda,
  		Ipopt::Number* lambda);

  /** Method to return the objective value */
  bool eval_f(Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Number& obj_value);

  /** Method to return the gradient of the objective */
  bool eval_grad_f(Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Number* grad_f);

  /** Method to return the constraint residuals */
  bool eval_g(Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Index m, Ipopt::Number* g);

  /** Method to return:
   *   1) The structure of the jacobian (if "values" is NULL)
   *   2) The values of the jacobian (if "values" is not NULL)
   */
  bool eval_jac_g(Ipopt::Index n, const Ipopt::Number* x, bool new_x,
  		Ipopt::Index m, Ipopt::Index nele_jac, Ipopt::Index* iRow, Ipopt::Index *jCol,
  		Ipopt::Number* values);

	/** Compute the value of a single constraint.
	 * @param n the number of variables
	 * @param x the point to evaluate
	 * @param new_x whether x is a new point
	 * @param i the constraint number (starting counting from 0)
	 * @param gi to store the value of g_i at x
	 */
	bool eval_gi(Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Index i, Ipopt::Number& gi);
	
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
	bool eval_grad_gi(Ipopt::Index n, const Ipopt::Number* x, bool new_x,
			Ipopt::Index i, Ipopt::Index& nele_grad_gi, Ipopt::Index* jCol, Ipopt::Number* values);

  /** Method to return:
   *   1) The structure of the hessian of the lagrangian (if "values" is NULL)
   *   2) The values of the hessian of the lagrangian (if "values" is not NULL)
   */
  bool eval_h(Ipopt::Index n, const Ipopt::Number* x, bool new_x,
  		Ipopt::Number obj_factor, Ipopt::Index m, const Ipopt::Number* lambda,
  		bool new_lambda, Ipopt::Index nele_hess, Ipopt::Index* iRow,
  		Ipopt::Index* jCol, Ipopt::Number* values);

	void finalize_solution(Bonmin::TMINLP::SolverReturn status, Ipopt::Index n, const Ipopt::Number* x, Ipopt::Number obj_value);

	/** Provides information about SOS constraints.
	 */
  const Bonmin::TMINLP::SosInfo* sosConstraints() const;
  
	/** Provides information about branching priorities. 
	 */
  const Bonmin::TMINLP::BranchingInfo* branchingInfo() const;
};

#endif /*GAMSMINLP_HPP_*/
