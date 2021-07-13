// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors: Stefan Vigerske

#include "GamsLinksConfig.h"
#include "GamsMINLP.hpp"
#include "BonminConfig.h"

#include <cstring> // for memcpy

#include "gmomcc.h"
#include "gevmcc.h"

using namespace Ipopt;
using namespace Bonmin;

GamsMINLP::GamsMINLP(
   struct gmoRec*     gmo_,               /**< GAMS modeling object */
   bool               in_couenne_         /**< whether the MINLP is used within Couenne */
)
: gmo(gmo_),
  gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL),
  iRowStart(NULL),
  jCol(NULL),
  grad(NULL),
  in_couenne(in_couenne_),
  negativesos(false),
  domviolations(0),
  model_status(gmoModelStat_ErrorNoSolution),
  solver_status(gmoSolveStat_SetupErr)
{
   assert(gmo != NULL);

   isMin = (gmoSense(gmo) == gmoObj_Min ? 1.0 : -1.0);

   setupPrioritiesSOS();
}

GamsMINLP::~GamsMINLP()
{
   delete[] iRowStart;
   delete[] jCol;
   delete[] grad;
}

void GamsMINLP::setupPrioritiesSOS()
{
   // range of priority values
   double minprior = gmoMinf(gmo);
   double maxprior = gmoPinf(gmo);

   // take care of integer variables branching priorities
   if( gmoPriorOpt(gmo) )
   {
      // first check which range of priorities is given
      for( int i = 0; i < gmoN(gmo); ++i )
      {
         if( gmoGetVarTypeOne(gmo, i) == gmovar_X )
            continue; // GAMS does not allow branching priorities for continuous variables
         if( gmoGetVarPriorOne(gmo,i) < minprior )
            minprior = gmoGetVarPriorOne(gmo,i);
         if( gmoGetVarPriorOne(gmo,i) > maxprior )
            maxprior = gmoGetVarPriorOne(gmo,i);
      }

      if( minprior != maxprior )
      {
         branchinginfo.size = gmoN(gmo);
         branchinginfo.priorities = new int[branchinginfo.size];
         for( int i = 0; i < branchinginfo.size; ++i )
         {
            if( gmoGetVarTypeOne(gmo, i) == gmovar_X )
               branchinginfo.priorities[i] = 0;
            else
               // we map gams priorities into the range {1,..,1000}
               // CBC: 1000 is standard priority and 1 is highest priority
               // GAMS: 1 is standard priority for discrete variables, and as smaller the value as higher the priority
               branchinginfo.priorities[i] = 1+(int)(999*(gmoGetVarPriorOne(gmo,i)-minprior) / (maxprior-minprior));
         }
      }
   }

   // Tell solver which variables belong to SOS of type 1 or 2
   int numSos1, numSos2, nzSos, numSos;
   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
   numSos = numSos1 + numSos2;

   sosinfo.num   = numSos; // number of sos
   sosinfo.numNz = nzSos;  // number of variables in sos
   if( sosinfo.num == 0 || sosinfo.numNz == 0 )
   {
      sosinfo.num   = 0;
      sosinfo.numNz = 0;
      return;
   }

   sosinfo.indices    = new int[sosinfo.numNz];
   sosinfo.starts     = new int[sosinfo.num+1];
   sosinfo.weights    = new double[sosinfo.numNz];
   sosinfo.priorities = new int[sosinfo.num];
   sosinfo.types      = new char[sosinfo.num];
   int* sostype       = new int[numSos];

   gmoGetSosConstraints(gmo, sostype, sosinfo.starts, sosinfo.indices, sosinfo.weights);

   // check whether some var in a SOS has a negative lower bound
   // Bonmin doesn't handle this well and so we want to reject such problems
   for( int i = 0; i < numSos; ++i )
   {
      sosinfo.types[i] = (char)sostype[i];
      sosinfo.priorities[i] = gmoN(gmo) - (sosinfo.starts[i+1] - sosinfo.starts[i]); // branch on long sets first

      for( int j = sosinfo.starts[i]; j < sosinfo.starts[i+1]; ++j )
      {
         if( gmoGetVarLowerOne(gmo, sosinfo.indices[j]) < 0.0 )
         {
            char buffer[GMS_SSSIZE+100];
            char varname[GMS_SSSIZE];
            if( gmoDict(gmo) )
               gmoGetVarNameOne(gmo, sosinfo.indices[j], varname);
            else
               sprintf(varname, "x%d", sosinfo.indices[j]);
            sprintf(buffer, "Variable %s in SOS has negative lower bound %g\n", varname, gmoGetVarLowerOne(gmo, sosinfo.indices[j]));
            gevLogPChar(gev, buffer);
            negativesos = true;
         }
      }
   }

   delete[] sostype;
}

bool GamsMINLP::get_nlp_info(
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
   Index_style = Ipopt::TNLP::C_STYLE;

   return true;
}

