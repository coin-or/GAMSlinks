// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsBonmin.hpp"
#include "GamsMINLP.hpp"
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsCbc.hpp"  // in case we need to solve an LP or MIP
#include "GamsIpopt.hpp" // in case we need to solve an NLP
#include "GamsCbcHeurBBTrace.hpp"
#include "GAMSlinksConfig.h"

#include "BonminConfig.h"
#include "BonBonminSetup.hpp"
#include "BonCbc.hpp"

#include <cstdio>
#include <cstring>

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#include "gclgms.h"
#ifdef GAMS_BUILD
#include "gmspal.h"  /* for audit line */
extern "C" void HSLGAMSInit();
#endif

#include "GamsCompatibility.h"

using namespace Bonmin;
using namespace Ipopt;

GamsBonmin::~GamsBonmin()
{
   delete bonmin_setup;
   delete msghandler;
}

int GamsBonmin::readyAPI(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct optRec*     opt                 /**< GAMS options object */
)
{
   this->gmo = gmo;
   assert(gmo != NULL);

   if( getGmoReady() || getGevReady() )
      return 1;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

#ifdef GAMS_BUILD
#include "coinlibdCLsvn.h"
   char buffer[512];
   auditGetLine(buffer, sizeof(buffer));
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);

   ipoptLicensed = checkIpoptLicense(gmo) && (2651979 != gevGetIntOpt(gev,gevInteger1));
   if( ipoptLicensed )
     HSLGAMSInit();
#endif

   gevLogStatPChar(gev, "\nCOIN-OR Bonmin (Bonmin Library "BONMIN_VERSION")\nwritten by P. Bonami\n");

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

   bonmin_setup->roptions()->SetRegisteringCategory("Output and log-level options", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup->roptions()->AddStringOption2("print_funceval_statistics",
      "Switch to enable printing statistics on number of evaluations of GAMS functions/gradients/Hessian.",
      "no",
      "no", "", "yes", "");

   bonmin_setup->roptions()->AddStringOption1("miptrace",
      "Name of file for writing branch-and-bound progress information.",
      "", "*", "");

   bonmin_setup->roptions()->AddLowerBoundedIntegerOption("miptracenodefreq",
      "Frequency in number of nodes for writing branch-and-bound progress information.",
      0, 100, "giving 0 disables writing of N-lines to trace file");

   bonmin_setup->roptions()->AddLowerBoundedNumberOption("miptracetimefreq",
      "Frequency in seconds for writing branch-and-bound progress information.",
      0.0, false, 5.0, "giving 0.0 disables writing of T-lines to trace file");

   bonmin_setup->roptions()->SetRegisteringCategory("Output", Bonmin::RegisteredOptions::IpoptCategory);
   bonmin_setup->roptions()->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "no",
      "no", "", "yes", "");

   bonmin_setup->roptions()->SetRegisteringCategory("NLP interface", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup->roptions()->AddStringOption2("solvefinal",
      "Switch to disable solving MINLP with discrete variables fixed to solution values after solve.",
      "yes",
      "no", "", "yes", "",
      "If enabled, then the dual values from the resolved NLP are made available in GAMS.");

   return 0;
}

