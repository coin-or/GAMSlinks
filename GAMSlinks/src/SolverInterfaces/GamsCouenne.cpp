// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsCouenne.hpp"
#include "GamsMINLP.hpp"
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsNLinstr.h"
#include "GamsCbc.hpp"
#include "GamsCbcHeurSolveTrace.hpp"
#include "GAMSlinksConfig.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <list>

// dos compiler does not know PI
#ifndef M_PI
#define M_PI           3.141592653589793238462643
#endif

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#include "gclgms.h"
#ifdef GAMS_BUILD
#include "palmcc.h"
#endif

#include "GamsCompatibility.h"

#include "CbcBranchActual.hpp"  // for CbcSOS
#include "CbcBranchLotsize.hpp" // for CbcLotsize
#include "CouenneBab.hpp"
#include "BonCouenneSetup.hpp"
#include "BonCouenneInterface.hpp"
#include "CouenneProblem.hpp"
#include "CouenneTypes.hpp"
#include "CouenneExprClone.hpp"
#include "CouenneExprGroup.hpp"
#include "CouenneExprAbs.hpp"
#include "CouenneExprConst.hpp"
#include "CouenneExprCos.hpp"
#include "CouenneExprDiv.hpp"
#include "CouenneExprExp.hpp"
#include "CouenneExprInv.hpp"
#include "CouenneExprLog.hpp"
//#include "CouenneExprMax.hpp"
//#include "CouenneExprMin.hpp"
#include "CouenneExprMul.hpp"
#include "CouenneExprOpp.hpp"
#include "CouenneExprPow.hpp"
#include "CouenneExprSin.hpp"
#include "CouenneExprSub.hpp"
#include "CouenneExprSum.hpp"
#include "CouenneExprVar.hpp"
//#include "exprQuad.hpp"
//#include "lqelems.hpp"

using namespace Couenne;
using namespace Bonmin;
using namespace Ipopt;

GamsCouenne::~GamsCouenne()
{
   delete couenne_setup;
   delete msghandler;

#ifdef GAMS_BUILD
   if( pal != NULL )
      palFree(&pal);
#endif
}

int GamsCouenne::readyAPI(
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

   assert(pal == NULL);

#ifdef GAMS_BUILD
   char buffer[GMS_SSSIZE];

   if( !palCreate(&pal, buffer, sizeof(buffer)) )
      return 1;

#define PALPTR pal
#include "coinlibdCL2svn.h" 
   palGetAuditLine(pal, buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

   initLicensing(gmo, pal);
   ipoptlicensed = HSLInit(gmo, pal);

   gevLogStatPChar(gev, "\nCOIN-OR Couenne (Couenne Library "COUENNE_VERSION")\nwritten by P. Belotti\n\n");

   delete couenne_setup;
   couenne_setup = new CouenneSetup();

   SmartPtr<Journalist> jnlst = new Journalist();
   SmartPtr<Journal> jrnl = new GamsJournal(gev, "console", J_ITERSUMMARY, J_STRONGWARNING);
   jrnl->SetPrintLevel(J_DBG, J_NONE);
   if( !jnlst->AddJournal(jrnl) )
      gevLogStat(gev, "Failed to register GamsJournal for IPOPT output.");

   SmartPtr<Bonmin::RegisteredOptions> roptions = new Bonmin::RegisteredOptions();
   SmartPtr<Ipopt::OptionsList> options = new Ipopt::OptionsList(GetRawPtr(roptions), jnlst);

   couenne_setup->setOptionsAndJournalist(roptions, options, jnlst);
   couenne_setup->registerOptions();

   couenne_setup->roptions()->AddStringOption1("solvetrace",
      "Name of file for writing solving progress information.",
      "", "*", "");

   couenne_setup->roptions()->AddLowerBoundedIntegerOption("solvetracenodefreq",
      "Frequency in number of nodes for writing solving progress information.",
      0, 100, "giving 0 disables writing of N-lines to trace file");

   couenne_setup->roptions()->AddLowerBoundedNumberOption("solvetracetimefreq",
      "Frequency in seconds for writing solving progress information.",
      0.0, false, 5.0, "giving 0.0 disables writing of T-lines to trace file");


   couenne_setup->roptions()->SetRegisteringCategory("Output", Bonmin::RegisteredOptions::IpoptCategory);
   couenne_setup->roptions()->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "yes",
      "no", "", "yes", "");

   return 0;
}

