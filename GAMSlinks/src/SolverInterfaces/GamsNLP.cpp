// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors: Steven Dirkse, Stefan Vigerske

#include "GamsNLP.hpp"
#include "GAMSlinksConfig.h"

#include "IpIpoptCalculatedQuantities.hpp"
#include "IpIpoptData.hpp"
#include "IpTNLPAdapter.hpp"
#include "IpOrigIpoptNLP.hpp"

#include <cstring> // for memset
#include <cmath>   // for fabs
#include <cstdio>  // for sprintf
#include <cassert>

#include "gmomcc.h"
#include "gevmcc.h"

#include "GamsCompatibility.h"

using namespace Ipopt;

GamsNLP::GamsNLP(
   struct gmoRec*     gmo_                /**< GAMS modeling object */
)
: iRowStart(NULL),
  jCol(NULL),
  grad(NULL),
  mininfeasconviolsc(1E+20),
  mininfeasconviolunsc(1E+20),
  mininfeasprimals(NULL),
  mininfeasdualeqs(NULL),
  mininfeasduallbs(NULL),
  mininfeasdualubs(NULL),
  mininfeasscaledviol(NULL),
  mininfeascomplxlb(NULL),
  mininfeascomplxub(NULL),
  mininfeascomplglb(NULL),
  mininfeascomplgub(NULL),
  div_iter_tol(1E+20),
  scaled_conviol_tol(1E-8),
  unscaled_conviol_tol(1E-4),
  reportmininfeas(false),
  domviolations(0)
{
   gmo = gmo_;
   assert(gmo != NULL);
   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev);

   domviollimit = gevGetIntOpt(gev, gevDomLim);

   /* stop fast in case of eval errors, since we do not look into values anyway */
   gmoEvalErrorMethodSet(gmo, gmoEVALERRORMETHOD_FASTSTOP);

   reset_eval_counter();
}

GamsNLP::~GamsNLP()
{
   delete[] iRowStart;
   delete[] jCol;
   delete[] grad;
   delete[] mininfeasprimals;
   delete[] mininfeasdualeqs;
   delete[] mininfeasduallbs;
   delete[] mininfeasdualubs;
   delete[] mininfeasscaledviol;
   delete[] mininfeascomplxlb;
   delete[] mininfeascomplxub;
   delete[] mininfeascomplglb;
   delete[] mininfeascomplgub;
}

