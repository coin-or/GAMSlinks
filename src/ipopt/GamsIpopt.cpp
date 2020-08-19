// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsLinksConfig.h"
#include "GamsIpopt.hpp"
#include "GamsNLP.hpp"
#include "GamsJournal.hpp"

#include "IpoptConfig.h"
#include "IpSolveStatistics.hpp"

#include <cstdio>
#include <cstring>
#include <cassert>

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#include "palmcc.h"

#include "GamsLicensing.h"
#include "GamsHelper.h"
#ifdef GAMS_BUILD
#include "GamsHSLInit.h"
#endif

using namespace Ipopt;

int GamsIpopt::readyAPI(
   struct gmoRec*     gmo_
)
{
   struct palRec* pal;
   char buffer[GMS_SSSIZE];

   gmo = gmo_;
   assert(gmo != NULL);

   /* free a previous Ipopt instance, if existing */
   ipopt = NULL;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   ipoptlicensed = false;

   if( !palCreate(&pal, buffer, sizeof(buffer)) )
   {
      gevLogStat(gev, buffer);
      return 1;
   }

#if PALAPIVERSION >= 3
   palSetSystemName(pal, "COIN-OR Ipopt");
   palGetAuditLine(pal,buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

#ifdef GAMS_BUILD
   GAMSinitLicensing(gmo, pal);
   if( gevGetIntOpt(gev, gevCurSolver) == gevSolver2Id(gev, "ipopth") )
   {
      ipoptlicensed = GAMScheckIpoptLicense(pal, false);
      if( !ipoptlicensed )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_License);
         gmoModelStatSet(gmo, gmoModelStat_LicenseError);
         gevLogStatPChar(gev, "\nYou may want to try the free version IPOPT instead of IPOPTH.\n\n");
         return 1;
      }
      GamsHSLInit();
   }
#endif

   gevLogStatPChar(gev, "\nCOIN-OR Interior Point Optimizer (Ipopt Library " IPOPT_VERSION ")\n");
   if( ipoptlicensed )
      gevLogStatPChar(gev, "written by A. Waechter, commercially supported by GAMS Development Corp.\n");
   else
      gevLogStatPChar(gev, "written by A. Waechter.\n");

#ifdef GAMS_BUILD
   if( !ipoptlicensed && GAMScheckIpoptLicense(pal, true) )
      gevLogPChar(gev, "\nNote: This is the free version IPOPT, but you could also use the commercially supported and potentially higher performance version IPOPTH.\n");
#endif

   palFree(&pal);

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
   ipopt->Options()->SetIntegerValue("acceptable_iter", 0, true, true);  // GAMS does not have a proper status if Ipopt returns with this
   ipopt->Options()->SetIntegerValue("max_iter", gevGetIntOpt(gev, gevIterLim), true, true);
   ipopt->Options()->SetNumericValue("max_cpu_time", gevGetDblOpt(gev, gevResLim), true, true);
   ipopt->Options()->SetStringValue("mu_strategy", "adaptive", true, true);
   ipopt->Options()->SetStringValue("ma86_order", "auto", true, true);
   if( ipoptlicensed )
   {
      ipopt->Options()->SetStringValue("linear_solver", "ma27", true, true);
      ipopt->Options()->SetStringValue("linear_system_scaling", "mc19", true, true);
   }
   else
   {
      ipopt->Options()->SetStringValue("linear_solver", "mumps", true, true);
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
      ipopt->Initialize(buffer, false);
   }
   else
      ipopt->Initialize("", false);

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
   // TODO if hessian is only approximated by GMO (extr. func without 2nd deriv info), then could suggest user to enable Ipopts hessian approx via log
   std::string hess_approx;
   ipopt->Options()->GetStringValue("hessian_approximation", hess_approx, "");
   if( hess_approx == "exact" )
   {
      int do2dir = 0;
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
   gmoEvalErrorMsg(gmo, printevalerror);

   ipopt->Options()->GetBoolValue("report_mininfeas_solution", nlp->reportmininfeas, "");

   // set number of threads in linear algebra
   GAMSsetNumThreads(gev, gevThreads(gev));

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

#define GAMSSOLVER_ID ipo
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
   gmoInitMutexes();
   gevInitMutexes();
   palInitMutexes();
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
   gmoFiniMutexes();
   gevFiniMutexes();
   palFiniMutexes();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,create)(void** Cptr, char* msgBuf, int msgBufLen)
{
   assert(Cptr != NULL);
   assert(msgBuf != NULL);

   *Cptr = NULL;

   if( !gmoGetReady(msgBuf, msgBufLen) )
      return 0;

   if( !gevGetReady(msgBuf, msgBufLen) )
      return 0;

   if( !palGetReady(msgBuf, msgBufLen) )
      return 0;

   *Cptr = (void*) new GamsIpopt();
   if( *Cptr == NULL )
   {
      snprintf(msgBuf, msgBufLen, "Out of memory when creating GamsIpopt object.\n");
      if( msgBufLen > 0 )
         msgBuf[msgBufLen] = '\0';
      return 0;
   }

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   assert(Cptr != NULL);

   delete (GamsIpopt*)*Cptr;
   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsIpopt*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsIpopt*)Cptr)->readyAPI(Gptr);
}