int GamsCouenne::callSolver()
{
   assert(gmo != NULL);
   assert(couenne_setup != NULL);

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

   gmoInterfaceSet(gmo, gmoIFace_Raw);
   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);
   // get rid of free rows
   if( gmoGetEquTypeCnt(gmo, gmoequ_N) )
      gmoSetNRowPerm(gmo);

   if( gmoN(gmo) == 0 )
   {
      gevLogStat(gev, "Error: Couenne requires variables.");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   // Couenne cannot handle semicontinuous, semiinteger or SOS
   // we could pass the CbcObjects to the B&B model, but it seems that the constraints need to be present also in CouenneProblem, which seems not possible in general
   if( gmoGetVarTypeCnt(gmo, gmovar_SC) > 0 || gmoGetVarTypeCnt(gmo, gmovar_SI) > 0 )
   {
      gevLogStat(gev, "ERROR: Semicontinuous and semiinteger variables not supported by Couenne.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   if( gmoGetVarTypeCnt(gmo, gmovar_S1) > 0 || gmoGetVarTypeCnt(gmo, gmovar_S2) > 0 )
   {
      gevLogStat(gev, "ERROR: Special ordered sets not supported by Couenne.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   // Change some options
   couenne_setup->options()->clear();
   couenne_setup->options()->SetNumericValue("bound_relax_factor", 1e-10, true, true);
   couenne_setup->options()->SetStringValue("ma86_order", "auto", true, true);
   if( gevGetIntOpt(gev, gevUseCutOff) )
      couenne_setup->options()->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == gmoObj_Min ? gevGetDblOpt(gev, gevCutOff) : -gevGetDblOpt(gev, gevCutOff), true, true);
   couenne_setup->options()->SetNumericValue("bonmin.allowable_gap", gevGetDblOpt(gev, gevOptCA), true, true);
   couenne_setup->options()->SetNumericValue("bonmin.allowable_fraction_gap", gevGetDblOpt(gev, gevOptCR), true, true);
   if( gevGetIntOpt(gev, gevNodeLim) > 0 )
      couenne_setup->options()->SetIntegerValue("bonmin.node_limit", gevGetIntOpt(gev, gevNodeLim), true, true);
   couenne_setup->options()->SetNumericValue("bonmin.time_limit", gevGetDblOpt(gev, gevResLim), true, true);
   if( gevGetIntOpt(gev, gevIterLim) < ITERLIM_INFINITY )
      couenne_setup->options()->SetIntegerValue("bonmin.iteration_limit", gevGetIntOpt(gev, gevIterLim), true, true);
   couenne_setup->options()->SetIntegerValue("bonmin.problem_print_level", J_STRONGWARNING, true, true); /* otherwise Couenne prints the problem to stdout */
   if( ipoptlicensed )
   {
      couenne_setup->options()->SetStringValue("linear_solver", "ma27", true, true);
      couenne_setup->options()->SetStringValue("linear_system_scaling", "mc19", true, true);
      //gevLog(gev, "Enabled use of HSL MA27 and MC19 from commercially supported Ipopt.\n");
   }

   // workaround for bug in couenne reformulation: if there are tiny constants, delete_redundant might setup a nonstandard reformulation (e.g., using x*x instead of x^2)
   // thus, we change the default of delete_redundant to off in this case
   double* constants = (double*)gmoPPool(gmo);
   int constantlen = gmoNLConst(gmo);
   for( int i = 0; i < constantlen; ++i )
      if( CoinAbs(constants[i]) < COUENNE_EPS )
      {
         couenne_setup->options()->SetStringValue("delete_redundant", "no", "couenne.");
         break;
      }

   if( gmoNLM(gmo) == 0 && (gmoModelType(gmo) == gmoProc_qcp || gmoModelType(gmo) == gmoProc_rmiqcp || gmoModelType(gmo) == gmoProc_miqcp) )
      couenne_setup->options()->SetStringValue("hessian_constant", "yes", true, true);

   // forbid overwriting Ipopt's default for +/-infinity... why did I do this?
//   double ipoptinf;
//   options->GetNumericValue("nlp_lower_bound_inf", ipoptinf, "");
//   options->SetNumericValue("nlp_lower_bound_inf", ipoptinf, false, true);
//   gmoMinfSet(gmo, ipoptinf);
//   options->GetNumericValue("nlp_upper_bound_inf", ipoptinf, "");
//   options->SetNumericValue("nlp_upper_bound_inf", ipoptinf, false, true);
//   gmoPinfSet(gmo, ipoptinf);

   // set GMO's and Ipopt's value for infinity to Couenne's value for infinity
   gmoMinfSet(gmo, -COUENNE_INFINITY);
   gmoPinfSet(gmo,  COUENNE_INFINITY);
   couenne_setup->options()->SetNumericValue("nlp_lower_bound_inf", -COUENNE_INFINITY, false, true);
   couenne_setup->options()->SetNumericValue("nlp_upper_bound_inf",  COUENNE_INFINITY, false, true);

   // process options file
   try
   {
      if( gmoOptFile(gmo) )
      {
         char buffer[GMS_SSSIZE];
         couenne_setup->options()->SetStringValue("print_user_options", "yes", true, true);
         gmoNameOptFile(gmo, buffer);
         couenne_setup->BabSetupBase::readOptionsFile(buffer);
      }
      else
      {
         // need to let Couenne read something, otherwise it will try to read its default option file couenne.opt
         couenne_setup->readOptionsString(std::string());
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

   // check for CPLEX license, if used
   std::string parvalue;
   couenne_setup->options()->GetStringValue("lp_solver", parvalue, "");
#ifdef COIN_HAS_OSICPX
   if( parvalue == "cplex" && !checkCplexLicense(gmo, pal) )
   {
      gevLogStat(gev, "CPLEX as LP solver chosen, but no CPLEX license available. Aborting.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_License);
      gmoModelStatSet(gmo, gmoModelStat_LicenseError);
      return 1;
   }
#endif
   if( parvalue != "clp" )
      gevLogStat(gev, "WARNING: Changing the LP solver in Couenne may give incorrect results!");

   // if feaspump is activated and should use SCIP, check for academic license of SCIP
   // also do not accept demo mode, since we do not know how large the MIPs in the feaspump will be
#ifdef COIN_HAS_SCIP
   bool usescip;
   couenne_setup->options()->GetBoolValue("feas_pump_heuristic", usescip, "couenne.");
   if( usescip )
      couenne_setup->options()->GetBoolValue("feas_pump_usescip", usescip, "couenne.");
   if( usescip && !checkScipLicense(gmo, pal) )
   {
      gevLogStat(gev, "*** Use of SCIP is limited to academic users.");
      gevLogStat(gev, "*** Please contact koch@zib.de to arrange for a license.");
      gevLogStat(gev, "Disabling use of SCIP in feasibility pump.");
      couenne_setup->options()->SetStringValue("feas_pump_usescip", "no", false, false);
   }
#endif

   bool printevalerror;
   couenne_setup->options()->GetBoolValue("print_eval_error", printevalerror, "");
   gmoEvalErrorNoMsg(gmo, !printevalerror);

   // initialize Hessian in GMO, if required
   couenne_setup->options()->GetStringValue("hessian_approximation", parvalue, "");
   if( parvalue == "exact" )
   {
      int do2dir = 1;
      int dohess = 1;
      gmoHessLoad(gmo, 0, &do2dir, &dohess);
      if( !dohess )
      {
         gevLogStat(gev, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
         couenne_setup->options()->SetStringValue("hessian_approximation", "limited-memory");
      }
   }

   // setup Couenne Problem
   CouenneProblem* problem = setupProblem();
   if( problem == NULL )
   {
      gevLogStat(gev, "Error in setting up problem for Couenne.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 1;
   }

   // setup MINLP
   SmartPtr<GamsMINLP> minlp = new GamsMINLP(gmo);
   minlp->in_couenne = true;
   couenne_setup->options()->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");

   // set number of threads in linear algebra in ipopt
   setNumThreadsLinearAlgebra(gev, gevThreads(gev));

   // initialize solvetrace, if activated
   GAMS_SOLVETRACE* solvetrace_ = NULL;
   std::string solvetrace;
   couenne_setup->options()->GetStringValue("solvetrace", solvetrace, "");
   if( solvetrace != "" )
   {
      int nodefreq;
      double timefreq;
      int rc;

      couenne_setup->options()->GetIntegerValue("solvetracenodefreq", nodefreq, "");
      couenne_setup->options()->GetNumericValue("solvetracetimefreq", timefreq, "");
      rc = GAMSsolvetraceCreate(&solvetrace_, solvetrace.c_str(), "Couenne "COUENNE_VERSION, COUENNE_INFINITY, nodefreq, timefreq);
      if( rc != 0 )
      {
         gevLogStat(gev, "Initializing solvetrace failed.");
         GAMSsolvetraceFree(&solvetrace_);
      }
      else
      {
         couenne_setup->heuristics().push_back(BabSetupBase::HeuristicMethod());
         BabSetupBase::HeuristicMethod& heurmeth(couenne_setup->heuristics().back());
         heurmeth.heuristic = new GamsCbcHeurSolveTrace(solvetrace_, minlp->isMin);
         heurmeth.id = "Solvetrace writing heuristic";
      }
   }

   if( msghandler == NULL )
      msghandler = new GamsMessageHandler(gev);

   if( minlp->isMin == -1 )
      gevLog(gev, "Note: Maximization problem reformulated as minimization problem for Couenne, objective values are negated in output.");

   char buffer[1024];
   try
   {
      CouenneBab bb;
      bb.setProblem(problem);

      // setup CouenneInterface, so that we can pass in our message handler
      CouenneInterface* ci = new CouenneInterface;
      ci->initialize(couenne_setup->roptions(), couenne_setup->options(), couenne_setup->journalist(), GetRawPtr(minlp));
      ci->passInMessageHandler(msghandler);

      // not sufficient to support SOS/SemiCon/SemiInt: passSOSSemiCon(ci, &bb.model());

      minlp->nlp->clockStart = gevTimeDiffStart(gev);
      if( solvetrace_ != NULL )
         GAMSsolvetraceResetStarttime(solvetrace_);

      // initialize Couenne, e.g., reformulate problem
      // pass also bb in case user enabled recognition of SOS in problem
      if( !couenne_setup->InitializeCouenne(NULL, problem, GetRawPtr(minlp), ci, &bb) )
      {
         gevLogStat(gev, "Reformulation finds model infeasible.\n");
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
         delete problem;
         goto TERMINATE;
      }

      // check time usage and reduce timelimit for B&B accordingly
      double preprocessTime = gevTimeDiffStart(gev) - minlp->nlp->clockStart;

      snprintf(buffer, 1024, "Couenne initialized (%g seconds).\n\n", preprocessTime);
      gevLogStatPChar(gev, buffer);

      double reslim = gevGetDblOpt(gev, gevResLim);
      if( preprocessTime >= reslim )
      {
         gevLogStat(gev, "Time is up.\n");
         gmoSetHeadnTail(gmo, gmoHresused,  preprocessTime);
         gmoSetHeadnTail(gmo, gmoTmipnod,   0);
         gmoSetHeadnTail(gmo, gmoHiterused, 0);
         gmoSetHeadnTail(gmo, gmoHdomused,  static_cast<double>(minlp->nlp->domviolations));
         gmoSetHeadnTail(gmo, gmoTmipbest,  gmoValNA(gmo));
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         goto TERMINATE;
      }

      couenne_setup->options()->SetNumericValue("bonmin.time_limit", reslim - preprocessTime, true, true);
      couenne_setup->setDoubleParameter(BabSetupBase::MaxTime, reslim - preprocessTime);

      if( solvetrace_ != NULL )
         GAMSsolvetraceSetInfinity(solvetrace_, couenne_setup->continuousSolver()->getInfinity());

      // do branch and bound
      bb(*couenne_setup);

      // check optimization outcome
      double best_val   = minlp->isMin * bb.bestObj();
      double best_bound = minlp->isMin * bb.model().getBestPossibleObjValue(); //bestBound();
      bool havesol;
      switch( minlp->model_status )
      {
         case gmoModelStat_OptimalGlobal:
         case gmoModelStat_OptimalLocal:
         case gmoModelStat_Integer:
            havesol = true;
            break;
         default:
            havesol = false;
            break;
      }
      assert(bb.bestSolution() != NULL || !havesol);

      // correct best_bound, if necessary (works around sideeffect of other workaround in couenne)
      if( minlp->isMin * best_bound > minlp->isMin * best_val )
         best_bound = best_val;

      gmoSetHeadnTail(gmo, gmoHresused,  gevTimeDiffStart(gev) - minlp->nlp->clockStart);
      gmoSetHeadnTail(gmo, gmoTmipnod,   bb.numNodes());
      gmoSetHeadnTail(gmo, gmoHiterused, bb.iterationCount());
      gmoSetHeadnTail(gmo, gmoHdomused,  static_cast<double>(minlp->nlp->domviolations));
      if( best_bound > -1e200 && best_bound < 1e200 )
         gmoSetHeadnTail(gmo, gmoTmipbest, best_bound);
      gmoModelStatSet(gmo, minlp->model_status);
      gmoSolveStatSet(gmo, minlp->solver_status);

      if( solvetrace_ != NULL )
         GAMSsolvetraceAddEndLine(solvetrace_, bb.numNodes(), best_bound,
            havesol ? minlp->isMin * bb.bestObj() : minlp->isMin * bb.model().getInfinity());

      if( havesol )
      {
         gevLogStat(gev, "\nCouenne finished. Found feasible solution.");

#if GMOAPIVERSION < 12
         double* lambda = new double[gmoM(gmo)];
         for( int i = 0; i < gmoM(gmo); ++i )
            lambda[i] = gmoValNA(gmo);

         /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
         gmoSetSolution2(gmo, bb.bestSolution(), lambda);

         delete[] lambda;
#else
         /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
         gmoSetSolutionPrimal(gmo, bb.bestSolution());
#endif
      }
      else
      {
         gevLogStat(gev, "\nCouenne finished. No feasible solution found.");
      }

      // print solving outcome (primal/dual bounds, gap)
      gevLogStat(gev, "");
      if( havesol )
      {
         snprintf(buffer, sizeof(buffer), "Best solution: %15.6e   (%d nodes, %g seconds)\n", best_val, bb.numNodes(), gmoGetHeadnTail(gmo, gmoHresused));
         gevLogStat(gev, buffer);
      }
      if( best_bound > -1e200 && best_bound < 1e200 )
      {
         snprintf(buffer, sizeof(buffer), "Best possible: %15.6e", best_bound);
         gevLogStat(gev, buffer);

         if( havesol )
         {
            double optca;
            double optcr;
            couenne_setup->options()->GetNumericValue("allowable_gap", optca, "bonmin.");
            couenne_setup->options()->GetNumericValue("allowable_fraction_gap", optcr, "bonmin.");

            snprintf(buffer, sizeof(buffer), "Absolute gap: %16.6e   (absolute tolerance optca: %g)", CoinAbs(best_val-best_bound), optca);
            gevLogStat(gev, buffer);
            snprintf(buffer, sizeof(buffer), "Relative gap: %16.6e   (relative tolerance optcr: %g)", CoinAbs(best_val-best_bound)/CoinMax(CoinAbs(best_bound), 1.0), optcr);
            gevLogStat(gev, buffer);
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

   if( gmoModelType(gmo) == gmoProc_cns )
      switch( gmoModelStat(gmo) )
      {
         case gmoModelStat_OptimalGlobal:
         case gmoModelStat_OptimalLocal:
         case gmoModelStat_NonOptimalIntermed:
         case gmoModelStat_Integer:
            gmoModelStatSet(gmo, gmoModelStat_Solved);
   }

TERMINATE:
   if( solvetrace_ != NULL )
      GAMSsolvetraceFree(&solvetrace_);

   return 0;
}

bool GamsCouenne::isMIP()
{
   return gmoNLNZ(gmo) == 0 && (gmoObjStyle(gmo) == gmoObjType_Var || gmoObjNLNZ(gmo) == 0);
}

CouenneProblem* GamsCouenne::setupProblem()
{
   assert(gmo != NULL);
   assert(couenne_setup != NULL);

   CouenneProblem* prob = new CouenneProblem(NULL, NULL, couenne_setup->journalist());
   CouenneProblem* prob_ = NULL;

   gmoUseQSet(gmo, 1);

   double isMin = (gmoSense(gmo) == gmoObj_Min) ? 1.0 : -1.0;

   int nz;
   int nlnz;

   double* lincoefs  = new double[gmoN(gmo)];
   int*    lincolidx = new int[gmoN(gmo)];
   int*    nlflag    = new int[gmoN(gmo)];

   double* quadcoefs = new double[gmoMaxQnz(gmo)];
   int* qrow         = new int[gmoMaxQnz(gmo)];
   int* qcol         = new int[gmoMaxQnz(gmo)];
   int qnz;

   int* opcodes = new int[gmoMaxSingleFNL(gmo)+1];
   int* fields  = new int[gmoMaxSingleFNL(gmo)+1];
   int constantlen = gmoNLConst(gmo);
   double* constants = (double*)gmoPPool(gmo);
   int codelen;

   exprGroup::lincoeff lin;
   expression* body = NULL;

   double* x_ = new double[gmoN(gmo)];
   double* lb = new double[gmoN(gmo)];
   double* ub = new double[gmoN(gmo)];

   gmoGetVarL(gmo, x_);
   gmoGetVarLower(gmo, lb);
   gmoGetVarUpper(gmo, ub);

   // add variables
   for( int i = 0; i < gmoN(gmo); ++i )
   {
      switch( gmoGetVarTypeOne(gmo, i) )
      {
         case gmovar_SC:
            lb[i] = 0.0;
         case gmovar_X:
         case gmovar_S1:
         case gmovar_S2:
            prob->addVariable(false, prob->domain());
            break;

         case gmovar_SI:
            ub[i] = 0.0;
         case gmovar_B:
         case gmovar_I:
            prob->addVariable(true, prob->domain());
            break;

         default:
            gevLogStat(gev, "Unknown variable type.");
            goto TERMINATE;
      }
   }

   // add variable bounds and initial values
   prob->domain()->push(gmoN(gmo), x_, lb, ub);

   // add objective function
   gmoGetObjSparse(gmo, lincolidx, lincoefs, nlflag, &nz, &nlnz);

   if( gmoGetObjOrder(gmo) <= gmoorder_Q )
   {
      // linear or quadratic objective
      lin.reserve(nz);
      for( int i = 0; i < nz; ++i )
         lin.push_back(std::pair<exprVar*, CouNumber>(prob->Var(lincolidx[i]), isMin*lincoefs[i]));

      if( gmoGetObjOrder(gmo) == gmoorder_Q )
      {
         // TODO: use exprQuad?
         qnz = gmoObjQNZ(gmo);
         gmoGetObjQ(gmo, qcol, qrow, quadcoefs);
         expression** quadpart = new expression*[qnz];

         for( int j = 0; j < qnz; ++j )
         {
            assert(qcol[j] >= 0);
            assert(qrow[j] >= 0);
            assert(qcol[j] < gmoN(gmo));
            assert(qrow[j] < gmoN(gmo));
            if( qcol[j] == qrow[j] )
            {
               quadcoefs[j] /= 2.0; // for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO
               quadpart[j] = new exprPow(new exprClone(prob->Var(qcol[j])), new exprConst(2.0));
            }
            else
            {
               quadpart[j] = new exprMul(new exprClone(prob->Var(qcol[j])), new exprClone(prob->Var(qrow[j])));
            }
            quadcoefs[j] *= isMin;
            if( quadcoefs[j] == -1.0 )
               quadpart[j] = new exprOpp(quadpart[j]);
            else if( quadcoefs[j] != 1.0 )
               quadpart[j] = new exprMul(quadpart[j], new exprConst(quadcoefs[j]));
         }

         body = new exprGroup(isMin*gmoObjConst(gmo), lin, quadpart, qnz);
      }
      else
      {
         body = new exprGroup(isMin*gmoObjConst(gmo), lin, NULL, 0);
      }
   }
   else
   {
      // general nonlinear objective
      lin.reserve(nz-nlnz);
      for( int i = 0; i < nz; ++i )
      {
         if( nlflag[i] )
            continue;
         lin.push_back(std::pair<exprVar*, CouNumber>(prob->Var(lincolidx[i]), isMin*lincoefs[i]));
      }

      gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);

      expression** nl = new expression*[1];
      if( (gevGetIntOpt(gev, gevInteger1) & 0x4) && (2651979 != gevGetIntOpt(gev, gevInteger1)) )
         std::clog << "parse instructions for objective" << std::endl;
      nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
      if( nl[0] == NULL )
         goto TERMINATE;

      // multiply with -isMin/objjacval
      double objjacval = isMin*gmoObjJacVal(gmo);
      if( objjacval == 1.0 )
         nl[0] = new exprOpp(nl[0]);
      else if( objjacval != -1.0 )
         nl[0] = new exprMul(nl[0], new exprConst(-1.0/objjacval));

      body = new exprGroup(isMin*gmoObjConst(gmo), lin, nl, 1);
   }

   prob->addObjective(body, "min");

   // add constraints
   for( int i = 0; i < gmoM(gmo); ++i )
   {
      gmoGetRowSparse(gmo, i, lincolidx, lincoefs, nlflag, &nz, &nlnz);
      lin.clear();

      if( gmoGetEquOrderOne(gmo, i) <= gmoorder_Q )
      {
         // linear or quadratic constraint
         lin.reserve(nz);
         for (int j = 0; j < nz; ++j)
            lin.push_back(std::pair<exprVar*, CouNumber>(prob->Var(lincolidx[j]), lincoefs[j]));

         if( gmoGetEquOrderOne(gmo, i) == gmoorder_Q ) {
            qnz = gmoGetRowQNZOne(gmo,i);
            gmoGetRowQ(gmo, i, qcol, qrow, quadcoefs);
            expression** quadpart = new expression*[qnz];

            for( int j = 0; j < qnz; ++j )
            {
               assert(qcol[j] >= 0);
               assert(qrow[j] >= 0);
               assert(qcol[j] < gmoN(gmo));
               assert(qrow[j] < gmoN(gmo));
               if( qcol[j] == qrow[j] )
               {
                  quadcoefs[j] /= 2.0; // for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO.
                  quadpart[j] = new exprPow(new exprClone(prob->Var(qcol[j])), new exprConst(2.0));
               }
               else
               {
                  quadpart[j] = new exprMul(new exprClone(prob->Var(qcol[j])), new exprClone(prob->Var(qrow[j])));
               }
               if( quadcoefs[j] == -1.0 )
                  quadpart[j] = new exprOpp(quadpart[j]);
               else if( quadcoefs[j] != 1.0 )
                  quadpart[j] = new exprMul(quadpart[j], new exprConst(quadcoefs[j]));
            }

            body = new exprGroup(0, lin, quadpart, qnz);
         }
         else
         {
            body = new exprGroup(0, lin, NULL, 0);
         }
      }
      else
      {
         // general nonlinear constraint
         lin.reserve(nz-nlnz);
         for( int j = 0; j < nz; ++j )
         {
            if( nlflag[j] )
               continue;
            lin.push_back(std::pair<exprVar*, CouNumber>(prob->Var(lincolidx[j]), lincoefs[j]));
         }

         gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
         expression** nl = new expression*[1];
         if( (gevGetIntOpt(gev, gevInteger1)) & 0x4 && (2651979 != gevGetIntOpt(gev, gevInteger1)) )
            std::clog << "parse instructions for constraint " << i << std::endl;
         nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
         if( nl[0] == NULL )
            goto TERMINATE;

         body = new exprGroup(0, lin, nl, 1);
      }

      switch( gmoGetEquTypeOne(gmo, i) )
      {
         case gmoequ_E:
            prob->addEQConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
            break;
         case gmoequ_L:
            prob->addLEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
            break;
         case gmoequ_G:
            prob->addGEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
            break;
         case gmoequ_N:
            // ignore free constraints, should have been permutated out anyway
            break;
         case gmoequ_X:
            gevLogStat(gev, "External functions not supported by Couenne.\n");
            goto TERMINATE;
         case gmoequ_C:
            gevLogStat(gev, "Conic constraints not supported by Couenne.\n");
            goto TERMINATE;
         case gmoequ_B:
            gevLogStat(gev, "Logic constraints not supported by Couenne.\n");
            goto TERMINATE;
         default:
            gevLogStat(gev, "Unexpected constraint type.\n");
            goto TERMINATE;
      }
   }

   if( (gevGetIntOpt(gev, gevInteger1) & 0x2) && (2651979 != gevGetIntOpt(gev, gevInteger1)) )
      prob->print();
   prob->initOptions(couenne_setup->options());

   // if we made it until here, then we are successful, so store prob in prob_
   prob_ = prob;

TERMINATE:
   delete[] lincolidx;
   delete[] lincoefs;
   delete[] nlflag;

   delete[] quadcoefs;
   delete[] qrow;
   delete[] qcol;

   delete[] opcodes;
   delete[] fields;

   delete[] x_;
   delete[] lb;
   delete[] ub;

   if( prob_ == NULL )
      delete prob;

   gmoUseQSet(gmo, 0);

   return prob_;
}

Couenne::expression* GamsCouenne::parseGamsInstructions(
   Couenne::CouenneProblem* prob,         /**< Couenne problem that holds variables */
   int                codelen,            /**< length of GAMS instructions */
   int*               opcodes,            /**< opcodes of GAMS instructions */
   int*               fields,             /**< fields of GAMS instructions */
   int                constantlen,        /**< length of GAMS constants pool */
   double*            constants           /**< GAMS constants pool */
)
{
   bool debugoutput = (gevGetIntOpt(gev, gevInteger1) & 0x4) && (2651979 != gevGetIntOpt(gev, gevInteger1));
#define debugout if( debugoutput ) std::clog

   std::list<expression*> stack;

   int nargs = -1;

   for( int pos = 0; pos < codelen; ++pos )
   {
      GamsOpCode opcode = (GamsOpCode)opcodes[pos];
      int address = fields[pos]-1;

      debugout << '\t' << GamsOpCodeName[opcode] << ": ";

      expression* exp = NULL;

      switch( opcode )
      {
         case nlNoOp:   // no operation
         case nlStore:  // store row
         case nlHeader: // header
         {
            debugout << "ignored" << std::endl;
            break;
         }

         case nlPushV: // push variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "push variable " << address << std::endl;
            exp = new exprClone(prob->Variables()[address]);
            break;
         }

         case nlPushI: // push constant
         {
            debugout << "push constant " << constants[address] << std::endl;
            exp = new exprConst(constants[address]);
            break;
         }

         case nlPushZero: // push zero
         {
            debugout << "push constant zero" << std::endl;
            exp = new exprConst(0.0);
            break;
         }

         case nlAdd: // add
         {
            debugout << "add" << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = stack.back(); stack.pop_back();
            exp = new exprSum(term1, term2);
            break;
         }

         case nlAddV: // add variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "add variable " << address << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprClone(prob->Variables()[address]);
            exp = new exprSum(term1, term2);
            break;
         }

         case nlAddI: // add immediate
         {
            debugout << "add constant " << constants[address] << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprConst(constants[address]);
            exp = new exprSum(term1, term2);
            break;
         }

         case nlSub: // minus
         {
            debugout << "minus" << std::endl;
            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = stack.back(); stack.pop_back();
            exp = new exprSub(term2, term1);
            break;
         }

         case nlSubV: // substract variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "substract variable " << address << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprClone(prob->Variables()[address]);
            exp = new exprSub(term1, term2);
            break;
         }

         case nlSubI: // substract immediate
         {
            debugout << "substract constant " << constants[address] << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprConst(constants[address]);
            exp = new exprSub(term1, term2);
            break;
         }

         case nlMul: // multiply
         {
            debugout << "multiply" << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = stack.back(); stack.pop_back();
            exp = new exprMul(term1, term2);
            break;
         }

         case nlMulV: // multiply variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "multiply variable " << address << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprClone(prob->Variables()[address]);
            exp = new exprMul(term1, term2);
            break;
         }

         case nlMulI: // multiply immediate
         {
            debugout << "multiply constant " << constants[address] << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprConst(constants[address]);
            exp = new exprMul(term1, term2);
            break;
         }

         case nlMulIAdd: // multiply immediate add
         {
            debugout << "multiply constant " << constants[address] << " and add " << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            term1 = new exprMul(term1, new exprConst(constants[address]));
            expression* term2 = stack.back(); stack.pop_back();

            exp = new exprSum(term1, term2);
            break;
         }

         case nlDiv: // divide
         {
            debugout << "divide" << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = stack.back(); stack.pop_back();
            if( term2->Type() == CONST )
               exp = new exprMul(term2, new exprInv(term1));
            else
               exp = new exprDiv(term2, term1);
            break;
         }

         case nlDivV: // divide variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "divide variable " << address << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprClone(prob->Variables()[address]);
            if( term1->Type() == CONST )
               exp = new exprMul(term1, new exprInv(term2));
            else
               exp = new exprDiv(term1, term2);
            break;
         }

         case nlDivI: // divide immediate
         {
            debugout << "divide constant " << constants[address] << std::endl;

            expression* term1 = stack.back(); stack.pop_back();
            expression* term2 = new exprConst(constants[address]);
            exp = new exprDiv(term1, term2);
            break;
         }

         case nlUMin: // unary minus
         {
            debugout << "negate" << std::endl;

            expression* term = stack.back(); stack.pop_back();
            exp = new exprOpp(term);
            break;
         }

         case nlUMinV: // unary minus variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "push negated variable " << address << std::endl;

            exp = new exprOpp(new exprClone(prob->Variables()[address]));
            break;
         }

         case nlFuncArgN: // number of function arguments
         {
            nargs = address;
            debugout << nargs << " arguments" << std::endl;
            break;
         }

         case nlCallArg1 :
         case nlCallArg2 :
         case nlCallArgN :
         {
            debugout << "call function ";

            switch( GamsFuncCode(address+1) )  // undo shift by 1
            {
#if 0 /* min and max are not fully implemented in Couenne yet */
               case fnmin:
               {
                  debugout << "min" << std::endl;

                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();
                  exp = new exprMin(term1, term2);
                  break;
               }

               case fnmax:
               {
                  debugout << "max" << std::endl;

                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();
                  exp = new exprMax(term1, term2);
                  break;
               }
#endif
               case fnsqr:
               {
                  debugout << "square" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprPow(term, new exprConst(2.0));
                  break;
               }

               case fnexp:
               case fnslexp:
               case fnsqexp:
               {
                  debugout << "exp" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprExp(term);
                  break;
               }

               case fnlog:
               {
                  debugout << "ln" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprLog(term);
                  break;
               }

               case fnlog10:
               case fnsllog10:
               case fnsqlog10:
               {
                  debugout << "log10 = ln * 1/ln(10)" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprMul(new exprLog(term), new exprConst(1.0/log(10.0)));
                  break;
               }

               case fnlog2 :
               {
                  debugout << "log2 = ln * 1/ln(2)" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprMul(new exprLog(term), new exprConst(1.0/log(2.0)));
                  break;
               }

               case fnsqrt:
               {
                  debugout << "sqrt" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprPow(term, new exprConst(0.5));
                  break;
               }

               case fnabs:
               {
                  debugout << "abs" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprAbs(term);
                  break;
               }

               case fncos:
               {
                  debugout << "cos" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprCos(term);
                  break;
               }

               case fnsin:
               {
                  debugout << "sin" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprSin(term);
                  break;
               }

               case fnpower:  // x ^ y
               case fnrpower: // x ^ y
               {
                  debugout << "power(x,y) = exp(log(y)*x)" << std::endl;

                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();
                  if( term1->Type() == CONST )
                     exp = new exprPow(term2, term1);
                  else
                     exp = new exprExp(new exprMul(new exprLog(term2), term1));
                  break;
               }

               case fncvpower: // constant ^ x
               {
                  debugout << "power(a,x) = exp(log(a)*x)" << std::endl;

                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();

                  assert(term2->Type() == CONST);
                  exp = new exprExp(new exprMul(new exprConst(log(((exprConst*)term2)->Value())), term1));
                  delete term2;
                  break;
               }

               case fnvcpower: // x ^ constant
               {
                  debugout << "power(x,a)" << std::endl;

                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();
                  assert(term1->Type() == CONST);
                  exp = new exprPow(term2, term1);
                  break;
               }

               case fnsignpower: // sign(x)*abs(x)^c = x * abs(x)^(c-1)
               {
                  debugout << "signed power" << std::endl;
                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();

                  assert(term1->Type() == CONST);
                  exp = new exprMul(term2, new exprPow(new exprAbs(new exprClone(term2)), new exprConst(((exprConst*)term1)->Value() - 1.0)));
                  delete term1;
                  break;
               }

               case fnpi:
               {
                  debugout << "pi" << std::endl;
                  exp = new exprConst(M_PI);
                  break;
               }

               case fndiv:
               case fndiv0:
               {
                  debugout << "divide" << std::endl;
                  expression* term1 = stack.back(); stack.pop_back();
                  expression* term2 = stack.back(); stack.pop_back();
                  if( term2->Type() == CONST )
                     exp = new exprMul(term2, new exprInv(term1));
                  else
                     exp = new exprDiv(term2, term1);
                  break;
               }

               case fnslrec: // 1/x
               case fnsqrec: // 1/x
               {
                  debugout << "reciproce" << std::endl;

                  expression* term = stack.back(); stack.pop_back();
                  exp = new exprInv(term);
                  break;
               }

               case fnpoly: /* univariate polynomial */
               {
                  debugout << "univariate polynomial of degree " << nargs-1 << std::endl;
                  assert(nargs >= 0);
                  switch( nargs )
                  {
                     case 0:
                        delete stack.back(); stack.pop_back(); // delete variable of polynomial
                        exp = new exprConst(0.0);
                        break;

                     case 1: // "constant" polynomial
                        exp = stack.back(); stack.pop_back();
                        delete stack.back(); stack.pop_back(); // delete variable of polynomial
                        break;

                     default: // polynomial is at least linear
                     {
                        std::vector<expression*> coeff(nargs);
                        while( nargs )
                        {
                           assert(!stack.empty());
                           coeff[nargs-1] = stack.back();
                           stack.pop_back();
                           --nargs;
                        }
                        assert(!stack.empty());
                        expression* var = stack.back(); stack.pop_back();
                        expression** monomials = new expression*[coeff.size()];
                        monomials[0] = coeff[0];
                        monomials[1] = new exprMul(coeff[1], var);
                        for( size_t i = 2; i < coeff.size(); ++i )
                           monomials[i] = new exprMul(coeff[i], new exprPow(new exprClone(var), new exprConst(static_cast<double>(i))));
                        exp = new exprSum(monomials, static_cast<int>(coeff.size()));
                     }
                  }
                  nargs = -1;
                  break;
               }

               // TODO some more we could handle
               case fnceil: case fnfloor: case fnround:
               case fnmod: case fntrunc: case fnsign:
               case fnarctan: case fnerrf: case fndunfm:
               case fndnorm: case fnerror: case fnfrac: case fnerrorl:
               case fnfact /* factorial */:
               case fnunfmi /* uniform random number */:
               case fnncpf /* fischer: sqrt(x1^2+x2^2+2*x3) */:
               case fnncpcm /* chen-mangasarian: x1-x3*ln(1+exp((x1-x2)/x3))*/:
               case fnentropy /* x*ln(x) */: case fnsigmoid /* 1/(1+exp(-x)) */:
               case fnboolnot: case fnbooland:
               case fnboolor: case fnboolxor: case fnboolimp:
               case fnbooleqv: case fnrelopeq: case fnrelopgt:
               case fnrelopge: case fnreloplt: case fnrelople:
               case fnrelopne: case fnifthen:
               case fnedist /* euclidian distance */:
               case fncentropy /* x*ln((x+d)/(y+d))*/:
               case fngamma: case fnloggamma: case fnbeta:
               case fnlogbeta: case fngammareg: case fnbetareg:
               case fnsinh: case fncosh: case fntanh:
               case fnncpvusin /* veelken-ulbrich */:
               case fnncpvupow /* veelken-ulbrich */:
               case fnbinomial:
               case fntan: case fnarccos:
               case fnarcsin: case fnarctan2 /* arctan(x2/x1) */:
               case fnmin: case fnmax:
               default:
               {
                  char buffer[256];
                  sprintf(buffer, "Gams function code %d not supported.", address+1);
                  gevLogStat(gev, buffer);
                  while( !stack.empty() )
                  {
                     delete stack.back();
                     stack.pop_back();
                  }
                  return NULL;
               }
            }
            break;
         }

         default:
         {
            char buffer[256];
            sprintf(buffer, "Gams opcode %d not supported.", opcode);
            gevLogStat(gev, buffer);
            while( !stack.empty() )
            {
               delete stack.back();
               stack.pop_back();
            }
            return NULL;
         }
      }

      if( exp != NULL )
         stack.push_back(exp);
   }

   assert(stack.size() == 1);
   return stack.back();
#undef debugout
}

void GamsCouenne::passSOSSemiCon(
   Couenne::CouenneInterface* ci,            /**< Couenne interface where to add objects for SOS and semicon constraints */
   CbcModel*             cbcmodel            /**< CBC model used for B&B */
   )
{
   assert(dynamic_cast<OsiSolverInterface*>(ci) != NULL);

   // assemble SOS of type 1 or 2
   int numSos1, numSos2, nzSos;
   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
   if( nzSos > 0 )
   {
      int numSos = numSos1 + numSos2;
      OsiObject** sosobjects = new OsiObject*[numSos];

      int* sostype  = new int[numSos];
      int* sosbeg   = new int[numSos+1];
      int* sosind   = new int[nzSos];
      double* soswt = new double[nzSos];

      gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);

      int*    which   = new int[CoinMin(gmoN(gmo), nzSos)];
      double* weights = new double[CoinMin(gmoN(gmo), nzSos)];

      for( int i = 0; i < numSos; ++i )
      {
         int k = 0;
         for( int j = sosbeg[i]; j < sosbeg[i+1]; ++j, ++k )
         {
            which[k]   = sosind[j];
            weights[k] = soswt[j];
            assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? gmovar_S1 : gmovar_S2));
         }
         sosobjects[i] = new CbcSOS(cbcmodel, k, which, weights, i, sostype[i]);
         sosobjects[i]->setPriority(gmoN(gmo)-k); // branch on long sets first
      }

      delete[] which;
      delete[] weights;
      delete[] sostype;
      delete[] sosbeg;
      delete[] sosind;
      delete[] soswt;

      (dynamic_cast<OsiSolverInterface*>(ci))->addObjects(numSos, sosobjects);
      for( int i = 0; i < numSos; ++i )
         delete sosobjects[i];
      delete[] sosobjects;
   }

   // assemble semicontinuous and semiinteger variables
   int numSemi = gmoGetVarTypeCnt(gmo, gmovar_SC) + gmoGetVarTypeCnt(gmo, gmovar_SI);
   if( numSemi > 0)
   {
      OsiObject** semiobjects = new OsiObject*[numSemi];
      int object_nr = 0;
      double points[4];
      points[0] = 0.;
      points[1] = 0.;
      for( int i = 0; i < gmoN(gmo); ++i )
      {
         enum gmoVarType vartype = (enum gmoVarType)gmoGetVarTypeOne(gmo, i);
         if( vartype != gmovar_SC && vartype != gmovar_SI )
            continue;

         double varlb = gmoGetVarLowerOne(gmo, i);
         double varub = gmoGetVarUpperOne(gmo, i);

         if( vartype == gmovar_SI && varub - varlb <= 1000 )
         {
            // model lotsize for discrete variable as a set of integer values
            int len = (int)(varub - varlb + 2);
            double* points2 = new double[len];
            points2[0] = 0.;
            int j = 1;
            for( int p = (int)varlb; p <= varub; ++p, ++j )
               points2[j] = (double)p;
            semiobjects[object_nr] = new CbcLotsize(cbcmodel, i, len, points2, false);
            delete[] points2;
         }
         else
         {
            // lotsize for continuous variable or integer with large upper bounds
            if( vartype == gmovar_SI )
               gevLogStat(gev, "Warning: Support of semiinteger variables with a range larger than 1000 is experimental.\n");
            points[2] = varlb;
            points[3] = varub;
            if( varlb == varub ) // var. can be only 0 or varlb
               semiobjects[object_nr] = new CbcLotsize(cbcmodel, i, 2, points+1, false);
            else // var. can be 0 or in the range between low and upper
               semiobjects[object_nr] = new CbcLotsize(cbcmodel, i, 2, points,   true );
         }

         ++object_nr;
      }
      assert(object_nr == numSemi);

      /* pass in lotsize objects */
      (dynamic_cast<OsiSolverInterface*>(ci))->addObjects(numSemi, semiobjects);
      for( int i = 0; i < numSemi; ++i )
         delete semiobjects[i];
      delete[] semiobjects;
   }
}

#define GAMSSOLVERC_ID         cou
#define GAMSSOLVERC_CLASS      GamsCouenne
#include "GamsSolverC_tpl.cpp"
