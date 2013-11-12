// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsIpopt.hpp"
#include "GamsNLP.hpp"
#include "GamsJournal.hpp"
#include "GAMSlinksConfig.h"

#include "IpoptConfig.h"
#include "IpSolveStatistics.hpp"

#include <cstdio>
#include <cstring>
#include <cassert>

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "palmcc.h"
#endif

#include "GamsCompatibility.h"

using namespace Ipopt;

int GamsIpopt::readyAPI(
   struct gmoRec*     gmo_,
   struct optRec*     opt
)
{
   gmo = gmo_;
   assert(gmo != NULL);

   /* free a previous Ipopt instance, if existing */
   ipopt = NULL;

   if( getGmoReady() || getGevReady() )
      return 1;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   ipoptlicensed = false;
#ifdef GAMS_BUILD
   struct palRec* pal;
   char buffer[GMS_SSSIZE];

   if( !palCreate(&pal, buffer, sizeof(buffer)) )
      return 1;

#define PALPTR pal
#include "coinlibdCL3svn.h" 
   palGetAuditLine(pal,buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);

   initLicensing(gmo, pal);
   if( gevGetIntOpt(gev, gevCurSolver) == gevSolver2Id(gev, "ipopth") )
   {
      ipoptlicensed = HSLInit(gmo, pal);

      if( !ipoptlicensed  )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_License);
         gmoModelStatSet(gmo, gmoModelStat_LicenseError);
         gevLogStatPChar(gev, "\nYou may want to try the free version IPOPT instead of IPOPTH.\n\n");
         return 1;
      }
   }
#endif

   gevLogStatPChar(gev, "\nCOIN-OR Interior Point Optimizer (Ipopt Library "IPOPT_VERSION")\n");
   if( ipoptlicensed )
      gevLogStatPChar(gev, "written by A. Waechter, commercially supported by GAMS Development Corp.\n");
   else
      gevLogStatPChar(gev, "written by A. Waechter.\n");

#ifdef GAMS_BUILD
   if( !ipoptlicensed && (palLicenseCheckSubSys(pal, const_cast<char*>("IP")) == 0) )
      gevLogPChar(gev, "\nNote: This is the free version IPOPT, but you could also use the commercially supported and potentially higher performance version IPOPTH.\n");

   palFree(&pal);
#endif

   ipopt = new IpoptApplication(false);

   SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY);
   jrnl->SetPrintLevel(J_DBG, J_NONE);
   if( !ipopt->Jnlst()->AddJournal(jrnl) )
      gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");

   ipopt->RegOptions()->SetRegisteringCategory("Output");
   ipopt->RegOptions()->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "yes",
      "no", "", "yes", "");
   ipopt->RegOptions()->AddStringOption2("report_mininfeas_solution",
      "Switch to report intermediate solution with minimal constraint violation to GAMS if the final solution is not feasible.",
      "no",
      "no", "", "yes", "",
      "This option allows to obtain the most feasible solution found by Ipopt during the iteration process, if it stops at a (locally) infeasible solution, due to a limit (time, iterations, ...), or with a failure in the restoration phase.");

   gevTerminateInstall(gev);

   return 0;
}