void GamsNLP::reset_eval_counter()
{
   nevalobj      = 0;
   nevalobjgrad  = 0;
   nevalcons     = 0;
   nevalconsjac  = 0;
   nevallaghess  = 0;
   nevalnewpoint = 0;
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

bool GamsNLP::get_variables_linearity(
   Ipopt::Index       n,
   LinearityType*     var_types
)
{
   assert(n == gmoN(gmo));

   if( gmoNLNZ(gmo) == 0 && gmoObjNLNZ(gmo) == 0 )
   {
      // problem is linear
      if( LINEAR == 0 )
         memset(var_types, LINEAR, n*sizeof(LinearityType));
      else
         for( Index i = 0; i < n; ++i )
            var_types[i] = LINEAR;
      return true;
   }

   int jnz, jqnz, jnlnz, jobjnz;
   for( int i = 0; i < n; ++i )
   {
      gmoGetColStat(gmo, i, &jnz, &jqnz, &jnlnz, &jobjnz);
      if( jnlnz > 0 || (jobjnz == 1)) // jobjnz is -1 if linear in obj, +1 if nonlinear in obj, and 0 if not there
         var_types[i] = NON_LINEAR;
      else
         var_types[i] = LINEAR;
   }

   return true;
}

bool GamsNLP::get_constraints_linearity(
   Ipopt::Index       m,
   LinearityType*     const_types
)
{
   assert(m == gmoM(gmo));

   if( gmoNLNZ(gmo) == 0 )
   {
      // all constraints are linear
      if( LINEAR == 0 )
         memset(const_types, LINEAR, m*sizeof(LinearityType));
      else
         for( Index i = 0; i < m; ++i )
            const_types[i] = LINEAR;
      return true;
   }

   for( Index i = 0; i < m; ++i )
      const_types[i] = gmoGetEquOrderOne(gmo, i) > gmoorder_L ? NON_LINEAR : LINEAR;

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

   std::vector<std::string>& varnames(var_string_md["idx_names"]);
   varnames.reserve(n);
   for( Index i = 0; i < n; ++i )
   {
      gmoGetVarNameOne(gmo, i, buffer);
      varnames.push_back(buffer);
   }

   std::vector<std::string>& connames(con_string_md["idx_names"]);
   connames.reserve(m);
   for( Index i = 0; i < m; ++i)
   {
      gmoGetEquNameOne(gmo, i, buffer);
      connames.push_back(buffer);
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
      ++nevalnewpoint;
   }

   int nerror;
   int rc;
   rc = gmoEvalFuncObj(gmo, x, &obj_value, &nerror);

   if( rc != 0 )
   {
      char buffer[255];
      sprintf(buffer, "Critical error %d detected in evaluation of objective function!\n"
         "Exiting from subroutine - eval_f\n", rc);
      gevLogStatPChar(gev, buffer);
      throw -1;
   }
   if( nerror > 0 )
   {
      ++domviolations;
      return false;
   }

   ++nevalobj;

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
      ++nevalnewpoint;
   }

   memset(grad_f, 0, n*sizeof(double));
   double val;
   double gx;
   int nerror;
   int rc;
   rc = gmoEvalGradObj(gmo, x, &val, grad_f, &gx, &nerror);

   if( rc != 0 )
   {
      char buffer[255];
      sprintf(buffer, "Critical error %d detected in evaluation of objective gradient!\n"
         "Exiting from subroutine - eval_grad_f\n", rc);
      gevLogStatPChar(gev, buffer);
      throw -1;
   }
   if( nerror > 0 )
   {
      ++domviolations;
      return false;
   }

   ++nevalobjgrad;

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
      ++nevalnewpoint;
   }

   int nerror, rc;
   for( int i = 0; i < m; ++i )
   {
      rc = gmoEvalFunc(gmo, i, x, &g[i], &nerror);
      if( rc != 0 )
      {
         char buffer[255];
         sprintf(buffer, "Critical error %d detected in evaluation of constraint %d!\n"
            "Exiting from subroutine - eval_g\n", rc, i);
         gevLogStatPChar(gev, buffer);
         throw -1;
      }
      if( nerror > 0 )
      {
         ++domviolations;
         return false;
      }
   }

   ++nevalcons;

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
      // assemble structure of jacobian
      assert(NULL != iRow);
      assert(NULL != jCol);

      delete[] iRowStart;
      delete[] this->jCol;
      iRowStart      = new int[m+1];
      this->jCol     = new int[nele_jac];
      double* jacval = new double[nele_jac];
      int*    nlflag = new int[nele_jac];
      gmoGetMatrixRow(gmo, iRowStart, this->jCol, jacval, nlflag);

      assert(iRowStart[m] == nele_jac);
      for( Index i = 0; i < m; ++i )
         for( int j = iRowStart[i]; j < iRowStart[i+1]; ++j )
            iRow[j] = i;
      memcpy(jCol, this->jCol, nele_jac * sizeof(int));

      delete[] jacval;
      delete[] nlflag;

      delete[] grad;
      grad = new double[n];
   }
   else
   {
      // compute values of jacobian
      assert(NULL != x);
      assert(NULL == iRow);
      assert(NULL == jCol);
      assert(NULL != iRowStart);
      assert(NULL != this->jCol);
      assert(NULL != grad);

      if( new_x )
      {
         ++nevalnewpoint;
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
            sprintf(buffer, "Critical error %d detected in evaluation of gradient for constraint %d!\n"
               "Exiting from subroutine - eval_jac_g\n", rc, rownr);
            gevLogStatPChar(gev, buffer);
            throw -1;
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

      ++nevalconsjac;
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
         ++nevalnewpoint;
         gmoEvalNewPoint(gmo, x);
      }

      // for GAMS, lambda would need to be multiplied by -1, we do this via the constraint weight
      int nerror;
      int rc;
      rc = gmoHessLagValue(gmo, const_cast<double*>(x), const_cast<double*>(lambda), values, obj_factor, -1.0, &nerror);
      if( rc != 0 )
      {
         char buffer[256];
         sprintf(buffer, "Critical error detected %d in evaluation of Hessian!\n"
            "Exiting from subroutine - eval_h\n", rc);
         gevLogStatPChar(gev, buffer);
         throw -1;
      }
      if( nerror > 0 )
      {
         ++domviolations;
         return false;
      }

      ++nevallaghess;
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

   /* store current solution for later use, if we should report solution with minimal infeasibility and current point has smaller scaled constraint violation than previously seen */
   if( reportmininfeas && mode == RegularMode && ip_cq->curr_nlp_constraint_violation(NORM_MAX) < mininfeasconviolsc )
   {
      Ipopt::TNLPAdapter* tnlp_adapter;

      tnlp_adapter = NULL;
      if( ip_cq != NULL )
      {
         Ipopt::OrigIpoptNLP* orignlp;

         orignlp = dynamic_cast<OrigIpoptNLP*>(GetRawPtr(ip_cq->GetIpoptNLP()));
         if( orignlp != NULL )
            tnlp_adapter = dynamic_cast<TNLPAdapter*>(GetRawPtr(orignlp->nlp()));
      }

      if( tnlp_adapter != NULL && ip_data != NULL && IsValid(ip_data->curr()) )
      {
         if( mininfeasprimals == NULL )
         {
            assert(mininfeasdualeqs == NULL);
            assert(mininfeasduallbs == NULL);
            assert(mininfeasdualubs == NULL);
            assert(mininfeasscaledviol == NULL);
            assert(mininfeascomplxlb == NULL);
            assert(mininfeascomplxub == NULL);
            assert(mininfeascomplglb == NULL);
            assert(mininfeascomplgub == NULL);

            mininfeasprimals = new double[gmoN(gmo)];
            mininfeasdualeqs = new double[gmoM(gmo)];
            mininfeasduallbs = new double[gmoN(gmo)];
            mininfeasdualubs = new double[gmoN(gmo)];
            mininfeasscaledviol = new double[gmoM(gmo)];
            mininfeascomplxlb = new double[gmoN(gmo)];
            mininfeascomplxub = new double[gmoN(gmo)];
            mininfeascomplglb = new double[gmoM(gmo)];
            mininfeascomplgub = new double[gmoM(gmo)];
         }

         mininfeasiter = iter;
         mininfeasconviolsc = ip_cq->curr_nlp_constraint_violation(NORM_MAX);
         mininfeasconviolunsc = ip_cq->unscaled_curr_nlp_constraint_violation(NORM_MAX);

         assert(IsValid(ip_data->curr()->x()));
         tnlp_adapter->ResortX(*ip_data->curr()->x(), mininfeasprimals);

         assert(IsValid(ip_data->curr()->y_c()));
         assert(IsValid(ip_data->curr()->y_d()));
         tnlp_adapter->ResortG(*ip_data->curr()->y_c(), *ip_data->curr()->y_d(), mininfeasdualeqs);

         // need to clear arrays first because ResortBnds only sets values for non-fixed variables
         memset(mininfeasduallbs, 0, gmoN(gmo)*sizeof(double));
         memset(mininfeasdualubs, 0, gmoN(gmo)*sizeof(double));
         assert(IsValid(ip_data->curr()->z_L()));
         assert(IsValid(ip_data->curr()->z_U()));
         tnlp_adapter->ResortBnds(*ip_data->curr()->z_L(), mininfeasduallbs, *ip_data->curr()->z_U(), mininfeasdualubs);

         tnlp_adapter->ResortG(*ip_cq->curr_c(), *ip_cq->curr_d_minus_s(), mininfeasscaledviol);

         SmartPtr<Vector> dummy = ip_cq->curr_c()->MakeNew();
         dummy->Set(0.0);

         // need to clear arrays first because ResortBnds only sets values for non-fixed variables
         memset(mininfeascomplxlb, 0, gmoN(gmo)*sizeof(double));
         memset(mininfeascomplxub, 0, gmoN(gmo)*sizeof(double));
         memset(mininfeascomplglb, 0, gmoM(gmo)*sizeof(double));
         memset(mininfeascomplgub, 0, gmoM(gmo)*sizeof(double));

         if( ip_cq->curr_compl_x_L()->Dim() > 0 && ip_cq->curr_compl_x_U()->Dim() > 0 )
            tnlp_adapter->ResortBnds(*ip_cq->curr_compl_x_L(), mininfeascomplxlb, *ip_cq->curr_compl_x_U(), mininfeascomplxub);
         if( ip_cq->curr_compl_s_L()->Dim() > 0 )
            tnlp_adapter->ResortG(*dummy, *ip_cq->curr_compl_s_L(), mininfeascomplglb);
         if( ip_cq->curr_compl_s_U()->Dim() > 0 )
            tnlp_adapter->ResortG(*dummy, *ip_cq->curr_compl_s_U(), mininfeascomplgub);
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
   char buffer[300];
   bool write_solution = false;

   assert(n == gmoN(gmo));
   assert(m == gmoM(gmo));

   switch( status )
   {
      case SUCCESS:
      case STOP_AT_ACCEPTABLE_POINT:
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
         gevLogStat(gev, "Diverging iterates: we'll guess unbounded!");
         gmoModelStatSet(gmo, gmoModelStat_Unbounded);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         write_solution = true;
         break;

      case USER_REQUESTED_STOP:
      case STOP_AT_TINY_STEP:
      case RESTORATION_FAILURE:
      case MAXITER_EXCEEDED:
      case CPUTIME_EXCEEDED:
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
            case STOP_AT_TINY_STEP:
            case RESTORATION_FAILURE:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver); // terminated by solver (normal completion not allowed by GAMS philosophy here: its not normal when it stops with an intermediate point)
               break;
            case MAXITER_EXCEEDED:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               break;
            case CPUTIME_EXCEEDED:
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               break;
            default: ;
         }
         /* decide on model status: check if current point is feasible */
         if( cq->curr_nlp_constraint_violation(NORM_MAX) > scaled_conviol_tol )
         {
            snprintf(buffer, sizeof(buffer), "Final point is not feasible: scaled constraint violation (%g) is larger than max(tol,acceptable_tol) (%g).", cq->curr_nlp_constraint_violation(NORM_MAX), scaled_conviol_tol);
            gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         }
         else if( cq->unscaled_curr_nlp_constraint_violation(NORM_MAX) > unscaled_conviol_tol )
         {
            snprintf(buffer, sizeof(buffer), "Final point is not feasible: unscaled constraint violation (%g) is larger than max(constr_viol_tol,acceptable_constr_viol_tol) (%g).", cq->unscaled_curr_nlp_constraint_violation(NORM_MAX), unscaled_conviol_tol);
            gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         }
         else
         {
            snprintf(buffer, sizeof(buffer), "Final point is feasible: scaled constraint violation (%g) is below max(tol,acceptable_tol) (%g) and unscaled constraint violation (%g) is below max(constr_viol_tol,acceptable_constr_viol_tol) (%g).",
               cq->curr_nlp_constraint_violation(NORM_MAX), scaled_conviol_tol, cq->unscaled_curr_nlp_constraint_violation(NORM_MAX), unscaled_conviol_tol);
            gmoModelStatSet(gmo, gmoModelStat_NonOptimalIntermed);
         }
         gevLog(gev, buffer);
         /* in any case, we write the current point (or an intermediate one) */
         write_solution = true;
         break;

      case ERROR_IN_STEP_COMPUTATION:
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
         gmoSolveStatSet(gmo, gmoSolveStat_InternalErr);
         break;

      default:
         sprintf(buffer, "OUCH: unhandled SolverReturn of %d\n", status);
         gevLogStatPChar(gev, buffer);
         gmoModelStatSet(gmo, gmoModelStat_ErrorUnknown);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
   }

   if( data != NULL )
      gmoSetHeadnTail(gmo, gmoHiterused, data->iter_count());
   gmoSetHeadnTail(gmo, gmoHresused, gevTimeDiffStart(gev) - clockStart);
   gmoSetHeadnTail(gmo, gmoHdomused, (double)domviolations);

   if( write_solution )
   {
      double* scaled_viol = NULL;
      double* compl_xL = NULL;
      double* compl_xU = NULL;
      double* compl_gL = NULL;
      double* compl_gU = NULL;

      if( mininfeasprimals != NULL && (gmoModelStat(gmo) == gmoModelStat_InfeasibleIntermed || gmoModelStat(gmo) == gmoModelStat_InfeasibleLocal) && mininfeasiter < data->iter_count() )
      {
         snprintf(buffer, sizeof(buffer), "\nReporting intermediate solution from iteration %d instead (scaled constraint violation = %g, unscaled constraint violation = %g).", mininfeasiter, mininfeasconviolsc, mininfeasconviolunsc);
         gevLog(gev, buffer);

         x = mininfeasprimals;
         z_L = mininfeasduallbs;
         z_U = mininfeasdualubs;
         lambda = mininfeasdualeqs;
         scaled_viol = mininfeasscaledviol;
         compl_xL = mininfeascomplxlb;
         compl_xU = mininfeascomplxub;
         compl_gL = mininfeascomplglb;
         compl_gU = mininfeascomplgub;
         /* recompute constraint activities, a bit dirty */
         eval_g(n, x, true, m, const_cast<double*>(g));

         /* decide on changing model status: check if current point is feasible */
         if( mininfeasconviolsc > scaled_conviol_tol )
         {
            snprintf(buffer, sizeof(buffer), "Intermediate solution is not feasible: scaled constraint violation (%g) is larger than max(tol,acceptable_tol) (%g).", mininfeasconviolsc, scaled_conviol_tol);
            gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         }
         else if( mininfeasconviolunsc > unscaled_conviol_tol )
         {
            snprintf(buffer, sizeof(buffer), "Intermediate solution is not feasible: unscaled constraint violation (%g) is larger than max(constr_viol_tol,acceptable_constr_viol_tol) (%g).", mininfeasconviolunsc, unscaled_conviol_tol);
            gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         }
         else
         {
            snprintf(buffer, sizeof(buffer), "Intermediate solution is feasible: scaled constraint violation (%g) is below max(tol,acceptable_tol) (%g) and unscaled constraint violation (%g) is below max(constr_viol_tol,acceptable_constr_viol_tol) (%g).",
               mininfeasconviolsc, scaled_conviol_tol, mininfeasconviolunsc, unscaled_conviol_tol);
            gmoModelStatSet(gmo, gmoModelStat_NonOptimalIntermed);
         }
         gevLog(gev, buffer);
      }
      else
      {
         //    1. Use IpoptCalculatedQuantities::GetIpoptNLP() to get a pointer to an Ipopt::IpoptNLP.
         //    2. Cast this Ipopt::IpoptNLP to an Ipopt::OrigIpoptNLP
         //    3. Use Ipopt::OrigIpoptNLP::nlp() to get the Ipopt::NLP
         //    4. This Ipopt::NLP is actually the TNLPAdapter
         TNLPAdapter* tnlp_adapter = NULL;
         OrigIpoptNLP* orignlp = NULL;
         if( cq != NULL )
            orignlp = dynamic_cast<OrigIpoptNLP*>(GetRawPtr(cq->GetIpoptNLP()));
         if( orignlp != NULL )
            tnlp_adapter = dynamic_cast<TNLPAdapter*>(GetRawPtr(orignlp->nlp()));

         if( tnlp_adapter != NULL )
         {
            scaled_viol = new double[m];
            tnlp_adapter->ResortG(*cq->curr_c(), *cq->curr_d_minus_s(), scaled_viol);

            SmartPtr<Vector> dummy = cq->curr_c()->MakeNew();
            dummy->Set(0.0);
            compl_xL = new double[n];
            compl_xU = new double[n];
            compl_gL = new double[m];
            compl_gU = new double[m];

            memset(compl_xL, 0, n*sizeof(double));
            memset(compl_xU, 0, n*sizeof(double));
            memset(compl_gL, 0, m*sizeof(double));
            memset(compl_gU, 0, m*sizeof(double));

            if( cq->curr_compl_x_L()->Dim() && cq->curr_compl_x_U()->Dim() )
               tnlp_adapter->ResortBnds(*cq->curr_compl_x_L(), compl_xL, *cq->curr_compl_x_U(), compl_xU);
            if( cq->curr_compl_s_L()->Dim() )
               tnlp_adapter->ResortG(*dummy, *cq->curr_compl_s_L(), compl_gL);
            if( cq->curr_compl_s_U()->Dim() )
               tnlp_adapter->ResortG(*dummy, *cq->curr_compl_s_U(), compl_gU);
         }
      }

      int*    colBasStat = new int[n];
      int*    colIndic   = new int[n];
      double* colMarg    = new double[n];
      for( Index i = 0; i < n; ++i )
      {
         colBasStat[i] = gmoBstat_Super;
         if( gmoGetVarLowerOne(gmo, i) != gmoGetVarUpperOne(gmo, i) && compl_xL != NULL && (fabs(compl_xL[i]) > scaled_conviol_tol || fabs(compl_xU[i]) > scaled_conviol_tol) )
            colIndic[i] = gmoCstat_NonOpt;
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
         rowBasStat[i] = gmoBstat_Super;
         if( scaled_viol != NULL && fabs(scaled_viol[i]) > scaled_conviol_tol )
            rowIndic[i] = gmoCstat_Infeas;
         else if( compl_gL != NULL && (fabs(compl_gL[i]) > scaled_conviol_tol || fabs(compl_gU[i]) > scaled_conviol_tol) )
            rowIndic[i] = gmoCstat_NonOpt;
         else
            rowIndic[i] = gmoCstat_OK;
         negLambda[i] = -lambda[i];
      }

      gmoSetSolution8(gmo, x, colMarg, negLambda, g, colBasStat, colIndic, rowBasStat, rowIndic);
      gmoSetHeadnTail(gmo, gmoHobjval, obj_value);

      if( scaled_viol != mininfeasscaledviol )
      {
         delete[] scaled_viol;
         delete[] compl_xL;
         delete[] compl_xU;
         delete[] compl_gL;
         delete[] compl_gU;
      }
      delete[] colBasStat;
      delete[] colIndic;
      delete[] colMarg;
      delete[] rowBasStat;
      delete[] rowIndic;
      delete[] negLambda;
   }
}
