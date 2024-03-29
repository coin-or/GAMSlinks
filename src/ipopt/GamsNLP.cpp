// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors: Steven Dirkse, Stefan Vigerske

#include "GamsLinksConfig.h"
#include "GamsNLP.hpp"

#include "IpIpoptCalculatedQuantities.hpp"
#include "IpIpoptData.hpp"

#include <cstring> // for memset
#include <cstdio>  // for sprintf
#include <cassert>

#include "gmomcc.h"
#include "gevmcc.h"

using namespace Ipopt;

GamsNLP::GamsNLP(
   struct gmoRec*     gmo_                /**< GAMS modeling object */
)
: domviolations(0),
  iRowStart(NULL),
  jCol(NULL),
  grad(NULL),
  mininfeasiter(-1),
  mininfeasconviol(1E+20),
  mininfeasprimals(NULL),
  mininfeasactivity(NULL),
  mininfeasviol(NULL),
  mininfeasdualeqs(NULL),
  mininfeasduallbs(NULL),
  mininfeasdualubs(NULL),
  mininfeascomplxlb(NULL),
  mininfeascomplxub(NULL),
  mininfeascomplg(NULL),
  div_iter_tol(1E+20),
  conviol_tol(1E-6),
  compl_tol(1E-4),
  reportmininfeas(false)
{
   gmo = gmo_;
   assert(gmo != NULL);
   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev);

   domviollimit = gevGetIntOpt(gev, gevDomLim);

   /* stop fast in case of eval errors, since we do not look into values anyway */
   gmoEvalErrorMethodSet(gmo, gmoEVALERRORMETHOD_FASTSTOP);
}

GamsNLP::~GamsNLP()
{
   delete[] iRowStart;
   delete[] jCol;
   delete[] grad;
   delete[] mininfeasprimals;
   delete[] mininfeasviol;
   delete[] mininfeasactivity;
   delete[] mininfeasdualeqs;
   delete[] mininfeasduallbs;
   delete[] mininfeasdualubs;
   delete[] mininfeascomplxlb;
   delete[] mininfeascomplxub;
   delete[] mininfeascomplg;
}

bool GamsNLP::get_nlp_info(
   Ipopt::Index&      n,
   Ipopt::Index&      m,
   Ipopt::Index&      nnz_jac_g,
   Ipopt::Index&      nnz_h_lag,
   Ipopt::TNLP::IndexStyleEnum& Index_style
)
{
   n           = gmoN(gmo);
   m           = gmoM(gmo);
   nnz_jac_g   = gmoNZ(gmo);
   nnz_h_lag   = gmoHessLagNz(gmo);
   Index_style = C_STYLE;

   return true;
}

bool GamsNLP::get_bounds_info(
   Ipopt::Index       n,
   Ipopt::Number*     x_l,
   Ipopt::Number*     x_u,
   Ipopt::Index       m,
   Ipopt::Number*     g_l,
   Ipopt::Number*     g_u
)
{
   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));

   gmoGetVarLower(gmo, x_l);
   gmoGetVarUpper(gmo, x_u);
   gmoGetRhs(gmo, g_u);

   for( int i = 0; i < m; ++i, ++g_u, ++g_l )
   {
      switch( gmoGetEquTypeOne(gmo, i) )
      {
         case gmoequ_E:	//equality
            *g_l = *g_u;
            break;
         case gmoequ_G: // greater-equal
            *g_l = *g_u;
            *g_u = gmoPinf(gmo);
            break;
         case gmoequ_L:	// lower-equal
            *g_l = gmoMinf(gmo);
            break;
         case gmoequ_N: // free row
            *g_l = gmoMinf(gmo);
            *g_u = gmoPinf(gmo);
            break;
         case gmoequ_C: // conic function
            gevLogStat(gev, "Error: Conic constraints not supported.");
            return false;
            break;
         case gmoequ_B: // logic function
            gevLogStat(gev, "Error: Logic constraints not supported.");
            return false;
            break;
         default:
            gevLogStat(gev, "Error: Unsupported equation type.");
            return false;
      }
   }

   return true;
}