bool GamsMINLP::get_bounds_info(
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
         case gmoequ_E: //equality
            *g_l = *g_u;
            break;
         case gmoequ_G: // greater-equal
            *g_l = *g_u;
            *g_u = gmoPinf(gmo);
            break;
         case gmoequ_L: // lower-equal
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

bool GamsMINLP::get_starting_point(
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

bool GamsMINLP::get_variables_linearity(
   Ipopt::Index                 n,
   Ipopt::TNLP::LinearityType*  var_types
)
{
   assert(n == gmoN(gmo));

   if( gmoNLNZ(gmo) == 0 && gmoObjNLNZ(gmo) == 0 )
   {
      // problem is linear
      if( Ipopt::TNLP::LINEAR == 0 )
         memset(var_types, Ipopt::TNLP::LINEAR, n*sizeof(Ipopt::TNLP::LinearityType));
      else
         for( Index i = 0; i < n; ++i )
            var_types[i] = Ipopt::TNLP::LINEAR;
      return true;
   }

   int jnz, jqnz, jnlnz, jobjnz;
   for( int i = 0; i < n; ++i )
   {
      gmoGetColStat(gmo, i, &jnz, &jqnz, &jnlnz, &jobjnz);
      if( jnlnz > 0 || (jobjnz == 1)) // jobjnz is -1 if linear in obj, +1 if nonlinear in obj, and 0 if not there
         var_types[i] = Ipopt::TNLP::NON_LINEAR;
      else
         var_types[i] = Ipopt::TNLP::LINEAR;
   }

   return true;
}

bool GamsMINLP::get_constraints_linearity(
   Ipopt::Index                 m,
   Ipopt::TNLP::LinearityType*  const_types
)
{
   assert(m == gmoM(gmo));

   if( gmoNLNZ(gmo) == 0 )
   {
      // all constraints are linear
      if( Ipopt::TNLP::LINEAR == 0 )
         memset(const_types, Ipopt::TNLP::LINEAR, m*sizeof(Ipopt::TNLP::LinearityType));
      else
         for( Index i = 0; i < m; ++i )
            const_types[i] = Ipopt::TNLP::LINEAR;
      return true;
   }

   for( Index i = 0; i < m; ++i )
      const_types[i] = gmoGetEquOrderOne(gmo, i) > gmoorder_L ? Ipopt::TNLP::NON_LINEAR : Ipopt::TNLP::LINEAR;

   return true;
}

bool GamsMINLP::get_variables_types(
   Ipopt::Index       n,
   Bonmin::TMINLP::VariableType* var_types
)
{
   for( int i = 0; i < n; ++i )
   {
      switch( gmoGetVarTypeOne(gmo, i) )
      {
         case gmovar_X:
            var_types[i] = CONTINUOUS;
            break;
         case gmovar_B:
            var_types[i] = BINARY;
            break;
         case gmovar_I:
            var_types[i] = INTEGER;
            break;
         case gmovar_S1:
         case gmovar_S2:
            var_types[i] = CONTINUOUS;
            break;
         case gmovar_SC:
         case gmovar_SI:
         default:
         {
            gevLogStat(gev, "Error: Semicontinuous and semiinteger variables not supported.\n");
            return false;
         }
      }
   }

   return true;
}

bool GamsMINLP::hasLinearObjective()
{
   return gmoObjNLNZ(gmo) == 0;
}

bool GamsMINLP::eval_f(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool               new_x,
   Ipopt::Number&     obj_value
)
{
   assert(n == gmoN(gmo));

   if( new_x )
      gmoEvalNewPoint(gmo, x);

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

   obj_value *= isMin;
   return true;
}

bool GamsMINLP::eval_grad_f(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool               new_x,
   Ipopt::Number*     grad_f
)
{
   assert(n == gmoN(gmo));

   if( new_x )
      gmoEvalNewPoint(gmo, x);

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

   if( isMin == -1.0 )
      for( int i = n; i; --i )
         (*grad_f++) *= -1.0;

   return true;
}

bool GamsMINLP::eval_g(
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
      gmoEvalNewPoint(gmo, x);

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

   return true;
}

bool GamsMINLP::eval_jac_g(
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
      // compute values of jacobian
      assert(NULL != x);
      assert(NULL == iRow);
      assert(NULL == jCol);
      assert(NULL != iRowStart);
      assert(NULL != this->jCol);
      assert(NULL != grad);

      if( new_x )
         gmoEvalNewPoint(gmo, x);

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
   }

   return true;
}

bool GamsMINLP::eval_gi(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool               new_x,
   Ipopt::Index       i,
   Ipopt::Number&     gi
)
{
   assert(n == gmoN(gmo));

   if( new_x )
      gmoEvalNewPoint(gmo, x);

   int nerror;
   int rc = gmoEvalFunc(gmo, i, x, &gi, &nerror);

   if( rc != 0 )
   {
      char buffer[255];
      sprintf(buffer, "Critical error %d detected in evaluation of constraint %d!\n"
         "Exiting from subroutine - eval_gi\n", rc, i);
      gevLogStatPChar(gev, buffer);
      throw -1;
   }
   if( nerror > 0 )
   {
      ++domviolations;
      return false;
   }

   return true;
}

bool GamsMINLP::eval_grad_gi(
   Ipopt::Index       n,
   const Ipopt::Number* x,
   bool               new_x,
   Ipopt::Index       i,
   Ipopt::Index&      nele_grad_gi,
   Ipopt::Index*      jCol,
   Ipopt::Number*     values
)
{
   assert(n == gmoN(gmo));
   assert(i < gmoM(gmo));

   if( values == NULL )
   {
      assert(NULL == x);
      assert(NULL != jCol);
      assert(NULL != jCol);
      assert(NULL != iRowStart);

      nele_grad_gi = iRowStart[i+1] - iRowStart[i];
      memcpy(jCol, jCol + iRowStart[i], nele_grad_gi * sizeof(int));

   }
   else
   {
      assert(NULL != x);
      assert(NULL == jCol);
      assert(NULL != grad);

      if( new_x )
         gmoEvalNewPoint(gmo, x);

      double val;
      double gx;

      int nerror;
      int rc = gmoEvalGrad(gmo, i, x, &val, grad, &gx, &nerror);

      if( rc != 0 )
      {
         char buffer[255];
         sprintf(buffer, "Critical error %d detected in evaluation of gradient of constraint %d!\n"
            "Exiting from subroutine - eval_grad_gi\n", rc, i);
         gevLogStatPChar(gev, buffer);
         throw -1;
      }
      if( nerror > 0 )
      {
         ++domviolations;
         return false;
      }

      int next = iRowStart[i+1];
      for( int k = iRowStart[i]; k < next; ++k, ++values )
         *values = grad[jCol[k]];
   }

   return true;
}

bool GamsMINLP::eval_h(
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
         gmoEvalNewPoint(gmo, x);

      // for GAMS, lambda would need to be multiplied by -1, we do this via the constraint weight
      int nerror;
      int rc;
      rc = gmoHessLagValue(gmo, const_cast<double*>(x), const_cast<double*>(lambda), values, isMin*obj_factor, -1.0, &nerror);
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
   }

   return true;
}

