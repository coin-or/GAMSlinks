// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors: Steven Dirkse, Stefan Vigerske

#ifndef GAMSNLP_HPP_
#define GAMSNLP_HPP_

#include "IpTNLP.hpp"

struct gmoRec;
struct gevRec;

/** a TNLP for Ipopt that uses GMO to interface the problem formulation */
class DllExport GamsNLP : public Ipopt::TNLP
{
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */

   long int              domviollimit;       /**< domain violations limit */
   long int              domviolations;      /**< number of domain violations */

   int*                  iRowStart;          /**< row starts in Jacobian */
   int*                  jCol;               /**< column indices in Jacobian */
   double*               grad;               /**< working memory for storing gradient values */

   int                   mininfeasiter;      /**< iteration number of minimal infeasible solution */
   double                mininfeasconviol;   /**< constraint violation in minimal infeasible solution */
   double*               mininfeasprimals;   /**< primal values in minimal infeasible solution */
   double*               mininfeasactivity;  /**< constraint activity (level values) in minimal infeasible solution */
   double*               mininfeasviol;      /**< constraint violation in minimal infeasible solution */
   double*               mininfeasdualeqs;   /**< dual values for equations in minimal infeasible solution */
   double*               mininfeasduallbs;   /**< dual values for variable lower bounds in minimal infeasible solution */
   double*               mininfeasdualubs;   /**< dual values for variable lower bounds in minimal infeasible solution */
   double*               mininfeascomplxlb;  /**< complementarity in variable lower bounds in minimal infeasible solution */
   double*               mininfeascomplxub;  /**< complementarity in variable upper bounds in minimal infeasible solution */
   double*               mininfeascomplg;    /**< complementarity in constraints in minimal infeasible solution */

public:
   double                div_iter_tol;       /**< value above which divergence is claimed */
   double                conviol_tol;        /**< constraint violation tolerance */
   double                compl_tol;          /**< complementarity tolerance */
   bool                  reportmininfeas;    /**< should an intermediate solution with minimal primal infeasibility be reported if final solution is not feasible? */

   GamsNLP(
      struct gmoRec*     gmo_                /**< GAMS modeling object */
   );

   ~GamsNLP();

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