bool GamsNLP::get_starting_point(
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
   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));

   if( init_lambda )
   {
      gmoGetEquM(gmo, lambda);
      for( Index j = m; j; --j, ++lambda )
         *lambda *= -1;
   }

   if( init_z )
   {
      gmoGetVarM(gmo, z_L);
      for( Index j = n; j; --j, ++z_L, ++z_U )
      {
         if( *z_L < 0 )
         {
            *z_U = -*z_L;
            *z_L = 0;
         }
         else
            *z_U = 0;
      }
   }

   if( init_x )
   {
      gmoGetVarL(gmo, x);

      // check that we are not above or below tolerance for diverging iterates
      for( Index j = 0;  j < n;  ++j )
      {
         if( x[j] < -div_iter_tol )
         {
            char buffer[255];
            sprintf(buffer, "Initial value %e for variable %d below diverging iterates tolerance %e. Set initial value to %e.\n", x[j], j, -div_iter_tol, -0.99*div_iter_tol);
            gevLogStatPChar(gev, buffer);
            x[j] = -0.99*div_iter_tol;
         }
         else if( x[j] > div_iter_tol )
         {
            char buffer[255];
            sprintf(buffer, "Initial value %e for variable %d above diverging iterates tolerance %e. Set initial value to %e.\n", x[j], j, div_iter_tol, 0.99*div_iter_tol);
            gevLogStatPChar(gev, buffer);
            x[j] = 0.99*div_iter_tol;
         }
      }
   }

   return true;
}

bool GamsNLP::get_var_con_metadata(
   Ipopt::Index       n,
   StringMetaDataMapType&  var_string_md,
   IntegerMetaDataMapType& var_integer_md,
   NumericMetaDataMapType& var_numeric_md,
   Ipopt::Index       m,
   StringMetaDataMapType&  con_string_md,
   IntegerMetaDataMapType& con_integer_md,
   NumericMetaDataMapType& con_numeric_md
)
{
   if( gmoDict(gmo) == NULL )
      return false;

   char buffer[1024];
   size_t namemem;

   namemem = (n + m) * sizeof(char*);

   std::vector<std::string>& varnames(var_string_md["idx_names"]);
   varnames.reserve(n);
   for( Index i = 0; i < n; ++i )
   {
      gmoGetVarNameOne(gmo, i, buffer);
      varnames.push_back(buffer);
      namemem += strlen(buffer) + 1;
   }

   std::vector<std::string>& connames(con_string_md["idx_names"]);
   connames.reserve(m);
   for( Index i = 0; i < m; ++i)
   {
      gmoGetEquNameOne(gmo, i, buffer);
      connames.push_back(buffer);
      namemem += strlen(buffer) + 1;
   }

   if( (namemem >> 20) > 0 )
   {
      sprintf(buffer, "Space for names approximately %u MB.\nUse statement '<modelname>.dictfile=0;' to turn dictionary off.\n", (unsigned int)(namemem>>20));
      gevLogStatPChar(gev, buffer);
   }

   return true;
}

Ipopt::Index GamsNLP::get_number_of_nonlinear_variables()
{
   if( gmoNLNZ(gmo) == 0 && gmoObjNLNZ(gmo) == 0 )
      return 0; // problem is linear

   int count = 0;
   int jnz, jqnz, jnlnz, jobjnz;
   for( Index i = 0; i < gmoN(gmo); ++i )
   {
      gmoGetColStat(gmo, i, &jnz, &jqnz, &jnlnz, &jobjnz);
      if( jnlnz > 0 || (jobjnz == 1) ) // jobjnz is -1 if linear in obj, +1 if nonlinear in obj, and 0 if not there
         ++count;
   }

   return count;
}

