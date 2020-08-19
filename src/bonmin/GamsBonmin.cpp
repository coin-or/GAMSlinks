// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <cstdio>
#include <cstring>
#include <climits>

#include "GamsBonmin.hpp"
#include "GamsMINLP.hpp"
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsCbcHeurSolveTrace.hpp"
#include "GamsLinksConfig.h"

#include "BonminConfig.h"
#include "BonBonminSetup.hpp"
#include "BonCbc.hpp"

#if defined(__linux) && defined(GAMSLINKS_HAS_CPLEX)
#include "cplex.h"
#endif

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#include "gclgms.h"
#include "palmcc.h"

#include "GamsLicensing.h"
#include "GamsHelper.h"
#ifdef GAMS_BUILD
#include "GamsHSLInit.h"
#endif

using namespace Bonmin;
using namespace Ipopt;

GamsBonmin::~GamsBonmin()
{
   delete bonmin_setup;
   delete msghandler;

   if( pal != NULL )
      palFree(&pal);
}

int GamsBonmin::readyAPI(
   struct gmoRec*     gmo                 /**< GAMS modeling object */
)
{
   this->gmo = gmo;
   assert(gmo != NULL);

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   ipoptlicensed = false;
   char buffer[GMS_SSSIZE];

   if( pal == NULL && !palCreate(&pal, buffer, sizeof(buffer)) )
      return 1;

#if PALAPIVERSION >= 3
   palSetSystemName(pal, "COIN-OR Bonmin");
   palGetAuditLine(pal, buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

   GAMSinitLicensing(gmo, pal);
#ifdef GAMS_BUILD
   if( gevGetIntOpt(gev, gevCurSolver) == gevSolver2Id(gev, "bonminh") )
   {
      ipoptlicensed = GAMScheckIpoptLicense(pal, false);
      if( !ipoptlicensed )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_License);
         gmoModelStatSet(gmo, gmoModelStat_LicenseError);
         gevLogStatPChar(gev, "\nYou may want to try the free version BONMIN instead of BONMINH\n\n");
         return 1;
      }
      GamsHSLInit();
   }
#endif

   gevLogStatPChar(gev, "\nCOIN-OR Bonmin (Bonmin Library " BONMIN_VERSION ")\n");
   if( ipoptlicensed )
      gevLogStatPChar(gev, "written by P. Bonami, with commercially supported IpOpt.\n");
   else
      gevLogStatPChar(gev, "written by P. Bonami.\n");

#ifdef GAMS_BUILD
   if( !ipoptlicensed && GAMScheckIpoptLicense(pal, true) )
      gevLogPChar(gev, "\nNote: This is the free version BONMIN, but you could also use the commercially supported and potentially higher performance version BONMINH.\n");
