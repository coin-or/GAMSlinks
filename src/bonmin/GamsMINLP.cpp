// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors: Stefan Vigerske

#include "GamsMINLP.hpp"
#include "GAMSlinksConfig.h"
#include "BonminConfig.h"

#include <cstring> // for memcpy

#include "gmomcc.h"
#include "gevmcc.h"

#include "GamsCompatibility.h"

using namespace Ipopt;
using namespace Bonmin;

GamsMINLP::GamsMINLP(
   struct gmoRec*     gmo_,               /**< GAMS modeling object */
   bool               in_couenne_         /**< whether the MINLP is used within Couenne */
)
: gmo(gmo_),
  gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL),
  in_couenne(in_couenne_),
  model_status(gmoModelStat_ErrorNoSolution),
  solver_status(gmoSolveStat_SetupErr)
{
   assert(gmo != NULL);

   isMin = (gmoSense(gmo) == gmoObj_Min ? 1.0 : -1.0);

   nlp = new GamsNLP(gmo);

   setupPrioritiesSOS();

   reset_eval_counter();
}

void GamsMINLP::reset_eval_counter()
{
   nlp->reset_eval_counter();
   nevalsinglecons     = 0;
   nevalsingleconsgrad = 0;
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

   for( int i = 0; i < numSos; ++i )
   {
      sosinfo.types[i] = (char)sostype[i];
      sosinfo.priorities[i] = gmoN(gmo) - (sosinfo.starts[i+1] - sosinfo.starts[i]); // branch on long sets first
   }

   delete[] sostype;
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
   {
      gmoEvalNewPoint(gmo, x);
      ++nlp->nevalnewpoint;
   }

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
      ++nlp->domviolations;
      return false;
   }

   ++nevalsinglecons;

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
      assert(NULL != nlp->jCol);
      assert(NULL != nlp->iRowStart);

      nele_grad_gi = nlp->iRowStart[i+1] - nlp->iRowStart[i];
      memcpy(jCol, nlp->jCol + nlp->iRowStart[i], nele_grad_gi * sizeof(int));

   }
   else
   {
      assert(NULL != x);
      assert(NULL == jCol);
      assert(NULL != nlp->grad);

      if( new_x )
      {
         ++nlp->nevalnewpoint;
         gmoEvalNewPoint(gmo, x);
      }

      double val;
      double gx;

      int nerror;
      int rc = gmoEvalGrad(gmo, i, x, &val, nlp->grad, &gx, &nerror);

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
         ++nlp->domviolations;
         return false;
      }

      int next = nlp->iRowStart[i+1];
      for( int k = nlp->iRowStart[i]; k < next; ++k, ++values )
         *values = nlp->grad[nlp->jCol[k]];

      ++nevalsingleconsgrad;
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
         if( gevTimeDiffStart(gev) - nlp->clockStart >= gevGetDblOpt(gev, gevResLim) )
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
