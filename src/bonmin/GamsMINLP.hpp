// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSMINLP_HPP_
#define GAMSMINLP_HPP_

#include "BonTMINLP.hpp"

class GamsBonmin;
class GamsCouenne;
class GamsCouenneSetup;

struct gmoRec;
struct gevRec;

/** a TMINLP for Bonmin that uses GMO to interface the problem formulation */
class GamsMINLP : public Bonmin::TMINLP
{
   friend class GamsBonmin;
   friend class GamsCouenne;
   friend class GamsCouenneSetup;

private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   double                isMin;              /**< objective sense (1.0 for minimization, -1.0 for maximization */

   long int              domviollimit;       /**< domain violations limit */

   int*                  iRowStart;          /**< row starts in jacobian */
   int*                  jCol;               /**< column indicies in jacobian */
   double*               grad;               /**< working memory for storing gradient values */

   bool                  in_couenne;         /**< whether we use this class in Couenne */

   Bonmin::TMINLP::SosInfo sosinfo;          /**< SOS information */
   bool                  negativesos;        /**< whether we saw variables in SOS with (possibly) negative variables */
   Bonmin::TMINLP::BranchingInfo branchinginfo; /**< Branching information */

   /** initializes sosinfo and branchinginfo */
   void setupPrioritiesSOS();

public:
   double                div_iter_tol;       /**< value above which divergence is claimed */
   double                clockStart;         /**< time when optimization started */
   long int              domviolations;      /**< number of domain violations */
   int                   model_status;       /**< holds GAMS model status when solve finished */
   int                   solver_status;      /**< holds GAMS model status when solve finished */

   GamsMINLP(
      struct gmoRec*     gmo_,               /**< GAMS modeling object */
      bool               in_couenne_ = false /**< whether the MINLP is used within Couenne */
      );

   ~GamsMINLP();

   /** whether we have variables in SOS that can take negative values */
   bool have_negative_sos()
   {
      return negativesos;
   }

   bool get_nlp_info(
      Ipopt::Index&      n,
      Ipopt::Index&      m,
      Ipopt::Index&      nnz_jac_g,
      Ipopt::Index&      nnz_h_lag,
      Ipopt::TNLP::IndexStyleEnum& index_style
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
      Ipopt::TNLP::LinearityType* var_linearity
   );

   bool get_constraints_linearity(
      Ipopt::Index       m,
      Ipopt::TNLP::LinearityType* cons_linearity
   );

   bool get_variables_types(
      Ipopt::Index       n,
      Bonmin::TMINLP::VariableType* var_types
   );

   bool hasLinearObjective();

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

   bool eval_gi(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Index       i,
      Ipopt::Number&     gi
   );

   bool eval_grad_gi(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Index       i,
      Ipopt::Index&      nele_grad_gi,
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

   void finalize_solution(
      Bonmin::TMINLP::SolverReturn status,
      Ipopt::Index       n,
      const Ipopt::Number* x,
      Ipopt::Number      obj_value
   );

   const Bonmin::TMINLP::SosInfo* sosConstraints() const
   {
      return sosinfo.num > 0 ? &sosinfo : NULL;
   }

   const Bonmin::TMINLP::BranchingInfo* branchingInfo() const
   {
      return branchinginfo.size > 0 ? &branchinginfo : NULL;
   }
};

#endif /* GAMSMINLP_HPP_ */