bool GamsNLP::get_list_of_nonlinear_variables(
   Ipopt::Index       num_nonlin_vars,
   Ipopt::Index*      pos_nonlin_vars
)
{
   int count = 0;
   int jnz, jqnz, jnlnz, jobjnz;
   for( int i = 0; i < gmoN(gmo); ++i )
   {
      gmoGetColStat(gmo, i, &jnz, &jqnz, &jnlnz, &jobjnz);
      if( jnlnz > 0 || (jobjnz == 1) ) // jobjnz is -1 if linear in obj, +1 if nonlinear in obj, and 0 if not there
      {
         assert(count < num_nonlin_vars);
         pos_nonlin_vars[count++] = i;
      }
   }
   assert(count == num_nonlin_vars);

   return true;
}

bool GamsNLP::eval_f(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool               new_x,
   Ipopt::Number&     obj_value
)
{
   assert(n == gmoN(gmo));

   if( new_x )
   {
      gmoEvalNewPoint(gmo, x);
   }

   int nerror;
   int rc = gmoEvalFuncObj(gmo, x, &obj_value, &nerror);

   if( rc != 0 )
   {
      char buffer[255];
      sprintf(buffer, "Critical error %d detected in evaluation of objective function!\n", rc);
      throw std::runtime_error(buffer);
   }

   if( nerror > 0 )
   {
      ++domviolations;
      return false;
   }

   return true;
}

bool GamsNLP::eval_grad_f(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool              new_x,
   Ipopt::Number*    grad_f
)
{
   assert(n == gmoN(gmo));

   if( new_x )
   {
      gmoEvalNewPoint(gmo, x);
   }

   memset(grad_f, 0, n*sizeof(double));
   double val;
   double gx;
   int nerror;
   int rc = gmoEvalGradObj(gmo, x, &val, grad_f, &gx, &nerror);

   if( rc != 0 )
   {
      char buffer[255];
      sprintf(buffer, "Critical error %d detected in evaluation of objective gradient!\n", rc);
      throw std::runtime_error(buffer);
   }

   if( nerror > 0 )
   {
      ++domviolations;
      return false;
   }

   return true;
}

bool GamsNLP::eval_g(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool               new_x,
   Ipopt::Index       m,
   Ipopt::Number*     g
)
{
   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));

   if( new_x )
   {
      gmoEvalNewPoint(gmo, x);
   }

   int nerror, rc;
   for( int i = 0; i < m; ++i )
   {
      rc = gmoEvalFunc(gmo, i, x, &g[i], &nerror);
      if( rc != 0 )
      {
         char buffer[255];
         sprintf(buffer, "Critical error %d detected in evaluation of constraint %d!\n", rc, i);
         throw std::runtime_error(buffer);
      }
      if( nerror > 0 )
      {
         ++domviolations;
         return false;
      }
   }

   return true;
}

bool GamsNLP::eval_jac_g(
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
   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));
   assert(nele_jac == gmoNZ(gmo));

   if( values == NULL )
   {
      // assemble structure of Jacobian
      assert(NULL != iRow);
      assert(NULL != jCol);

      delete[] iRowStart;
      delete[] this->jCol;
      iRowStart      = new int[m+1];
      this->jCol     = new int[nele_jac];
      double* jacval = new double[nele_jac];
      
      gmoGetMatrixRow(gmo, iRowStart, this->jCol, jacval, NULL);

      assert(iRowStart[m] == nele_jac);
      for( Index i = 0; i < m; ++i )
         for( int j = iRowStart[i]; j < iRowStart[i+1]; ++j )
            iRow[j] = i;
      memcpy(jCol, this->jCol, nele_jac * sizeof(int));

      delete[] jacval;

      delete[] grad;
      grad = new double[n];
   }
   else
   {
      // compute values of Jacobian
      assert(NULL != x);
      assert(NULL == iRow);
      assert(NULL == jCol);
      assert(NULL != iRowStart);
      assert(NULL != this->jCol);
      assert(NULL != grad);

      if( new_x )
      {
         gmoEvalNewPoint(gmo, x);
      }

      double val;
      double gx;
      int nerror, rc;
      int k;
      int next;

      k = 0;
      for( int rownr = 0; rownr < m; ++rownr )
      {
         rc = gmoEvalGrad(gmo, rownr, x, &val, grad, &gx, &nerror);
         if( rc != 0 )
         {
            char buffer[255];
            sprintf(buffer, "Critical error %d detected in evaluation of gradient for constraint %d!\n", rc, rownr);
            throw std::runtime_error(buffer);
         }
         if( nerror > 0 )
         {
            ++domviolations;
            return false;
         }
         assert(k == iRowStart[rownr]);
         next = iRowStart[rownr+1];
         for( ; k < next; ++k )
            values[k] = grad[this->jCol[k]];
      }
      assert(k == nele_jac);
   }

   return true;
}

