// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsIpopt.hpp"
#include "GamsNLP.hpp"
#include "GamsJournal.hpp"

#include "IpoptConfig.h"
#include "IpSolveStatistics.hpp"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <climits>

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

void GamsIpopt::setupIpopt()
{
   ipopt = new IpoptApplication(false);

   // setup own journal
   if( gev != NULL )
   {
      SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY);
      jrnl->SetPrintLevel(J_DBG, J_NONE);
      if( !ipopt->Jnlst()->AddJournal(jrnl) )
         gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");
   }

   // add options
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

   // change some option defaults
   ipopt->Options()->clear();
   ipopt->Options()->SetNumericValue("bound_relax_factor", 1e-10, true, true);
   ipopt->Options()->SetIntegerValue("acceptable_iter", 0, true, true);  // GAMS does not have a proper status if Ipopt returns with this
   ipopt->Options()->SetNumericValue("constr_viol_tol", 1e-6, true, true);  // as with many other GAMS solvers
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

   warmstart = false;
}

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

   palSetSystemName(pal, "COIN-OR Ipopt");
   palGetAuditLine(pal,buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);

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

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);

   try
   {
      setupIpopt();
   }
   catch( const std::exception& e )
   {
      gevLogStat(gev, e.what());
      return 1;
   }

   gevTerminateInstall(gev);

   if( gmoScaleOpt(gmo) )
      ipopt->Options()->SetStringValue("nlp_scaling_method", "none", true, true);

   // if linear objective, then Ipopt can skip reevaluating the objective gradient
   if( gmoObjNLNZ(gmo) == 0 )
      ipopt->Options()->SetStringValue("grad_f_constant", "yes", true, true);
   if( gmoNLM(gmo) == 0 )
   {
      // if linear constraints, then Ipopt can skip reevaluating the Jacobian
      ipopt->Options()->SetStringValue("jac_c_constant", "yes", true, true);
      ipopt->Options()->SetStringValue("jac_d_constant", "yes", true, true);
      // if also quadratic objective, then Ipopt can skip reevaluating the Hessian, too
      if( gmoModelType(gmo) == gmoProc_qcp || gmoModelType(gmo) == gmoProc_rmiqcp )
         ipopt->Options()->SetStringValue("hessian_constant", "yes", true, true);
   }
   else
   {
      // there are nonlinear constraints, but lets see if maybe all equalities are linear or all inequalities are linear
      bool jac_c_constant = true;
      bool jac_d_constant = true;
      int nz, qnz, nlnz;
      for( int i = 0; i < gmoM(gmo) && (jac_c_constant || jac_d_constant); ++i )
      {
         gmoGetRowStat(gmo, i, &nz, &qnz, &nlnz);
         if( nlnz == 0 )
            continue;
         if( gmoGetEquTypeOne(gmo, i) == gmoequ_E ) // nonlinear equality
            jac_c_constant = false;
         else
            jac_d_constant = false;  // nonlinear inequality or free
      }
      if( jac_c_constant )
         ipopt->Options()->SetStringValue("jac_c_constant", "yes", true, true);
      if( jac_d_constant )
         ipopt->Options()->SetStringValue("jac_d_constant", "yes", true, true);
   }
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

   nlp = new GamsNLP(gmo);

   delete[] boundtype;
   boundtype = NULL;

   // get tolerances
   ipopt->Options()->GetNumericValue("diverging_iterates_tol", nlp->div_iter_tol, "");
   ipopt->Options()->GetNumericValue("constr_viol_tol", nlp->conviol_tol, "");
   ipopt->Options()->GetNumericValue("compl_inf_tol", nlp->compl_tol, "");

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

   ipopt->Options()->GetBoolValue("report_mininfeas_solution", nlp->reportmininfeas, "");

   return 0;
}

int GamsIpopt::callSolver()
{
   assert(gmo != NULL);
   assert(gev != NULL);
   assert(IsValid(ipopt));
   assert(IsValid(nlp));

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);

   if( gmoNZ64(gmo) > INT_MAX )
   {
      gevLogStat(gev, "ERROR: Problems with more than 2^31 nonzeros not supported.");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   // process options and setup NLP
   ipopt->Options()->SetIntegerValue("max_iter", gevGetIntOpt(gev, gevIterLim), true, true);
   ipopt->Options()->SetNumericValue("max_wall_time", gevGetDblOpt(gev, gevResLim), true, true);

   double ipoptinf;
   ipopt->Options()->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
   gmoMinfSet(gmo, ipoptinf);
   ipopt->Options()->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
   gmoPinfSet(gmo, ipoptinf);

   bool printevalerror;
   ipopt->Options()->GetBoolValue("print_eval_error", printevalerror, "");
   gmoEvalErrorMsg(gmo, printevalerror);

   if( boundtype == NULL )
   {
      // remember for which variable a lower or upper bound is given
      // we need this in modifyProblem (though we have no way to know whether that will be called, it seems)
      boundtype = new uint8_t[gmoN(gmo)];
      for( int i = 0; i < gmoN(gmo); ++i )
      {
         boundtype[i] = 0;
         if( gmoGetVarLowerOne(gmo, i) != gmoMinf(gmo) )
            boundtype[i] |= 1u;
         if( gmoGetVarUpperOne(gmo, i) != gmoPinf(gmo) )
            boundtype[i] |= 2u;
      }
   }

   // set number of threads in linear algebra
   if( gevGetIntOpt(gev, gevThreadsRaw) != 0 )
      GAMSsetNumThreads(gev, gevThreads(gev));
   else
      GAMSsetNumThreads(gev, 1);

   // solve NLP
   ApplicationReturnStatus status;
   try
   {
      if( !warmstart )
         status = ipopt->OptimizeTNLP(GetRawPtr(nlp));
      else
         status = ipopt->ReOptimizeTNLP(GetRawPtr(nlp));
   }
   catch( IpoptException& e )
   {
      status = Unrecoverable_Exception;
      gevLogStat(gev, e.Message().c_str());
   }

   SmartPtr<SolveStatistics> solvestat = ipopt->Statistics();
   if( IsValid(solvestat) )
   {
      gmoSetHeadnTail(gmo, gmoHresused, solvestat->TotalWallclockTime());
      gmoSetHeadnTail(gmo, gmoHiterused, solvestat->IterationCount());
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
      case Maximum_WallTime_Exceeded:
      case Restoration_Failed:
      case Error_In_Step_Computation:
      case Feasible_Point_Found:
         break; // these should have been handled by FinalizeSolution already

      case Not_Enough_Degrees_Of_Freedom:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      case Invalid_Problem_Definition:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SetupErr);
         break;

      case Invalid_Option:
      {
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SetupErr);
         std::string linsolver;
         ipopt->Options()->GetStringValue("linear_solver", linsolver, "");
         if( linsolver == "pardiso" )
         {
            gevLogStat(gev, "NOTE: With Ipopt 3.14 (GAMS 36), value pardiso of option linear_solver has been renamed to pardisomkl.");
            gevLogStat(gev, "      If using Pardiso from MKL was intended, the Ipopt options file needs to be changed to \"linear_solver pardisomkl\".");
         }
         break;
      }

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

