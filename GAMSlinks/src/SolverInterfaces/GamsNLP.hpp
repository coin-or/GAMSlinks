// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors: Steven Dirkse, Stefan Vigerske

#ifndef GAMSNLP_HPP_
#define GAMSNLP_HPP_

#include "IpTNLP.hpp"

struct gmoRec;
struct gevRec;
class GamsMINLP;

/** a TNLP for Ipopt that uses GMO to interface the problem formulation */
class GamsNLP : public Ipopt::TNLP
{
   friend class GamsMINLP;

private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */

   long int              domviollimit;       /**< domain violations limit */

   int*                  iRowStart;          /**< row starts in jacobian */
   int*                  jCol;               /**< column indicies in jacobian */
   double*               grad;               /**< working memory for storing gradient values */

   long int              nevalobj;           /**< number of objective function evaluations */
   long int              nevalobjgrad;       /**< number of objective gradient evaluations */
   long int              nevalcons;          /**< number of constraint functions evaluations */
   long int              nevalconsjac;       /**< number of jacobian evaluations */
   long int              nevallaghess;       /**< number of lagrangian hessian evaluations */
   long int              nevalnewpoint;      /**< number of evaluations at new points */

public:
   double                div_iter_tol;       /**< value above which divergence is claimed */
   double                scaled_conviol_tol; /**< scaled constraint violation tolerance */
   double                unscaled_conviol_tol; /**< unscaled constraint violation tolerance */
   double                clockStart;         /**< time when optimization started */
   long int              domviolations;      /**< number of domain violations */

   GamsNLP(
      struct gmoRec*     gmo_                /**< GAMS modeling object */
   );

   ~GamsNLP();

   /** resets counter on function/gradient/hessian evaluations */
   void reset_eval_counter();

   /** gives number of objective function evaluations */
   long get_numeval_obj()      const { return nevalobj; }

   /** gives number of objective gradient evaluations */
   long get_numeval_objgrad()  const { return nevalobjgrad; }

   /* gives number of constraint evaluations */
   long get_numeval_cons()     const { return nevalcons; }

   /* gives number of jacobian evaluations */
   long get_numeval_consjac()  const { return nevalconsjac; }

   /* gives number of lagrangian hessian evaluations */
   long get_numeval_laghess()  const { return nevallaghess; }

   /* gives number of evaluations at new points */
   long get_numeval_newpoint() const { return nevalnewpoint; }

   bool get_nlp_info(
      Ipopt::Index&      n,
      Ipopt::Index&      m,
      Ipopt::Index&      nnz_jac_g,
      Ipopt::Index&      nnz_h_lag,
      Ipopt::TNLP::IndexStyleEnum& Index_style
   );

   bool get_bounds_info(
      Ipopt::Index       n,
      Ipopt::Number*     x_l,
      Ipopt::Number*     x_u,
      Ipopt::Index       m,
      Ipopt::Number*     g_l,
      Ipopt::Number*     g_u
   );

   bool get_starting_point(
      Ipopt::Index       n,
      bool               init_x,
      Ipopt::Number*     x,
      bool               init_z,
      Ipopt::Number*     z_L,
      Ipopt::Number*     z_U,
      Ipopt::Index       m,
      bool               init_lambda,
      Ipopt::Number*     lambda
   );

   bool get_variables_linearity(
      Ipopt::Index       n,
      LinearityType*     var_types
   );

   bool get_constraints_linearity(
      Ipopt::Index       m,
      LinearityType*     const_types
   );

   bool get_var_con_metadata(
      Ipopt::Index       n,
      StringMetaDataMapType&  var_string_md,
      IntegerMetaDataMapType& var_integer_md,
      NumericMetaDataMapType& var_numeric_md,
      Ipopt::Index       m,
      StringMetaDataMapType&  con_string_md,
      IntegerMetaDataMapType& con_integer_md,
      NumericMetaDataMapType& con_numeric_md
   );

   Ipopt::Index get_number_of_nonlinear_variables();

   bool get_list_of_nonlinear_variables(
      Ipopt::Index       num_nonlin_vars,
      Ipopt::Index*      pos_nonlin_vars
   );

   bool eval_f(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Number&     obj_value
   );

   bool eval_grad_f(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Number*     grad_f
   );

   bool eval_g(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Index       m,
      Ipopt::Number*     g
   );

   bool eval_jac_g(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Index       m,
      Ipopt::Index       nele_jac,
      Ipopt::Index*      iRow,
      Ipopt::Index*      jCol,
      Ipopt::Number*     values
   );

   bool eval_h(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Number      obj_factor,
      Ipopt::Index       m,
      const Ipopt::Number* lambda,
      bool               new_lambda,
      Ipopt::Index       nele_hess,
      Ipopt::Index*      iRow,
      Ipopt::Index*      jCol,
      Ipopt::Number*     values
   );

   bool intermediate_callback(
      Ipopt::AlgorithmMode mode,
      Ipopt::Index       iter,
      Ipopt::Number      obj_value,
      Ipopt::Number      inf_pr,
      Ipopt::Number      inf_du,
      Ipopt::Number      mu,
      Ipopt::Number      d_norm,
      Ipopt::Number      regularization_size,
      Ipopt::Number      alpha_du,
      Ipopt::Number      alpha_pr,
      Ipopt::Index       ls_trials,
      const Ipopt::IpoptData* ip_data,
      Ipopt::IpoptCalculatedQuantities* ip_cq
   );

   void finalize_solution(
      Ipopt::SolverReturn status,
      Ipopt::Index       n,
      const Ipopt::Number* x,
      const Ipopt::Number* z_L,
      const Ipopt::Number* z_U,
      Ipopt::Index       m,
      const Ipopt::Number* g,
      const Ipopt::Number* lambda,
      Ipopt::Number      obj_value,
      const Ipopt::IpoptData* data,
      Ipopt::IpoptCalculatedQuantities* cq
   );
};

#endif
