// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Steve Dirkse, Stefan Vigerske

#ifndef GAMSNLP_HPP_
#define GAMSNLP_HPP_

#include "GAMSlinksConfig.h"

#include "IpTNLP.hpp"

struct gmoRec;
class GamsMINLP;

/** A TNLP for Ipopt that uses GMO to interface the problem formulation.
 */
class GamsNLP : public Ipopt::TNLP {
	friend class GamsMINLP;
private:
	struct gmoRec* gmo;

	double timelimit;
	long int domviollimit;
	
	int* iRowStart;
	int* jCol;	
	double* grad;

public:
	double div_iter_tol;
	double scaled_conviol_tol;
	double unscaled_conviol_tol;
  double clockStart;
	long int domviolations;

	GamsNLP (struct gmoRec* gmo);

  ~GamsNLP();

  /** Method to return some info about the nlp */
  bool get_nlp_info(Ipopt::Index& n, Ipopt::Index& m, Ipopt::Index& nnz_jac_g,
  		Ipopt::Index& nnz_h_lag, Ipopt::TNLP::IndexStyleEnum& Index_style);

  /** Method to return the bounds for my problem */
  bool get_bounds_info(Ipopt::Index n, Ipopt::Number* x_l, Ipopt::Number* x_u,
  		Ipopt::Index m, Ipopt::Number* g_l, Ipopt::Number* g_u);

  /** Method to return the starting point for the algorithm */
  bool get_starting_point(Ipopt::Index n, bool init_x, Ipopt::Number* x,
  		bool init_z, Ipopt::Number* z_L, Ipopt::Number* z_U,
  		Ipopt::Index m, bool init_lambda,
  		Ipopt::Number* lambda);
  
  /** Method to return the variables linearity. */
  bool get_variables_linearity(Ipopt::Index n, LinearityType* var_types);

  /** Method to return the constraint linearity. */
  bool get_constraints_linearity(Ipopt::Index m, LinearityType* const_types);

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

  /** Method to return:
   *   1) The structure of the hessian of the lagrangian (if "values" is NULL)
   *   2) The values of the hessian of the lagrangian (if "values" is not NULL)
   */
  bool eval_h(Ipopt::Index n, const Ipopt::Number* x, bool new_x,
  		Ipopt::Number obj_factor, Ipopt::Index m, const Ipopt::Number* lambda,
  		bool new_lambda, Ipopt::Index nele_hess, Ipopt::Index* iRow,
  		Ipopt::Index* jCol, Ipopt::Number* values);

	/** Method called by the solver at each iteration.
	 * Checks the domain violation limit and time limit and stops the solver in case of limit exceedance.
	 */
	bool intermediate_callback(Ipopt::AlgorithmMode mode, Ipopt::Index iter, Ipopt::Number obj_value, Ipopt::Number inf_pr, Ipopt::Number inf_du, Ipopt::Number mu, Ipopt::Number d_norm, Ipopt::Number regularization_size, Ipopt::Number alpha_du, Ipopt::Number alpha_pr, Ipopt::Index ls_trials, const Ipopt::IpoptData *ip_data, Ipopt::IpoptCalculatedQuantities *ip_cq);

  /** This method is called when the algorithm is complete so the TNLP can store/write the solution.
   */
  void finalize_solution(Ipopt::SolverReturn status,
  		Ipopt::Index n, const Ipopt::Number* x, const Ipopt::Number* z_L,
  		const Ipopt::Number* z_U,
  		Ipopt::Index m, const Ipopt::Number* g, const Ipopt::Number* lambda,
  		Ipopt::Number obj_value,
  		const Ipopt::IpoptData* data, Ipopt::IpoptCalculatedQuantities* cq);
};


#endif /*GAMSNLP_HPP_*/