#endif

   delete bonmin_setup;
   bonmin_setup = new BonminSetup();

   // instead of initializeOptionsAndJournalist we do it our own way, so we can use a GamsJournal
   SmartPtr<OptionsList> options = new OptionsList();
   SmartPtr<Journalist> journalist= new Journalist();
   SmartPtr<Bonmin::RegisteredOptions> roptions = new Bonmin::RegisteredOptions();
   SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY, J_STRONGWARNING);
   jrnl->SetPrintLevel(J_DBG, J_NONE);
   if( !journalist->AddJournal(jrnl) )
      gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");
   options->SetJournalist(journalist);
   options->SetRegisteredOptions(GetRawPtr(roptions));

   bonmin_setup->setOptionsAndJournalist(roptions, options, journalist);
   bonmin_setup->registerOptions();

   bonmin_setup->roptions()->SetRegisteringCategory("Output and Loglevel", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup->roptions()->AddStringOption2("print_funceval_statistics",
      "Switch to enable printing statistics on number of evaluations of GAMS functions/gradients/Hessian.",
      "no",
      "no", "", "yes", "");

   bonmin_setup->roptions()->AddStringOption1("solvetrace",
      "Name of file for writing solving progress information.",
      "", "*", "");

   bonmin_setup->roptions()->AddLowerBoundedIntegerOption("solvetracenodefreq",
      "Frequency in number of nodes for writing solving progress information.",
      0, 100, "giving 0 disables writing of N-lines to trace file");

   bonmin_setup->roptions()->AddLowerBoundedNumberOption("solvetracetimefreq",
      "Frequency in seconds for writing solving progress information.",
      0.0, false, 5.0, "giving 0.0 disables writing of T-lines to trace file");

   bonmin_setup->roptions()->SetRegisteringCategory("Output", Bonmin::RegisteredOptions::IpoptCategory);
   bonmin_setup->roptions()->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "yes",
      "no", "", "yes", "");

   bonmin_setup->roptions()->SetRegisteringCategory("NLP interface", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup->roptions()->AddStringOption2("solvefinal",
      "Switch to disable solving MINLP with discrete variables fixed to solution values after solve.",
      "yes",
      "no", "", "yes", "",
      "If enabled, then the dual values from the resolved NLP are made available in GAMS.");

   bonmin_setup->roptions()->SetRegisteringCategory("Branch-and-bound options", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup->roptions()->AddStringOption2("clocktype",
      "Type of clock to use for time_limit",
      "wall",
      "cpu", "CPU time", "wall", "Wall-clock time",
      "");

   return 0;
}

int GamsBonmin::callSolver()
{
   assert(gmo != NULL);
   assert(bonmin_setup != NULL);

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);

   if( gmoN(gmo) == 0 )
   {
      gevLogStat(gev, "Error: Bonmin requires variables.");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   if( gmoGetVarTypeCnt(gmo, gmovar_SC) > 0 || gmoGetVarTypeCnt(gmo, gmovar_SI) > 0 )
   {
      gevLogStat(gev, "ERROR: Semicontinuous and semiinteger variables not supported by Bonmin.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   // Change some options
   bonmin_setup->options()->clear();
   bonmin_setup->options()->SetNumericValue("bound_relax_factor", 1e-10, true, true);
   bonmin_setup->options()->SetStringValue("ma86_order", "auto", true, true);
   bonmin_setup->options()->SetIntegerValue("bonmin.nlp_log_at_root", Ipopt::J_ITERSUMMARY, true, true);
   bonmin_setup->options()->SetNumericValue("print_frequency_time", 0.5, true, true);
   if( gevGetIntOpt(gev, gevUseCutOff) )
      bonmin_setup->options()->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == gmoObj_Min ? gevGetDblOpt(gev, gevCutOff) : -gevGetDblOpt(gev, gevCutOff), true, true);
   bonmin_setup->options()->SetNumericValue("bonmin.allowable_gap", gevGetDblOpt(gev, gevOptCA), true, true);
   bonmin_setup->options()->SetNumericValue("bonmin.allowable_fraction_gap", gevGetDblOpt(gev, gevOptCR), true, true);
   if( gevGetIntOpt(gev, gevNodeLim) > 0 )
      bonmin_setup->options()->SetIntegerValue("bonmin.node_limit", gevGetIntOpt(gev, gevNodeLim), true, true);
   bonmin_setup->options()->SetNumericValue("bonmin.time_limit", gevGetDblOpt(gev, gevResLim), true, true);
   if( gevGetIntOpt(gev, gevIterLim) < INT_MAX )
      bonmin_setup->options()->SetIntegerValue("bonmin.iteration_limit", gevGetIntOpt(gev, gevIterLim), true, true);
   bonmin_setup->options()->SetIntegerValue("number_cpx_threads", gevThreads(gev), true, true);

   if( gmoNLM(gmo) == 0 && (gmoModelType(gmo) == gmoProc_qcp || gmoModelType(gmo) == gmoProc_rmiqcp || gmoModelType(gmo) == gmoProc_miqcp) )
      bonmin_setup->options()->SetStringValue("hessian_constant", "yes", true, true);

   if( ipoptlicensed )
   {
      bonmin_setup->options()->SetStringValue("linear_solver", "ma27", true, true);
      bonmin_setup->options()->SetStringValue("linear_system_scaling", "mc19", true, true);
   }
   else
   {
      bonmin_setup->options()->SetStringValue("linear_solver", "mumps", true, true);
   }

   // process options file
   try
   {
      if( gmoOptFile(gmo) )
      {
         char buffer[GMS_SSSIZE];
         bonmin_setup->options()->SetStringValue("print_user_options", "yes", true, true);
         gmoNameOptFile(gmo, buffer);
         bonmin_setup->readOptionsFile(buffer);
      }
      else
      {
         // need to let Bonmin read something, otherwise it will try to read its default option file bonmin.opt
         bonmin_setup->readOptionsString(std::string());
      }
   }
   catch( IpoptException& error )
   {
      gevLogStat(gev, error.Message().c_str());
      return 1;
   }
   catch( const std::bad_alloc& )
   {
      gevLogStat(gev, "Error: Not enough memory\n");
      return -1;
   }
   catch( ... )
   {
      gevLogStat(gev, "Error: Unknown exception thrown.\n");
      return -1;
   }

   // check for GAMS/CPLEX license, if required
   std::string parvalue;
#ifdef GAMSLINKS_HAS_CPLEX
   std::string prefixes[7] = { "", "bonmin", "oa_decomposition.", "pump_for_minlp.", "rins.", "rens.", "local_branch." };
   for( int i = 0; i < 7; ++i )
   {
      bonmin_setup->options()->GetStringValue("milp_solver", parvalue, prefixes[i]);
      if( parvalue == "Cplex" )
      {
         if( !GAMScheckCPLEXLicense(pal, true) )
         {
            gevLogStat(gev, "No valid CPLEX license found.");
            return 1;
         }
         else
            break;
      }
   }
#endif

   double ipoptinf;
   bonmin_setup->options()->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
   gmoMinfSet(gmo, ipoptinf);
   bonmin_setup->options()->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
   gmoPinfSet(gmo, ipoptinf);

   // setup MINLP
   SmartPtr<GamsMINLP> minlp = new GamsMINLP(gmo);
   bonmin_setup->options()->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");

   if( minlp->have_negative_sos() )
   {
      gevLogStat(gev, "Error: Bonmin requires all variables in SOS to be non-negative.");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   // initialize Hessian in GMO, if required
   bool hessian_is_approx = false;
   bonmin_setup->options()->GetStringValue("hessian_approximation", parvalue, "");
   if( parvalue == "exact" )
   {
      int do2dir = 0;
      int dohess = 1;
      gmoHessLoad(gmo, 0, &do2dir, &dohess);
      if( !dohess )
      {
         gevLogStat(gev, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
         bonmin_setup->options()->SetStringValue("hessian_approximation", "limited-memory");
         hessian_is_approx = true;
      }
   }
   else
      hessian_is_approx = true;

   if( hessian_is_approx )
   {
      // check whether QP strong branching is enabled
      bonmin_setup->options()->GetStringValue("variable_selection", parvalue, "bonmin.");
      if( parvalue == "qp-strong-branching" )
      {
         gevLogStat(gev, "Error: QP strong branching does not work when the Hessian is approximated. Aborting...");
         return 1;
      }
   }

   // check whether evaluation errors should be printed in status file
   bool printevalerror;
   bonmin_setup->options()->GetBoolValue("print_eval_error", printevalerror, "");
   gmoEvalErrorMsg(gmo, printevalerror);

   // set number of threads in linear algebra in ipopt
   GAMSsetNumThreads(gev, gevThreads(gev));

   // initialize solvetrace, if activated
   GAMS_SOLVETRACE* solvetrace_ = NULL;
   std::string solvetrace;
   bonmin_setup->options()->GetStringValue("solvetrace", solvetrace, "");
   if( solvetrace != "" )
   {
      char buffer[GMS_SSSIZE];
      int nodefreq;
      double timefreq;
      int rc;

      bonmin_setup->options()->GetIntegerValue("solvetracenodefreq", nodefreq, "");
      bonmin_setup->options()->GetNumericValue("solvetracetimefreq", timefreq, "");
      gmoNameInput(gmo, buffer);
      rc = GAMSsolvetraceCreate(&solvetrace_, solvetrace.c_str(), "Bonmin", gmoOptFile(gmo), buffer, ipoptinf, nodefreq, timefreq);
      if( rc != 0 )
      {
         gevLogStat(gev, "Initializing solvetrace failed.");
         GAMSsolvetraceFree(&solvetrace_);
      }
      else
      {
         bonmin_setup->heuristics().push_back(BabSetupBase::HeuristicMethod());
         BabSetupBase::HeuristicMethod& heurmeth(bonmin_setup->heuristics().back());
         heurmeth.heuristic = new GamsCbcHeurSolveTrace(solvetrace_, minlp->isMin);
         heurmeth.id = "Solvetrace writing heuristic";
      }
   }

   if( msghandler == NULL )
      msghandler = new GamsMessageHandler(gev);

   if( minlp->isMin == -1 )
      gevLog(gev, "Note: Maximization problem reformulated as minimization problem for Bonmin, objective values are negated in output.");

   try
   {
      // initialize Bonmin for current MINLP and options
      // the easiest would be to call bonmin_setup.initializeBonmin(minlp), but then we cannot set the message handler
      // so we do the following
      {
         OsiTMINLPInterface first_osi_tminlp;
         first_osi_tminlp.passInMessageHandler(msghandler);
         first_osi_tminlp.initialize(bonmin_setup->roptions(), bonmin_setup->options(), bonmin_setup->journalist(), GetRawPtr(minlp));
         // double* sol = new double[gmoN(gmo)];
         // gmoGetVarL(gmo, sol);
         // first_osi_tminlp.activateRowCutDebugger(sol);
         // delete[] sol;
         bonmin_setup->initialize(first_osi_tminlp); // this will clone first_osi_tminlp

         if( solvetrace_ != NULL )
            GAMSsolvetraceSetInfinity(solvetrace_, first_osi_tminlp.getInfinity());
      }

      // try solving
      Bab bb;

      minlp->nlp->clockStart = gevTimeDiffStart(gev);
      if( solvetrace_ != NULL )
         GAMSsolvetraceResetStarttime(solvetrace_);

      // change to wall-clock time if not requested otherwise
      std::string clocktype;
      bonmin_setup->options()->GetStringValue("clocktype", clocktype, "");
      if( clocktype == "wall" )
         bb.model().setUseElapsedTime(true);

      bb(*bonmin_setup);

      /* store solve statistics */
      gmoSetHeadnTail(gmo, gmoHresused,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
      gmoSetHeadnTail(gmo, gmoTmipnod,   bb.numNodes());
      gmoSetHeadnTail(gmo, gmoHiterused, bb.iterationCount());
      gmoSetHeadnTail(gmo, gmoHdomused,  static_cast<double>(minlp->nlp->domviolations));
      double best_bound = minlp->isMin * bb.model().getBestPossibleObjValue(); //bestBound();
      if( best_bound > -1e200 && best_bound < 1e200 )
         gmoSetHeadnTail(gmo, gmoTmipbest, best_bound);
      gmoModelStatSet(gmo, minlp->model_status);
      gmoSolveStatSet(gmo, minlp->solver_status);

      if( solvetrace_ != NULL )
         GAMSsolvetraceAddEndLine(solvetrace_, bb.numNodes(), best_bound,
            bb.bestSolution() != NULL ? minlp->isMin * bb.bestObj() : minlp->isMin * bb.model().getInfinity());

      // store primal solution in gmo
      if( bb.bestSolution() != NULL )
      {
         char buf[100];
         snprintf(buf, 100, "\nBonmin finished. Found feasible solution. Objective function value = %g.", minlp->isMin * bb.bestObj());
         gevLogStat(gev, buf);

#if GMOAPIVERSION < 12
         double* lambda = new double[gmoM(gmo)];
         for( int i = 0; i < gmoM(gmo); ++i )
            lambda[i] = gmoValNA(gmo);

         /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
         gmoSetSolution2(gmo, bb.bestSolution(), lambda);

         delete[] lambda;
#else
         gmoSetSolutionPrimal(gmo, bb.bestSolution());
#endif
      }
      else
      {
         gevLogStat(gev, "\nBonmin finished. No feasible solution found.");
      }

      // print eval statistics, if requested
      bool printnevals;
      bonmin_setup->options()->GetBoolValue("print_funceval_statistics", printnevals, "bonmin.");
      if( printnevals )
      {
         char buffer[100];
         gevLog(gev, "");
         sprintf(buffer, "Number of evaluations at new points              = %ld", minlp->get_numeval_newpoint());       gevLog(gev, buffer);
         sprintf(buffer, "Number of objective function evaluations         = %ld", minlp->get_numeval_obj());            gevLog(gev, buffer);
         sprintf(buffer, "Number of objective gradient evaluations         = %ld", minlp->get_numeval_objgrad());        gevLog(gev, buffer);
         sprintf(buffer, "Number of all constraint functions evaluations   = %ld", minlp->get_numeval_cons());           gevLog(gev, buffer);
         sprintf(buffer, "Number of constraint jacobian evaluations        = %ld", minlp->get_numeval_consjac());        gevLog(gev, buffer);
         sprintf(buffer, "Number of single constraint function evaluations = %ld", minlp->get_numeval_singlecons());     gevLog(gev, buffer);
         sprintf(buffer, "Number of single constraint gradient evaluations = %ld", minlp->get_numeval_singleconsgrad()); gevLog(gev, buffer);
         sprintf(buffer, "Number of Lagrangian hessian evaluations         = %ld", minlp->get_numeval_laghess());        gevLog(gev, buffer);
         gevLog(gev, "");
      }

      // resolve MINLP with discrete variables fixed
      bool solvefinal;
      bonmin_setup->options()->GetBoolValue("solvefinal", solvefinal, "bonmin.");
      if( solvefinal && bb.bestSolution() != NULL && gmoNDisc(gmo) < gmoN(gmo) )
      {
         gevLog(gev, "Resolve with fixed discrete variables to get dual values.");

         OsiTMINLPInterface& osi_tminlp(*bonmin_setup->nonlinearSolver());
         for( Index i = 0; i < gmoN(gmo); ++i )
            if( gmoGetVarTypeOne(gmo, i) != gmovar_X )
               osi_tminlp.setColBounds(i, bb.bestSolution()[i], bb.bestSolution()[i]);
         osi_tminlp.setColSolution(bb.bestSolution());

         bool error_in_fixedsolve = false;
         try
         {
            bonmin_setup->options()->SetStringValue("print_user_options", "no", true, true);
            // let Ipopt handle fixed variables as constraints, so we get dual values for it, which seems to be expected by GAMS
            bonmin_setup->options()->SetStringValue("fixed_variable_treatment", "make_constraint", true, true);
            // since we changed fixed_variable_treatment, the NLP solved within Ipopt takes a different structure
            // calling disableWarmStart() ensures that IpoptSolver::OptimizeTNLP does not tries to ReOptimize the TNLP
            osi_tminlp.solver()->disableWarmStart();
            osi_tminlp.initialSolve();
            error_in_fixedsolve = !osi_tminlp.isProvenOptimal();
         }
         catch( TNLPSolver::UnsolvedError* E)
         {
            // there has been a failure to solve a problem with Ipopt
            char buf[1024];
            snprintf(buf, 1024, "Error: %s exited with error %s", E->solverName().c_str(), E->errorName().c_str());
            gevLogStat(gev, buf);
            error_in_fixedsolve = true;
         }
         if( !error_in_fixedsolve && fabs(gmoGetHeadnTail(gmo, gmoHobjval) - minlp->isMin*osi_tminlp.getObjValue()) > 1e-4 )
         {
            gevLog(gev, "Warning: Optimal value of NLP subproblem differ from best MINLP value reported by Bonmin.\n"
               "Will not replace solution, dual values will not be available.\n");
         }
         else if( !error_in_fixedsolve )
         {
            int n = gmoN(gmo);
            int m = gmoM(gmo);

            const double* z_L    = osi_tminlp.getRowPrice();
            const double* z_U    = z_L+n;
            const double* lambda = z_U+n;

            double* colMarg    = new double[n];
            for( Index i = 0; i < n; ++i )
            {
               // if, e.g., x_i has no lower bound, then the dual z_L[i] is -infinity
               colMarg[i] = 0;
               if( z_L[i] > gmoMinf(gmo) )
                  colMarg[i] += minlp->isMin * z_L[i];
               if( z_U[i] < gmoPinf(gmo) )
                  colMarg[i] -= minlp->isMin * z_U[i];
            }

            double* negLambda = NULL;
            if( minlp->isMin == 1.0 )
            {
               negLambda = new double[m];
               for( Index i = 0;  i < m;  i++ )
                  negLambda[i] = -lambda[i];
            }

            /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
            gmoSetSolution(gmo, osi_tminlp.getColSolution(), colMarg, negLambda != NULL ? negLambda : lambda, osi_tminlp.getRowActivity());

            delete[] colMarg;
            delete[] negLambda;
         }
         else
         {
            gevLogStat(gev, "Problems solving fixed problem. Dual variables for NLP subproblem not available.");
         }
      }

      // print solving outcome (primal/dual bounds, gap)
      gevLogStat(gev, "");
      char buf[1024];
      double best_val = gmoGetHeadnTail(gmo, gmoHobjval);
      if( bb.bestSolution() != NULL )
      {
         snprintf(buf, 1024, "Best solution: %15.6e   (%d nodes, %g seconds)\n", best_val, (int)gmoGetHeadnTail(gmo, gmoTmipnod), gmoGetHeadnTail(gmo, gmoHresused));
         gevLogStat(gev, buf);
      }
      if( best_bound > -1e200 && best_bound < 1e200 )
      {
         snprintf(buf, 1024, "Best possible: %15.6e   (only reliable for convex models)", best_bound);
         gevLogStat(gev, buf);

         if( bb.bestSolution() != NULL )
         {
            double optca;
            double optcr;
            bonmin_setup->options()->GetNumericValue("allowable_gap", optca, "bonmin.");
            bonmin_setup->options()->GetNumericValue("allowable_fraction_gap", optcr, "bonmin.");

            snprintf(buf, 255, "Absolute gap: %16.6e   (absolute tolerance optca: %g)", gmoGetAbsoluteGap(gmo), optca);
            gevLogStat(gev, buf);
            snprintf(buf, 255, "Relative gap: %16.6e   (relative tolerance optcr: %g)", gmoGetRelativeGap(gmo), optcr);
            gevLogStat(gev, buf);
         }
      }
   }
   catch( CoinError& error )
   {
      char buf[1024];
      snprintf(buf, 1024, "%s::%s\n%s", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
      gevLogStat(gev, buf);
      gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return -1;
   }
   catch( IpoptException& error )
   {
      gevLogStat(gev, error.Message().c_str());
      gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return -1;
   }
   catch( TNLPSolver::UnsolvedError* E)
   {
      // there has been a failure to solve a problem with Ipopt.
      char buf[1024];
      snprintf(buf, 1024, "Error: %s exited with error %s", E->solverName().c_str(), E->errorName().c_str());
      gevLogStat(gev, buf);
      switch( Ipopt::ApplicationReturnStatus(E->errorNum()) )
      {
         case Maximum_Iterations_Exceeded:
            gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            break;
         case Maximum_CpuTime_Exceeded:
         case Insufficient_Memory:
            gmoSolveStatSet(gmo, gmoSolveStat_Resource);
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            break;
         case Invalid_Number_Detected:
            gmoSolveStatSet(gmo, gmoSolveStat_EvalError);
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            break;
         case Restoration_Failed:
         case Error_In_Step_Computation:
         case Not_Enough_Degrees_Of_Freedom:
            gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            break;
         default:
            gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
            gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      }
      return 0;
   }
   catch( const std::bad_alloc& )
   {
      gevLogStat(gev, "Error: Not enough memory!");
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return -1;
   }
   catch( ... )
   {
      gevLogStat(gev, "Error: Unknown exception thrown.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return -1;
   }

   if( solvetrace_ != NULL )
      GAMSsolvetraceFree(&solvetrace_);

   return 0;
}

#define GAMSSOLVER_ID         bon
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
#if defined(__linux) && defined(GAMSLINKS_HAS_CPLEX)
   CPXinitialize();
#endif

   gmoInitMutexes();
   gevInitMutexes();
   palInitMutexes();
}

#ifdef GAMS_BUILD
extern "C" void mkl_finalize(void);
#endif
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
#if defined(__linux) && defined(GAMSLINKS_HAS_CPLEX)
   CPXfinalize();
#endif

   gmoFiniMutexes();
   gevFiniMutexes();
   palFiniMutexes();

#ifdef GAMS_BUILD
   mkl_finalize();
#endif
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

   *Cptr = (void*) new GamsBonmin();
   if( *Cptr == NULL )
   {
      snprintf(msgBuf, msgBufLen, "Out of memory when creating GamsBonmin object.\n");
      if( msgBufLen > 0 )
         msgBuf[msgBufLen] = '\0';
      return 0;
   }

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   assert(Cptr != NULL);

   delete (GamsBonmin*)*Cptr;
   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsBonmin*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsBonmin*)Cptr)->readyAPI(Gptr);
}