int GamsBonmin::callSolver()
{
   assert(gmo != NULL);
   assert(bonmin_setup != NULL);

   if( isMIP() && gmoOptFile(gmo) == 0 )
   {
      gevLogStat(gev, "Problem is linear. Passing over to Cbc.");
      GamsCbc cbc;
      int retcode;
      retcode = cbc.readyAPI(gmo, NULL);
      if( retcode == 0 )
         retcode = cbc.callSolver();
      return retcode;
   }

   if( isNLP() && gmoOptFile(gmo) == 0 )
   {
      gevLogStat(gev, "Problem is continuous. Passing over to Ipopt.");
      GamsIpopt ipopt;
      int retcode;
      retcode = ipopt.readyAPI(gmo, NULL);
      if( retcode == 0 )
         retcode = ipopt.callSolver();
      return retcode;
   }

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
   if( gevGetIntOpt(gev, gevUseCutOff) )
      bonmin_setup->options()->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == gmoObj_Min ? gevGetDblOpt(gev, gevCutOff) : -gevGetDblOpt(gev, gevCutOff), true, true);
   bonmin_setup->options()->SetNumericValue("bonmin.allowable_gap", gevGetDblOpt(gev, gevOptCA), true, true);
   bonmin_setup->options()->SetNumericValue("bonmin.allowable_fraction_gap", gevGetDblOpt(gev, gevOptCR), true, true);
   if( gevGetIntOpt(gev, gevNodeLim) > 0 )
      bonmin_setup->options()->SetIntegerValue("bonmin.node_limit", gevGetIntOpt(gev, gevNodeLim), true, true);
   bonmin_setup->options()->SetNumericValue("bonmin.time_limit", gevGetDblOpt(gev, gevResLim), true, true);
   if( gevGetIntOpt(gev, gevIterLim) < ITERLIM_INFINITY )
      bonmin_setup->options()->SetIntegerValue("bonmin.iteration_limit", gevGetIntOpt(gev, gevIterLim), true, true);

   if( gmoNLM(gmo) == 0 && (gmoModelType(gmo) == gmoProc_qcp || gmoModelType(gmo) == gmoProc_rmiqcp || gmoModelType(gmo) == gmoProc_miqcp) )
      bonmin_setup->options()->SetStringValue("hessian_constant", "yes", true, true);

   if( ipoptLicensed )
   {
      bonmin_setup->options()->SetStringValue("linear_solver", "ma27", true, true);
      bonmin_setup->options()->SetStringValue("linear_system_scaling", "mc19", true, true);
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
   catch( std::bad_alloc )
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
#ifdef COIN_HAS_OSICPX
   std::string prefixes[7] = { "", "bonmin", "oa_decomposition.", "pump_for_minlp.", "rins.", "rens.", "local_branch." };
   for( int i = 0; i < 7; ++i )
   {
      bonmin_setup->options()->GetStringValue("milp_solver", parvalue, prefixes[i]);
      if( parvalue == "Cplex" && (!checkLicense(gmo) || !registerGamsCplexLicense(gmo)) )
      {
         gevLogStat(gev, "No valid CPLEX license found.");
         return 1;
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

   // initialize Hessian in GMO, if required
   bool hessian_is_approx = false;
   bonmin_setup->options()->GetStringValue("hessian_approximation", parvalue, "");
   if( parvalue == "exact" )
   {
      int do2dir = 1;
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
   gmoEvalErrorNoMsg(gmo, printevalerror);

   // set number of threads in blas in ipopt
   setNumThreadsBlas(gev, gevThreads(gev));

   // initialize bbtrace, if activated
   GAMS_BBTRACE* bbtrace = NULL;
   std::string miptrace;
   bonmin_setup->options()->GetStringValue("miptrace", miptrace, "");
   if( miptrace != "" )
   {
      int nodefreq;
      double timefreq;
      int rc;

      bonmin_setup->options()->GetIntegerValue("miptracenodefreq", nodefreq, "");
      bonmin_setup->options()->GetNumericValue("miptracetimefreq", timefreq, "");
      rc = GAMSbbtraceCreate(&bbtrace, miptrace.c_str(), "Bonmin "BONMIN_VERSION, ipoptinf, nodefreq, timefreq);
      if( rc != 0 )
      {
         gevLogStat(gev, "Initializing miptrace failed.");
         GAMSbbtraceFree(&bbtrace);
      }
      else
      {
         bonmin_setup->heuristics().push_back(BabSetupBase::HeuristicMethod());
         BabSetupBase::HeuristicMethod& heurmeth(bonmin_setup->heuristics().back());
         heurmeth.heuristic = new GamsCbcHeurBBTrace(bbtrace, minlp->isMin);
         heurmeth.id = "MIPtrace writing heuristic";
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

         if( bbtrace != NULL )
            GAMSbbtraceSetInfinity(bbtrace, first_osi_tminlp.getInfinity());
      }

      // try solving
      Bab bb;

      minlp->nlp->clockStart = gevTimeDiffStart(gev);
      bb(*bonmin_setup);

      /* store solve statistics */
      gmoSetHeadnTail(gmo, gmoHresused,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
      gmoSetHeadnTail(gmo, gmoTmipnod,   bb.numNodes());
      gmoSetHeadnTail(gmo, gmoHiterused, bb.iterationCount());
      gmoSetHeadnTail(gmo, gmoHdomused,  static_cast<double>(minlp->nlp->domviolations));
      double best_bound = minlp->isMin * bb.bestBound();
      if( best_bound > -1e200 && best_bound < 1e200 )
         gmoSetHeadnTail(gmo, gmoTmipbest, best_bound);
      gmoModelStatSet(gmo, minlp->model_status);
      gmoSolveStatSet(gmo, minlp->solver_status);

      if( bbtrace != NULL )
         GAMSbbtraceAddEndLine(bbtrace, bb.numNodes(), best_bound,
            bb.bestSolution() != NULL ? minlp->isMin * bb.bestObj() : minlp->isMin * bb.model().getInfinity());

      // store primal solution in gmo
      if( bb.bestSolution() != NULL )
      {
         char buf[100];
         snprintf(buf, 100, "\nBonmin finished. Found feasible solution. Objective function value = %g.", minlp->isMin * bb.bestObj());
         gevLogStat(gev, buf);

         double* negLambda = new double[gmoM(gmo)];
         memset(negLambda, 0, gmoM(gmo)*sizeof(double));

         gmoSetSolution2(gmo, bb.bestSolution(), negLambda);
         gmoSetHeadnTail(gmo, gmoHobjval, minlp->isMin * bb.bestObj());

         delete[] negLambda;
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

            gmoSetSolution(gmo, osi_tminlp.getColSolution(), colMarg, negLambda != NULL ? negLambda : lambda, osi_tminlp.getRowActivity());
            gmoSetHeadnTail(gmo, gmoHobjval,  minlp->isMin * osi_tminlp.getObjValue());

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

            snprintf(buf, 255, "Absolute gap: %16.6e   (absolute tolerance optca: %g)", CoinAbs(best_val-best_bound), optca);
            gevLogStat(gev, buf);
            snprintf(buf, 255, "Relative gap: %16.6e   (relative tolerance optcr: %g)", CoinAbs(best_val-best_bound)/CoinMax(CoinAbs(best_bound), 1.0), optcr);
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
      gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return 0;
   }
   catch( std::bad_alloc )
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

   if( bbtrace != NULL )
      GAMSbbtraceFree(&bbtrace);

   return 0;
}

bool GamsBonmin::isNLP()
{
   switch( gmoModelType(gmo) )
   {
      case gmoProc_lp:
      case gmoProc_rmip:
      case gmoProc_qcp:
      case gmoProc_rmiqcp:
      case gmoProc_nlp:
      case gmoProc_rminlp:
         return true;
   }

   if( gmoNDisc(gmo) > 0 )
      return false;

   int numSos1, numSos2, nzSos;
   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
   if( nzSos > 0 )
      return false;

   return true;
}

bool GamsBonmin::isMIP()
{
   return gmoNLNZ(gmo) == 0 && (gmoObjStyle(gmo) == gmoObjType_Var || gmoObjNLNZ(gmo) == 0);
}

#define GAMSSOLVERC_ID         bon
#define GAMSSOLVERC_CLASS      GamsBonmin
#include "GamsSolverC_tpl.cpp"