int GamsIpopt::callSolver()
{
   assert(gmo != NULL);
   assert(gev != NULL);
   assert(IsValid(ipopt));

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);

   // change some option defaults
   ipopt->Options()->clear();
   ipopt->Options()->SetNumericValue("bound_relax_factor", 1e-10, true, true);
   ipopt->Options()->SetIntegerValue("max_iter", gevGetIntOpt(gev, gevIterLim), true, true);
   ipopt->Options()->SetNumericValue("max_cpu_time", gevGetDblOpt(gev, gevResLim), true, true);
   ipopt->Options()->SetStringValue("mu_strategy", "adaptive", true, true);
   ipopt->Options()->SetStringValue("ma86_order", "auto", true, true);
   if( ipoptlicensed )
   {
      ipopt->Options()->SetStringValue("linear_solver", "ma27", true, true);
      ipopt->Options()->SetStringValue("linear_system_scaling", "mc19", true, true);
   }

   // if we have linear rows and a quadratic objective, then the hessian of the Lag.func. is constant, and Ipopt can make use of this
   if( gmoNLM(gmo) == 0 && (gmoModelType(gmo) == gmoProc_qcp || gmoModelType(gmo) == gmoProc_rmiqcp) )
      ipopt->Options()->SetStringValue("hessian_constant", "yes", true, true);
   if( gmoSense(gmo) == gmoObj_Max )
      ipopt->Options()->SetNumericValue("obj_scaling_factor", -1.0, true, true);

   // read user options, if given
   if( gmoOptFile(gmo) > 0 )
   {
      char buffer[GMS_SSSIZE];
      ipopt->Options()->SetStringValue("print_user_options", "yes", true, true);
      gmoNameOptFile(gmo, buffer);
      ipopt->Initialize(buffer);
   }
   else
      ipopt->Initialize("");

   // process options and setup NLP
   double ipoptinf;
   ipopt->Options()->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
   gmoMinfSet(gmo, ipoptinf);
   ipopt->Options()->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
   gmoPinfSet(gmo, ipoptinf);

   SmartPtr<GamsNLP> nlp = new GamsNLP(gmo);
   ipopt->Options()->GetNumericValue("diverging_iterates_tol", nlp->div_iter_tol, "");

   // get scaled and unscaled constraint violation tolerances
   ipopt->Options()->GetNumericValue("tol", nlp->scaled_conviol_tol, "");
   ipopt->Options()->GetNumericValue("acceptable_tol", nlp->scaled_conviol_acctol, "");
   ipopt->Options()->GetNumericValue("constr_viol_tol", nlp->unscaled_conviol_tol, "");
   ipopt->Options()->GetNumericValue("acceptable_constr_viol_tol", nlp->unscaled_conviol_acctol, "");

   // initialize GMO hessian, if required
   std::string hess_approx;
   ipopt->Options()->GetStringValue("hessian_approximation", hess_approx, "");
   if( hess_approx == "exact" )
   {
      int do2dir = 1;
      int dohess = 1;
      gmoHessLoad(gmo, 0, &do2dir, &dohess);
      if( !dohess )
      {
         gevLogStat(gev, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
         ipopt->Options()->SetStringValue("hessian_approximation", "limited-memory");
      }
   }

   bool printevalerror;
   ipopt->Options()->GetBoolValue("print_eval_error", printevalerror, "");
   gmoEvalErrorNoMsg(gmo, !printevalerror);

   ipopt->Options()->GetBoolValue("report_mininfeas_solution", nlp->reportmininfeas, "");

   // set number of threads in linear algebra
   setNumThreads(gev, gevThreads(gev));

   // solve NLP
   nlp->clockStart = gevTimeDiffStart(gev);
   ApplicationReturnStatus status;
   try
   {
      status = ipopt->OptimizeTNLP(GetRawPtr(nlp));
   }
   catch( IpoptException& e )
   {
      status = Unrecoverable_Exception;
      gevLogStat(gev, e.Message().c_str());
   }

   // process solution status
   switch( status )
   {
      case Solve_Succeeded:
      case Solved_To_Acceptable_Level:
      case Infeasible_Problem_Detected:
      case Search_Direction_Becomes_Too_Small:
      case Diverging_Iterates:
      case User_Requested_Stop:
      case Maximum_Iterations_Exceeded:
      case Maximum_CpuTime_Exceeded:
      case Restoration_Failed:
      case Error_In_Step_Computation:
         break; // these should have been handled by FinalizeSolution already

      case Not_Enough_Degrees_Of_Freedom:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      case Invalid_Problem_Definition:
      case Invalid_Option:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SetupErr);
         break;

      case Invalid_Number_Detected:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_EvalError);
         break;

      case Unrecoverable_Exception:
      case Internal_Error:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_InternalErr);
         break;

      case Insufficient_Memory:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      case NonIpopt_Exception_Thrown:
      default:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         break;
   }

   if( gmoModelType(gmo) == gmoProc_cns )
      switch( gmoModelStat(gmo) )
      {
         case gmoModelStat_OptimalGlobal:
         case gmoModelStat_OptimalLocal:
         case gmoModelStat_Feasible:
         case gmoModelStat_Integer:
            gmoModelStatSet(gmo, gmoModelStat_Solved);
      }

   return 0;
}

#define GAMSSOLVERC_ID         ipo
#define GAMSSOLVERC_CLASS      GamsIpopt
#include "GamsSolverC_tpl.cpp"