bool GamsNLP::eval_h(
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
   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));
   assert(nele_hess == gmoHessLagNz(gmo));

   if( values == NULL )
   {
      // assemble structure of hessian
      // this is a symmetric matrix, thus fill the lower left triangle only
      assert(NULL == x);
      assert(NULL == lambda);
      assert(NULL != iRow);
      assert(NULL != jCol);

      gmoHessLagStruct(gmo, iRow, jCol);
   }
   else
   {
      // compute hessian values
      // this is a symmetric matrix, thus fill the lower left triangle only
      assert(NULL != x);
      assert(NULL != lambda);
      assert(NULL == iRow);
      assert(NULL == jCol);

      if( new_x )
      {
         gmoEvalNewPoint(gmo, x);
      }

      // for GAMS, lambda would need to be multiplied by -1, we do this via the constraint weight
      int nerror;
      int rc;
      rc = gmoHessLagValue(gmo, const_cast<double*>(x), const_cast<double*>(lambda), values, obj_factor, -1.0, &nerror);
      if( rc != 0 )
      {
         char buffer[256];
         sprintf(buffer, "Critical error detected %d in evaluation of Hessian!\n", rc);
         throw std::runtime_error(buffer);
      }
      if( nerror > 0 )
      {
         ++domviolations;
         return false;
      }
   }

   return true;
}

bool GamsNLP::intermediate_callback(
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
)
{
   if( domviollimit > 0 && domviolations >= domviollimit )
      return false;

   if( gevTerminateGet(gev) )
      return false;

   // store current solution for later use, if we should report solution with minimal infeasibility and current point has smaller constraint violation than previously seen
   if( reportmininfeas && mode == RegularMode && ip_cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) < mininfeasconviol )
   {
      if( mininfeasprimals == NULL )
      {
         assert(mininfeasactivity == NULL);
         assert(mininfeasviol == NULL);
         assert(mininfeasdualeqs == NULL);
         assert(mininfeasduallbs == NULL);
         assert(mininfeasdualubs == NULL);
         assert(mininfeascomplxlb == NULL);
         assert(mininfeascomplxub == NULL);
         assert(mininfeascomplg == NULL);

         mininfeasprimals = new double[gmoN(gmo)];
         mininfeasactivity = new double[gmoM(gmo)];
         mininfeasviol = new double[gmoM(gmo)];
         mininfeasdualeqs = new double[gmoM(gmo)];
         mininfeasduallbs = new double[gmoN(gmo)];
         mininfeasdualubs = new double[gmoN(gmo)];
         mininfeascomplxlb = new double[gmoN(gmo)];
         mininfeascomplxub = new double[gmoN(gmo)];
         mininfeascomplg = new double[gmoM(gmo)];
      }

      mininfeasiter = iter;
      mininfeasconviol = ip_cq->unscaled_curr_nlp_constraint_violation(NORM_MAX);
      if( !get_curr_iterate(ip_data, ip_cq, false, gmoN(gmo), mininfeasprimals, mininfeasduallbs, mininfeasdualubs, gmoM(gmo), mininfeasactivity, mininfeasdualeqs) ||
          !get_curr_violations(ip_data, ip_cq, false, gmoN(gmo), NULL, NULL, mininfeascomplxlb, mininfeascomplxub, NULL, gmoM(gmo), mininfeasviol, mininfeascomplg) )
      {
         mininfeasiter = -1;  // forget given point if there was a problem; however, this should not happen in our setup
         mininfeasconviol = 1E+20;
      }
   }

   return true;
}

