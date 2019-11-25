// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSMINLP_HPP_
#define GAMSMINLP_HPP_

#include "BonTMINLP.hpp"
#include "GamsNLP.hpp"

class GamsBonmin;
class GamsCouenne;
class GamsCouenneSetup;

struct gmoRec;
struct gevRec;
struct palRec;

/** a TMINLP for Bonmin that uses GMO to interface the problem formulation */
class GamsMINLP : public Bonmin::TMINLP
{
   friend class GamsBonmin;
   friend class GamsCouenne;
   friend class GamsCouenneSetup;

private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
#ifdef GAMS_BUILD
   struct palRec*        pal;                /**< GAMS audit and license object */
#endif
   double                isMin;              /**< objective sense (1.0 for minimization, -1.0 for maximization */

   bool                  in_couenne;         /**< whether we use this class in Couenne */

   Bonmin::TMINLP::SosInfo       sosinfo;       /**< SOS information */
   Bonmin::TMINLP::BranchingInfo branchinginfo; /**< Branching information */

   Ipopt::SmartPtr<GamsNLP> nlp;                /**< a GamsNLP used to store the underlying NLP */

   long                  nevalsinglecons;    /**< number of function evaluations for a single constraint */
   long                  nevalsingleconsgrad;/**< number of gradient evaluations for a single constraint */

   /** initializes sosinfo and branchinginfo */
   void setupPrioritiesSOS();

public:
   int                   model_status;       /**< holds GAMS model status when solve finished */
   int                   solver_status;      /**< holds GAMS model status when solve finished */

   GamsMINLP(
      struct gmoRec*     gmo_,               /**< GAMS modeling object */
      bool               in_couenne_ = false /**< whether the MINLP is used within Couenne */
      );

   /** resets counter on function/gradient/hessian evaluations */
   void reset_eval_counter();

   /** gives number of objective function evaluations */
   long get_numeval_obj()            const { return nlp->get_numeval_obj(); }

   /** gives number of objective gradient evaluations */
   long get_numeval_objgrad()        const { return nlp->get_numeval_objgrad(); }

   /* gives number of constraint evaluations */
   long get_numeval_cons()           const { return nlp->get_numeval_cons(); }

   /* gives number of jacobian evaluations */
   long get_numeval_consjac()        const { return nlp->get_numeval_consjac(); }

   /* gives number of lagrangian hessian evaluations */
   long get_numeval_laghess()        const { return nlp->get_numeval_laghess(); }

   /* gives number of evaluations at new points */
   long get_numeval_newpoint()       const { return nlp->get_numeval_newpoint(); }

   /* gives number of single constraint function evaluations */
   long get_numeval_singlecons()     const { return nevalsinglecons; }

   /* gives number of single constraint gradient evaluations */
   long get_numeval_singleconsgrad() const { return nevalsingleconsgrad; }

   bool get_nlp_info(
      Ipopt::Index&      n,
      Ipopt::Index&      m,
      Ipopt::Index&      nnz_jac_g,
      Ipopt::Index&      nnz_h_lag,
      Ipopt::TNLP::IndexStyleEnum& index_style
   )
   {
      return nlp->get_nlp_info(n, m, nnz_jac_g, nnz_h_lag, index_style);
   }

   bool get_bounds_info(
      Ipopt::Index       n,
      Ipopt::Number*     x_l,
      Ipopt::Number*     x_u,
      Ipopt::Index       m,
      Ipopt::Number*     g_l,
      Ipopt::Number*     g_u
   )
   {
      return nlp->get_bounds_info(n, x_l, x_u, m, g_l, g_u);
   }

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
   )
   {
      return nlp->get_starting_point(n, init_x, x, init_z, z_L, z_U, m, init_lambda, lambda);
   }

   bool get_variables_linearity(
      Ipopt::Index       n,
      Ipopt::TNLP::LinearityType* var_linearity
   )
   {
      return nlp->get_variables_linearity(n, var_linearity);
   }

   bool get_constraints_linearity(
      Ipopt::Index       m,
      Ipopt::TNLP::LinearityType* cons_linearity
   )
   {
      return nlp->get_constraints_linearity(m, cons_linearity);
   }

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
   )
   {
      if( !nlp->eval_f(n, x, new_x, obj_value) )
         return false;
      obj_value *= isMin;
      return true;
   }

   bool eval_grad_f(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Number*     grad_f
   )
   {
      if( !nlp->eval_grad_f(n, x, new_x, grad_f) )
         return false;

      if( isMin == -1.0 )
         for( int i = n; i; --i )
            (*grad_f++) *= -1.0;

      return true;
   }

   bool eval_g(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Index       m,
      Ipopt::Number*     g
   )
   {
      return nlp->eval_g(n, x, new_x, m, g);
   }

   bool eval_jac_g(
      Ipopt::Index       n,
      const Ipopt::Number* x,
      bool               new_x,
      Ipopt::Index       m,
      Ipopt::Index       nele_jac,
      Ipopt::Index*      iRow,
      Ipopt::Index*      jCol,
      Ipopt::Number*     values
   )
   {
      return nlp->eval_jac_g(n, x, new_x, m, nele_jac, iRow, jCol, values);
   }

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
   )
   {
      return nlp->eval_h(n, x, new_x, isMin*obj_factor, m, lambda, new_lambda, nele_hess, iRow, jCol, values);
   }

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