int GamsIpopt::modifyProblem()
{
   assert(IsValid(ipopt));
   assert(IsValid(nlp));

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);

   if( !warmstart )
   {
      ipopt->Options()->SetStringValue("warm_start_init_point", "yes");
      // these could make sense, but lets let the user set these (they usually don't affect the first solve)
      // ipopt->Options()->SetNumericValue("warm_start_bound_frac", 1e-16, true, false);
      // ipopt->Options()->SetNumericValue("warm_start_bound_push", 1e-16, true, false);
      // ipopt->Options()->SetNumericValue("warm_start_mult_bound_push", 1e-16, true, false);
      // ipopt->Options()->SetNumericValue("warm_start_slack_bound_frac", 1e-16, true, false);
      // ipopt->Options()->SetNumericValue("warm_start_slack_bound_push", 1e-16, true, false);
      // ipopt->Options()->SetNumericValue("mu_init", 1e-8, true, false);
      warmstart = true;
   }

   // check whether structure of NLP did not change
   // we can assume that nonzero-structure and equation sense (=L=, ...) didn't change
   // but for Ipopt it is a structural change if a variable bound appeared or disappeared
   bool structurechanged = false;
   for( int i = 0; i < gmoN(gmo); ++i )
   {
      uint8_t newboundtype = 0;
      if( gmoGetVarLowerOne(gmo, i) != gmoMinf(gmo) )
         newboundtype |= 1u;
      if( gmoGetVarUpperOne(gmo, i) != gmoPinf(gmo) )
         newboundtype |= 2u;

      if( boundtype[i] != newboundtype )
         structurechanged = true;
      boundtype[i] = newboundtype;
   }
   ipopt->Options()->SetStringValue("warm_start_same_structure", structurechanged ? "no" : "yes");

   return 0;
}

int ipoCreate(
   void** Cptr,
   char*  msgBuf,
   int    msgBufLen
   )
{
   assert(Cptr != NULL);
   assert(msgBuf != NULL);

   *Cptr = NULL;

   if( !gmoGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !gevGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !palGetReady(msgBuf, msgBufLen) )
      return 1;

   *Cptr = (void*) new GamsIpopt();
   if( *Cptr == NULL )
   {
      snprintf(msgBuf, msgBufLen, "Out of memory when creating GamsIpopt object.\n");
      if( msgBufLen > 0 )
         msgBuf[msgBufLen] = '\0';
      return 1;
   }

   return 0;
}

void ipoFree(
   void** Cptr
   )
{
   assert(Cptr != NULL);

   delete (GamsIpopt*)*Cptr;
   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();
}

int ipoCallSolver(
   void* Cptr
   )
{
   assert(Cptr != NULL);
   return ((GamsIpopt*)Cptr)->callSolver();
}

int ipoReadyAPI(
   void*       Cptr,
   gmoHandle_t Gptr
   )
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsIpopt*)Cptr)->readyAPI(Gptr);
}

int ipoHaveModifyProblem(
   void* Cptr
)
{
   return 0;
}

int ipoModifyProblem(
   void* Cptr
   )
{
   assert(Cptr != NULL);

   return ((GamsIpopt*)Cptr)->modifyProblem();
}

#ifdef __GNUC__
__attribute__((constructor))
#endif
static void ipoinit(void)
{
   gmoInitMutexes();
   gevInitMutexes();
   palInitMutexes();
}

#ifdef __GNUC__
__attribute__((destructor))
#endif
static void ipofini(void)
{
   gmoFiniMutexes();
   gevFiniMutexes();
   palFiniMutexes();
}

#ifdef _WIN32
#include <windows.h>  /* to make sure that BOOL is defined */

BOOL WINAPI DllMain(
   HINSTANCE hInst,
   DWORD     reason,
   LPVOID    reserved
)
{
   switch( reason )
   {
      case DLL_PROCESS_ATTACH:
         ipoinit();
         break;
      case DLL_PROCESS_DETACH:
         ipofini();
         break;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
         break;
   }
   return TRUE;
}
#endif