void GamsNLP::finalize_solution(
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
)
{
   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));

   bool write_solution = false;
   switch( status )
   {
      case SUCCESS:
         gmoModelStatSet(gmo, (gmoObjNLNZ(gmo) || gmoNLNZ(gmo)) ? gmoModelStat_OptimalLocal : gmoModelStat_OptimalGlobal);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         write_solution = true;
         break;

      case LOCAL_INFEASIBILITY:
         gmoModelStatSet(gmo, (gmoObjNLNZ(gmo) || gmoNLNZ(gmo)) ? gmoModelStat_InfeasibleLocal : gmoModelStat_InfeasibleGlobal);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         write_solution = true;
         break;

      case DIVERGING_ITERATES:
         gmoModelStatSet(gmo, gmoModelStat_Unbounded);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         write_solution = true;
         break;

      case FEASIBLE_POINT_FOUND:
         gmoModelStatSet(gmo, gmoModelStat_Feasible);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver); // terminated by solver (the point is feasible for constr_viol_tol, but not w.r.t. tol)
         write_solution = true;
         break;

      case STOP_AT_ACCEPTABLE_POINT:
      case USER_REQUESTED_STOP:
      case STOP_AT_TINY_STEP:
      case RESTORATION_FAILURE:
      case MAXITER_EXCEEDED:
      case CPUTIME_EXCEEDED:
      case WALLTIME_EXCEEDED:
      case ERROR_IN_STEP_COMPUTATION:
         /* decide on solver status */
         switch( status )
         {
            case USER_REQUESTED_STOP:
               if( domviollimit > 0 && domviolations >= domviollimit )
               {
                  gevLogStat(gev, "Domain violation limit exceeded!");
                  gmoSolveStatSet(gmo, gmoSolveStat_EvalError);
               }
               else
               {
                  gmoSolveStatSet(gmo, gmoSolveStat_User);
               }
               break;
            case STOP_AT_ACCEPTABLE_POINT:
            case STOP_AT_TINY_STEP:
            case RESTORATION_FAILURE:
            case ERROR_IN_STEP_COMPUTATION:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver); // terminated by solver (normal completion not allowed by GAMS philosophy here: its not normal when it stops with an intermediate point)
               break;
            case MAXITER_EXCEEDED:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               break;
            case CPUTIME_EXCEEDED:
            case WALLTIME_EXCEEDED:
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               break;
            default: ;
         }
         /* decide on model status: check if current point is feasible */
         if( cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) > conviol_tol )
            gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         else
            gmoModelStatSet(gmo, gmoModelStat_Feasible);
         /* in any case, we write the current point (or an intermediate one) */
         write_solution = true;
         break;

      case TOO_FEW_DEGREES_OF_FREEDOM:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      case INVALID_NUMBER_DETECTED:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         break;

      case INTERNAL_ERROR:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_InternalErr);
         break;

      case OUT_OF_MEMORY:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      default:
         gmoModelStatSet(gmo, gmoModelStat_ErrorUnknown);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
   }

   gmoSetHeadnTail(gmo, gmoHdomused, (double)domviolations);
   domviolations = 0;

   if( !write_solution )
      return;

   double* viol = NULL;
   double* compl_xL = NULL;
   double* compl_xU = NULL;
   double* compl_g = NULL;

   if( mininfeasprimals != NULL && (gmoModelStat(gmo) == gmoModelStat_InfeasibleIntermed || gmoModelStat(gmo) == gmoModelStat_InfeasibleLocal) && mininfeasiter >= 0 && mininfeasiter < data->iter_count() )
   {
      char buffer[300];
      sprintf(buffer, "\nReturning intermediate solution from iteration %d. ", mininfeasiter);
      gevLogPChar(gev, buffer);

      /* decide on changing model status: check if current point is feasible */
      if( mininfeasconviol > conviol_tol )
      {
         sprintf(buffer, "Solution is not feasible: constraint violation (%g) > constr_viol_tol (%g)\n.", mininfeasconviol, conviol_tol);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
      }
      else
      {
         sprintf(buffer, "Solution is feasible: constraint violation (%g) <= constr_viol_tol (%g)\n.", mininfeasconviol, conviol_tol);
         gmoModelStatSet(gmo, gmoModelStat_Feasible);
      }
      gevLogPChar(gev, buffer);

      x = mininfeasprimals;
      g = mininfeasactivity;
      viol = mininfeasviol;
      z_L = mininfeasduallbs;
      z_U = mininfeasdualubs;
      lambda = mininfeasdualeqs;
      compl_xL = mininfeascomplxlb;
      compl_xU = mininfeascomplxub;
      compl_g = mininfeascomplg;

      // do not use current point anymore (in case of warmstart)
      mininfeasiter = -1;
      mininfeasconviol = 1E+20;
   }
   else
   {
      viol = new double[m];
      compl_xL = new double[n];
      compl_xU = new double[n];
      compl_g = new double[m];

      // get detailed violations for column and row status
      if( !get_curr_violations(data, cq, false, n, NULL, NULL, compl_xL, compl_xU, NULL, m, viol, compl_g) )
      {
         // this should never happen
         memset(viol, 0, m*sizeof(double));
         memset(compl_xL, 0, m*sizeof(double));
         memset(compl_xU, 0, m*sizeof(double));
         memset(compl_g, 0, m*sizeof(double));
      }
   }

   int*    colBasStat = new int[n];
   int*    colIndic   = new int[n];
   double* colMarg    = new double[n];
   int ninfeas = 0;
   int nnopt = 0;
   for( Index i = 0; i < n; ++i )
   {
      colBasStat[i] = gmoBstat_Super;
      if( gmoGetVarLowerOne(gmo, i) != gmoGetVarUpperOne(gmo, i) && std::max(std::abs(compl_xL[i]), std::abs(compl_xU[i])) > compl_tol )
      {
         colIndic[i] = gmoCstat_NonOpt;
         ++nnopt;
      }
      else
         colIndic[i] = gmoCstat_OK;

      // if, e.g., x_i has no lower bound, then the dual z_L[i] is -infinity
      colMarg[i] = 0.0;
      if( z_L[i] > gmoMinf(gmo) )
         colMarg[i] += z_L[i];
      if( z_U[i] < gmoPinf(gmo) )
         colMarg[i] -= z_U[i];
   }

   int* rowBasStat   = new int[m];
   int* rowIndic     = new int[m];
   double* negLambda = new double[m];
   for( Index i = 0;  i < m;  i++ )
   {
      if( viol != NULL && viol[i] > conviol_tol )
      {
         rowIndic[i] = gmoCstat_Infeas;
         ++ninfeas;
      }
      else if( std::abs(compl_g[i]) > compl_tol )
      {
         rowIndic[i] = gmoCstat_NonOpt;
         ++nnopt;
      }
      else
         rowIndic[i] = gmoCstat_OK;

      rowBasStat[i] = gmoBstat_Super;
      negLambda[i] = -lambda[i];
   }

   /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
   gmoSetSolution8(gmo, x, colMarg, negLambda, g, colBasStat, colIndic, rowBasStat, rowIndic);

   gmoSetHeadnTail(gmo, gmoHobjval, obj_value);
   gmoSetHeadnTail(gmo, gmoTninf, ninfeas);
   gmoSetHeadnTail(gmo, gmoTnopt, nnopt);

   if( viol != mininfeasviol )
   {
      delete[] viol;
      delete[] compl_xL;
      delete[] compl_xU;
      delete[] compl_g;
   }
   delete[] colBasStat;
   delete[] colIndic;
   delete[] colMarg;
   delete[] rowBasStat;
   delete[] rowIndic;
   delete[] negLambda;
}