void GamsMINLP::finalize_solution(
   Bonmin::TMINLP::SolverReturn status,
   Ipopt::Index       n,
   const Ipopt::Number* x,
   Ipopt::Number      obj_value
)
{
   solver_status = gmoSolveStat_Normal;
   switch( status )
   {
      case TMINLP::SUCCESS:
      {
         if( x != NULL )
         {
            // report globally optimal if optcr=optca=0 and we are running in couenne or model is a mip
            if( gevGetDblOpt(gev, gevOptCA) == 0.0 && gevGetDblOpt(gev, gevOptCR) == 0.0 && (in_couenne || (gmoObjNLNZ(gmo) == 0 && gmoNLNZ(gmo) == 0)) )
               model_status = gmoModelStat_OptimalGlobal; // optimal
            else
               model_status = (gmoNDisc(gmo) > 0) ? gmoModelStat_Integer : gmoModelStat_Feasible; // (integer) feasible solution
         }
         else
         {
            // this should not happen
            model_status = gmoModelStat_ErrorNoSolution; // error - no solution
         }
         break;
      }

      case TMINLP::INFEASIBLE:
      {
         if( in_couenne || (gmoObjNLNZ(gmo) == 0 && gmoNLNZ(gmo) == 0) )
            model_status = gmoModelStat_InfeasibleNoSolution; // proven infeasible - no solution
         else
            model_status = gmoModelStat_InfeasibleLocal;
         break;
      }

      case TMINLP::CONTINUOUS_UNBOUNDED:
      {
         model_status = gmoModelStat_UnboundedNoSolution; // unbounded - no solution
         break;
      }

      case TMINLP::LIMIT_EXCEEDED:
      {
         if( gevTimeDiffStart(gev) - clockStart >= gevGetDblOpt(gev, gevResLim) )
            solver_status = gmoSolveStat_Resource;
         else // should be iteration limit or node limit, and GAMS only has a status for iteration limit
            solver_status = gmoSolveStat_Iteration;

         if( x != NULL )
            model_status = (gmoNDisc(gmo) > 0) ? gmoModelStat_Integer : gmoModelStat_Feasible; // (integer) feasible solution
         else
            model_status = gmoModelStat_NoSolutionReturned; // no solution returned

         break;
      }

      case TMINLP::USER_INTERRUPT:
      {
         solver_status = gmoSolveStat_User;

         if( x != NULL )
            model_status = (gmoNDisc(gmo) > 0) ? gmoModelStat_Integer : gmoModelStat_Feasible; // (integer) feasible solution
         else
            model_status = gmoModelStat_NoSolutionReturned; // no solution returned

         break;
      }

      case TMINLP::MINLP_ERROR:
      {
         solver_status = gmoSolveStat_SolverErr;
         if( x!= NULL )
            model_status = gmoModelStat_InfeasibleIntermed; // intermediate infeasible
         else
            model_status = gmoModelStat_NoSolutionReturned; // no solution returned
         break;
      }

      default:
      {
         // should not happen, since other mipStatus is not defined
         model_status  = gmoModelStat_ErrorUnknown; // error unknown
         solver_status = gmoSolveStat_InternalErr; // error internal solver error
      }
   }
}
