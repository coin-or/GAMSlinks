// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cfloat>

/* GAMS API */
#include "gevmcc.h"
#include "gmomcc.h"
#include "palmcc.h"
#include "GamsLinksConfig.h"
#ifdef GAMS_BUILD
#include "GamsLicensing.h"
#include "libloader.h"
#endif
#include "GamsSolveTrace.h"

/* HiGHS API */
#include "Highs.h"

#ifdef _WIN32
#define LIBCUPDLP_CPU "cupdlp_cpu.dll"
#define LIBCUPDLP_CUDA "cupdlp_cuda.dll"
#elif defined(__APPLE__)
#define LIBCUPDLP_CPU "libcupdlp_cpu.dylib"
#else
#define LIBCUPDLP_CPU "libcupdlp_cpu.so"
#define LIBCUPDLP_CUDA "libcupdlp_cuda.so"
#endif

typedef struct
{
   gmoHandle_t   gmo;
   gevHandle_t   gev;

   Highs*        highs;
   bool          ranging;
   int           mipstart;
   bool          illconditioning;
   bool          illconditioning_constraint;
   int           illconditioning_method;
   double        illconditioning_bound;
   std::string*  solvetracefile;          /* Name of solvetrace file to write */
   int           solvetracenodefreq;      /* Solvetrace node frequency */
   double        solvetracetimefreq;      /* Solvetrace time frequency */
   int           iis;
#ifdef GAMS_BUILD
   bool          pdlp_gpu;
#endif

   GAMS_solvetrace* solvetrace;
#ifdef GAMS_BUILD
   void*         cupdlplib_handle;
#endif
} gamshighs_t;

static
void gamshighs_callback(
   const int                   callback_type,
   const std::string&          message,
   const HighsCallbackOutput*  data_out,
   HighsCallbackInput*         data_in,
   void*                       user_callback_data
)
{
   gamshighs_t* gh = (gamshighs_t*) user_callback_data;
   assert(gh != NULL);

   if( callback_type == HighsCallbackType::kCallbackLogging )
   {
      if( data_out->log_type == HighsLogType::kInfo )
         gevLogPChar(gh->gev, message.c_str());
      else
         gevLogStatPChar(gh->gev, message.c_str());

      return;
   }

   if( gevTerminateGet(gh->gev) )
   {
      data_in->user_interrupt = 1;
   }

   if( gh->solvetrace != NULL && callback_type == HighsCallbackType::kCallbackMipInterrupt )
   {
      GAMSsolvetraceAddLine(gh->solvetrace, (int)data_out->mip_node_count, data_out->mip_dual_bound, data_out->mip_primal_bound);
   }
}

static
enum gmoVarEquBasisStatus translateBasisStatus(
   HighsBasisStatus status
)
{
   switch( status )
   {
      case HighsBasisStatus::kBasic:
         return gmoBstat_Basic;
      case HighsBasisStatus::kLower:
         return gmoBstat_Lower;
      case HighsBasisStatus::kNonbasic:
      case HighsBasisStatus::kZero:
         return gmoBstat_Super;
      case HighsBasisStatus::kUpper:
         return gmoBstat_Upper;
   }
   // this should never happen
   return gmoBstat_Super;
}

static
HighsBasisStatus translateBasisStatus(
   enum gmoVarEquBasisStatus status
)
{
   switch( status )
   {
      case gmoBstat_Basic:
         return HighsBasisStatus::kBasic;
      case gmoBstat_Lower:
         return HighsBasisStatus::kLower;
      case gmoBstat_Super:
         return HighsBasisStatus::kNonbasic;
      case gmoBstat_Upper:
         return HighsBasisStatus::kUpper;
   }
   // this should never happen
   return HighsBasisStatus::kNonbasic;
}

static
int setupProblem(
   gamshighs_t* gh
)
{
   HighsInt numCol;
   HighsInt numRow;
   HighsInt numNz;
   std::vector<double> col_costs;
   std::vector<double> col_lower;
   std::vector<double> col_upper;
   std::vector<double> row_lower;
   std::vector<double> row_upper;
   std::vector<int> astart;
   std::vector<int> aindex;
   std::vector<double> avalue;
   std::vector<HighsInt> integrality;
   HighsInt i;

   assert(gh != NULL);
   assert(gh->highs == NULL);

   if( gmoNZ64(gh->gmo) > INT_MAX )
   {
      gevLogStat(gh->gev, "ERROR: Problems with more than 2^31 nonzeros not supported by HiGHS link.");
      gmoSolveStatSet(gh->gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gh->gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }

   gh->highs = new Highs();
   gh->highs->setCallback(gamshighs_callback, gh);
   gh->highs->startCallback(HighsCallbackType::kCallbackLogging);
   gh->highs->startCallback(HighsCallbackType::kCallbackSimplexInterrupt);
   gh->highs->startCallback(HighsCallbackType::kCallbackIpmInterrupt);
   gh->highs->startCallback(HighsCallbackType::kCallbackMipInterrupt);

   HighsOptions& highsopt(const_cast<HighsOptions&>(gh->highs->getOptions()));

   highsopt.records.push_back(
      new OptionRecordBool("sensitivity",
         "Whether to run sensitivity analysis after solving an LP with a simplex method",
         false, &gh->ranging, false) );

   highsopt.records.push_back(
      new OptionRecordBool("illconditioning",
         "Whether to run ill conditioning analysis after solving an LP with a simplex method",
         false, &gh->illconditioning, false) );

   highsopt.records.push_back(
      new OptionRecordBool("illconditioning_constraint",
         "Whether to run ill conditioning on constraint view (alternative is variable view)",
         false, &gh->illconditioning_constraint, false) );

   highsopt.records.push_back(
      new OptionRecordInt("illconditioning_method",
         "Method to use for ill conditioning analysis, i.e., auxiliary problem to be solved",
         false, &gh->illconditioning_method, 0, 0, 1) );

   highsopt.records.push_back(
      new OptionRecordDouble("illconditioning_bound",
         "Bound on ill conditioning when using ill conditioning analysis method 1",
         false, &gh->illconditioning_bound, 0.0, 1e-4, DBL_MAX) );

   highsopt.records.push_back(
      new OptionRecordInt("mipstart",
         "Whether and how to pass initial level values as starting point to MIP solver",
         false, &gh->mipstart, 0, 2, 4) );

   highsopt.records.push_back(
      new OptionRecordString("solvetrace",
         "Name of file for writing solving progress information during MIP solve",
         false, gh->solvetracefile, "") );

   highsopt.records.push_back(
      new OptionRecordInt("solvetracenodefreq",
         "Frequency in number of nodes for writing to solve trace file",
         false, &gh->solvetracenodefreq, 0, 100, INT_MAX) );

   highsopt.records.push_back(
      new OptionRecordDouble("solvetracetimefreq",
         "Frequency in seconds for writing to solve trace file",
         false, &gh->solvetracetimefreq, 0.0, 5.0, DBL_MAX) );

   highsopt.records.push_back(
      new OptionRecordInt("iis",
         "whether to compute an irreducible infeasible subset of an LP",
         false, &gh->iis, 0, 0, 2) );

#ifdef GAMS_BUILD
   highsopt.records.push_back(
      new OptionRecordBool("pdlp_gpu",
         "Whether to attempt using an NVIDIA GPU when solver=pdlp.",
         false, &gh->pdlp_gpu, false) );
#endif

   numCol = gmoN(gh->gmo);
   numRow = gmoM(gh->gmo);
   numNz = gmoNZ(gh->gmo);

   /* columns */
   col_lower.resize(numCol);
   col_upper.resize(numCol);
   gmoGetVarLower(gh->gmo, col_lower.data());
   gmoGetVarUpper(gh->gmo, col_upper.data());

   /* integrality */
   if( gmoNDisc(gh->gmo) > 0 )
   {
      integrality.resize(numCol);
      for( i = 0; i < numCol; ++i )
      {
         switch( gmoGetVarTypeOne(gh->gmo, i) )
         {
            case gmovar_X:
               integrality[i] = (HighsInt)HighsVarType::kContinuous;
               break;
            case gmovar_B:
            case gmovar_I:
               integrality[i] = (HighsInt)HighsVarType::kInteger;
               break;
            case gmovar_SC:
               integrality[i] = (HighsInt)HighsVarType::kSemiContinuous;
               break;
            case gmovar_SI:
               integrality[i] = (HighsInt)HighsVarType::kSemiInteger;
               break;
            case gmovar_S1:
            case gmovar_S2:
               gevLogStatPChar(gh->gev, "Special ordered sets not supported.\n");
               gmoModelStatSet(gh->gmo, gmoModelStat_NoSolutionReturned);
               gmoSolveStatSet(gh->gmo, gmoSolveStat_Capability);
               return 0;
            default:
               /* should not occur */
               gevLogStatPChar(gh->gev, "Unsupported variable type.\n");
               return 1;

         }
      }
   }

   /* objective */
   col_costs.resize(numCol);
   gmoGetObjVector(gh->gmo, col_costs.data(), NULL);

   /* row left- and right-hand-side */
   row_lower.resize(numRow);
   row_upper.resize(numRow);
   for( i = 0; i < numRow; ++i )
   {
      switch( gmoGetEquTypeOne(gh->gmo, i) )
      {
         case gmoequ_E:
            row_lower[i] = row_upper[i] = gmoGetRhsOne(gh->gmo, i);
            break;

         case gmoequ_G:
            row_lower[i] = gmoGetRhsOne(gh->gmo, i);
            row_upper[i] = kHighsInf;
            break;

         case gmoequ_L:
            row_lower[i] = -kHighsInf;
            row_upper[i] = gmoGetRhsOne(gh->gmo, i);
            break;

         case gmoequ_N:
         case gmoequ_X:
         case gmoequ_C:
         case gmoequ_B:
            /* these should not occur */
            return 1;
      }
   }

   /* coefficients matrix */
   astart.resize(numCol + 1);
   aindex.resize(numNz);
   avalue.resize(numNz);
   gmoGetMatrixCol(gh->gmo, astart.data(), aindex.data(), avalue.data(), NULL);

   gh->highs->passModel(numCol, numRow, numNz,
      (HighsInt)MatrixFormat::kColwise,
      (HighsInt)(gmoSense(gh->gmo) == gmoObj_Min ? ObjSense::kMinimize : ObjSense::kMaximize),
      gmoObjConst(gh->gmo),
      col_costs.data(), col_lower.data(), col_upper.data(),
      row_lower.data(), row_upper.data(),
      astart.data(), aindex.data(), avalue.data(),
      integrality.empty() ? NULL : integrality.data());

   // passing an initial solution for an LP turns off presolve, since HiGHS constructs a basis from it (unless we specify a basis below)
#if 0
   // pass initial solution, if LP and rows are present
   // for a MIP, we consider setting a starting point in setupOptions()
   // so for an LP, the users starting point is set only once and not for every solve after a problem modification
   // for a MIP, we pass the users starting point to each solve
   if( gmoNDisc(gh->gmo) == 0 )
   {
      HighsSolution sol;
      sol.col_value.resize(numCol);
      sol.col_dual.resize(numCol);
      sol.row_value.resize(numRow);
      sol.row_dual.resize(numRow);
      gmoGetVarL(gh->gmo, &sol.col_value[0]);
      gmoGetVarM(gh->gmo, &sol.col_dual[0]);
      if( gmoM(gh->gmo) )
      {
         gmoGetEquL(gh->gmo, &sol.row_value[0]);
         gmoGetEquM(gh->gmo, &sol.row_dual[0]);
      }
      gh->highs->setSolution(sol);
   }
#endif

   if( gmoHaveBasis(gh->gmo) )
   {
      HighsBasis basis;
      basis.col_status.resize(numCol);
      basis.row_status.resize(numRow);
      HighsInt nbasic = 0;

      for( HighsInt i = 0; i < numCol; ++i )
      {
         basis.col_status[i] = translateBasisStatus((enum gmoVarEquBasisStatus) gmoGetVarStatOne(gh->gmo, i));
         if( basis.col_status[i] == HighsBasisStatus::kBasic )
            ++nbasic;
      }

      for( HighsInt i = 0; i < numRow; ++i )
      {
         basis.row_status[i] = translateBasisStatus((enum gmoVarEquBasisStatus) gmoGetEquStatOne(gh->gmo, i));
         if( basis.row_status[i] == HighsBasisStatus::kBasic )
            ++nbasic;
      }

      basis.valid = nbasic == numRow;
      /* HiGHS compiled without NDEBUG defined currently raises an assert in
       * basisOK() if given an invalid basis */
      if( basis.valid )
         gh->highs->setBasis(basis);
   }

   return 0;
}

static
int setupOptions(
   gamshighs_t* gh
)
{
   char buf[GMS_SSSIZE+50];

   assert(gh != NULL);
   assert(gh->highs != NULL);

   gh->highs->resetOptions();

   gh->highs->setOptionValue("time_limit", gevGetDblOpt(gh->gev, gevResLim));
   gh->highs->setOptionValue("simplex_iteration_limit", gevGetIntOpt(gh->gev, gevIterLim));
   gh->highs->setOptionValue("ipm_iteration_limit", gevGetIntOpt(gh->gev, gevIterLim));
   gh->highs->setOptionValue("pdlp_iteration_limit", gevGetIntOpt(gh->gev, gevIterLim));
   if( gevGetIntOpt(gh->gev, gevUseCutOff) )
      gh->highs->setOptionValue("objective_bound", gevGetDblOpt(gh->gev, gevCutOff));
   if( gevGetIntOpt(gh->gev, gevNodeLim) > 0 )
      gh->highs->setOptionValue("mip_max_nodes", gevGetIntOpt(gh->gev, gevNodeLim));
   gh->highs->setOptionValue("mip_rel_gap", gevGetDblOpt(gh->gev, gevOptCR));
   gh->highs->setOptionValue("mip_abs_gap", gevGetDblOpt(gh->gev, gevOptCA));
   if( gevGetIntOpt(gh->gev, gevThreadsRaw) != 0 )
      gh->highs->setOptionValue("threads", gevThreads(gh->gev));
   if( gevGetIntOpt(gh->gev, gevLogOption) == 0 )
      gh->highs->setOptionValue("output_flag", false);

   // we want the default of pdlp_e_restart_method be 0 or 1, depending on the value of option pdlp_gpu
   // to find out whether the user set the option, we set it to 2 first (this is a valid but undocumented value)
   gh->highs->setOptionValue("pdlp_e_restart_method", 2);

   if( gmoOptFile(gh->gmo) > 0 )
   {
      char optfilename[GMS_SSSIZE];
      gmoNameOptFile(gh->gmo, optfilename);
      gh->highs->readOptions(optfilename);
   }

   // setting option "solver" on a MIP means that only the LP relax is solved (https://github.com/ERGO-Code/HiGHS/issues/904)
   // that's not what we want
   if( gmoNDisc(gh->gmo) > 0 )
      gh->highs->setOptionValue("solver", "choose");

#ifdef GAMS_BUILD
   // disable GPU variant of PDLP if we have no cupdlp_cuda library
#ifndef LIBCUPDLP_CUDA
   if( gh->pdlp_gpu )
   {
      gevLogPChar(gh->gev, "Reset pdlp_gpu to false. CUDA-enabled cuPDLP-C not available on this platform.\n");
      gh->highs->setOptionValue("pdlp_gpu", false);
   }
#endif
#endif

#ifdef GAMS_BUILD
   // if pdlp_e_restart_method is still at 2, we assume user did not set this, and set to 0 or 1 depending on pdlp_gpu
   int pdlp_e_restart_method;
   gh->highs->getOptionValue("pdlp_e_restart_method", pdlp_e_restart_method);
   if( pdlp_e_restart_method == 2 )
      gh->highs->setOptionValue("pdlp_e_restart_method", gh->pdlp_gpu ? 1 : 0);
#endif

   gevLogPChar(gh->gev, "Options set:\n");
   for( OptionRecord* hopt : gh->highs->getOptions().records )
   {
      switch( hopt->type )
      {
         case HighsOptionType::kBool :
         {
            OptionRecordBool* opt = static_cast<OptionRecordBool*>(hopt);
            assert(opt->value != NULL);
            if( opt->default_value == *opt->value )
               continue;
            sprintf(buf, "  %-20s = %s\n", opt->name.c_str(), *opt->value ? "true" : "false");
            gevLogPChar(gh->gev, buf);
            break;
         }

         case HighsOptionType::kInt :
         {
            OptionRecordInt* opt = static_cast<OptionRecordInt*>(hopt);
            assert(opt->value != NULL);
            if( opt->default_value == *opt->value )
               continue;
            sprintf(buf, "  %-20s = %d\n", opt->name.c_str(), *opt->value);
            gevLogPChar(gh->gev, buf);
            break;
         }

         case HighsOptionType::kDouble :
         {
            OptionRecordDouble* opt = static_cast<OptionRecordDouble*>(hopt);
            assert(opt->value != NULL);
            if( opt->default_value == *opt->value )
               continue;
            sprintf(buf, "  %-20s = %g\n", opt->name.c_str(), *opt->value);
            gevLogPChar(gh->gev, buf);
            break;
         }

         case HighsOptionType::kString :
         {
            OptionRecordString* opt = static_cast<OptionRecordString*>(hopt);
            assert(opt->value != NULL);
            if( opt->default_value == *opt->value )
               continue;
            sprintf(buf, "  %-20s = %s\n", opt->name.c_str(), opt->value->c_str());
            gevLogPChar(gh->gev, buf);
            break;
         }
      }
   }

   // pass initial solution if mipstart set and MIP and we do not do IIS only
   if( gh->mipstart > 0 && gmoNDisc(gh->gmo) > 0 && gh->iis != 2 )
   {
      HighsSolution sol;
      switch( gh->mipstart )
      {
         /* pass all values and only check feasibility - default*/
         case 2:
         {
            /* HiGHS does not have an option where one can pass it a solution and
             * it would just discard if when it is not feasible.
             * The closest would be setting mip_max_start_nodes=0, but it would still
             * go through some presolve and LP solve.
             * Thus, we first check whether the initial values are feasible by using HiGHS' assessLpPrimalSolution().
             * However, HiGHS accepts a solution there only if row values are correct, so we calculate them first (so GAMS user doesn't have to set valid equation levels).
             * If the solution is found feasible, we pass it to HiGHS, which will call assessLpPrimalSolution() again...
             */
            sol.col_value.resize(gmoN(gh->gmo));
            gmoGetVarL(gh->gmo, sol.col_value.data());
            calculateRowValuesQuad(gh->highs->getLp(), sol.col_value, sol.row_value);
            sol.value_valid = true;

            bool valid;
            bool integral;
            bool feasible;
            assessLpPrimalSolution("", gh->highs->getOptions(),
               gh->highs->getLp(), sol, valid, integral, feasible);

            if( feasible )
            {
               gh->highs->setSolution(sol);
               snprintf(buf, sizeof(buf), "Initial variable level values are feasible. Passing to HiGHS.");
            }
            else
            {
               snprintf(buf, sizeof(buf), "Initial variable level values are not feasible.");
            }
            gevLogPChar(gh->gev, buf);

            break;
         }

         /* pass all values and let HiGHS try to repair
          * if all integer vars have integer values, it will solve a LP
          * if some integer vars have fractional values, it will solve a MIP
          */
         case 3:
         {
            sol.col_value.resize(gmoN(gh->gmo));
            gmoGetVarL(gh->gmo, sol.col_value.data());

            gh->highs->setSolution(sol);

            snprintf(buf, sizeof(buf), "Passed solution with values for all variables to HiGHS.");
            gevLogPChar(gh->gev, buf);

            break;
         }

         /* pass values for all discrete variables (1) or only those with fractionality below tryint (4)
          * HiGHS will solve a LP or MIP to find remaining variable values
          */
         case 1:
         case 4:
         {
            sol.col_value.assign(gmoN(gh->gmo), kHighsUndefined);

            double tryint = gevGetDblOpt(gh->gev, gevTryInt);
            int ndefined = 0;
            for( int i = 0; i < gmoN(gh->gmo); ++i )
            {
               if( gmoGetVarTypeOne(gh->gmo, i) == gmovar_B || gmoGetVarTypeOne(gh->gmo, i) == gmovar_I || gmoGetVarTypeOne(gh->gmo, i) == gmovar_SI )
               {
                  double val = gmoGetVarLOne(gh->gmo, i);
                  if( gh->mipstart == 1 )
                  {
                     sol.col_value[i] = val;
                     ++ndefined;
                  }
                  else if( fabs(round(val)-val) <= tryint )
                  {
                     // HiGHS would treat fractional value for integer as unspecified, so better round to integer
                     sol.col_value[i] = round(val);
                     ++ndefined;
                  }
               }
            }

            gh->highs->setSolution(sol);

            snprintf(buf, sizeof(buf), "Passed solution with values for %d variables (%.1f%%) to HiGHS.", ndefined, 100.0*(double)ndefined/gmoN(gh->gmo));
            gevLogPChar(gh->gev, buf);

            break;
         }

         default:
         {
            snprintf(buf, sizeof(buf), "Setting mipstart = %d not supported. Ignored.\n", gh->mipstart);
            gevLogStatPChar(gh->gev, buf);
            break;
         }
      }
   }

   std::string solver_choice;
   gh->highs->getOptionValue("solver", solver_choice);
   if( solver_choice == "pdlp" )
   {
#ifdef GAMS_BUILD
#ifdef LIBCUPDLP_CUDA
      if( gh->pdlp_gpu )
         gh->cupdlplib_handle = loadLibrary(LIBCUPDLP_CUDA, buf, sizeof(buf));
      else
#endif
         gh->cupdlplib_handle = loadLibrary(LIBCUPDLP_CPU, buf, sizeof(buf));
      if( gh->cupdlplib_handle == NULL )
      {
         gevLogStat(gh->gev, "Error: PDLP solver library not available:");
         gevLogStat(gh->gev, buf);
         return 1;
      }
      void* solveLpCupdlp_C_sym;
      solveLpCupdlp_C_sym = loadSymbol(gh->cupdlplib_handle, "solveLpCupdlp_C", buf, sizeof(buf));
      if( solveLpCupdlp_C_sym == NULL )
      {
         gevLogStat(gh->gev, "Error: PDLP solver library not available:");
         gevLogStat(gh->gev, buf);
         return 1;
      }
      gh->highs->getOptions().solveLpCupdlp_C = (HighsStatus (*)(HighsLpSolverObject*))solveLpCupdlp_C_sym;
#endif
   }

   return 0;
}

static
int processSolve(
   gamshighs_t* gh
)
{
   assert(gh != NULL);
   assert(gh->highs != NULL);

   gmoHandle_t gmo = gh->gmo;
   Highs* highs = gh->highs;

   if( gh->solvetrace != NULL )
      GAMSsolvetraceAddEndLine(gh->solvetrace, (int)highs->getInfo().mip_node_count, highs->getInfo().mip_dual_bound, highs->getInfo().objective_function_value);

   gmoSetHeadnTail(gmo, gmoHresused, gevTimeDiffStart(gh->gev));
   gmoSetHeadnTail(gmo, gmoHiterused, highs->getInfo().simplex_iteration_count);
   gmoSetHeadnTail(gmo, gmoTmipbest, highs->getInfo().mip_dual_bound);
   gmoSetHeadnTail(gmo, gmoTmipnod, highs->getInfo().mip_node_count);

   const HighsSolution &sol = highs->getSolution();

   // figure out model and solution status
   switch( highs->getModelStatus() )
   {
      case HighsModelStatus::kNotset:
      case HighsModelStatus::kLoadError:
      case HighsModelStatus::kModelError:
      case HighsModelStatus::kPresolveError:
      case HighsModelStatus::kSolveError:
      case HighsModelStatus::kPostsolveError:
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      case HighsModelStatus::kModelEmpty:
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         break;

      case HighsModelStatus::kOptimal:
         gmoSetHeadnTail(gmo, gmoHobjval, highs->getInfo().objective_function_value);  // so we can get a gap next
         if( gmoNDisc(gmo) == 0 || (gmoGetRelativeGap(gmo) <= 2e-9 || gmoGetAbsoluteGap(gmo) <= 1.1e-9) )
            gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
         else
            gmoModelStatSet(gmo, gmoModelStat_Integer);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         break;

      case HighsModelStatus::kInfeasible:
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         break;

      case HighsModelStatus::kUnboundedOrInfeasible:
      case HighsModelStatus::kUnbounded:
         gmoModelStatSet(gmo, sol.value_valid ? gmoModelStat_Unbounded : gmoModelStat_UnboundedNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         break;

      case HighsModelStatus::kObjectiveBound:
      case HighsModelStatus::kObjectiveTarget:
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         break;

      case HighsModelStatus::kTimeLimit:
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         break;

      case HighsModelStatus::kIterationLimit:
         // may also mean node limit
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         break;

      case HighsModelStatus::kSolutionLimit:
         // may also mean node limit
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         break;

      case HighsModelStatus::kMemoryLimit:
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         break;

      case HighsModelStatus::kInterrupt:
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_User);
         break;

      case HighsModelStatus::kUnknown:
         gmoModelStatSet(gmo, sol.value_valid && gmoNDisc(gh->gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         break;
   }

   if( sol.value_valid && !sol.dual_valid )
   {
      assert((HighsInt)sol.col_value.size() == gmoN(gmo));
      gmoSetSolutionPrimal(gmo, sol.col_value.data());
      gmoCompleteSolution(gmo);
   }
   else if( sol.value_valid && sol.dual_valid )
   {
      assert((HighsInt)sol.col_value.size() == gmoN(gmo));
      assert((HighsInt)sol.col_dual.size() == gmoN(gmo));
      assert((HighsInt)sol.row_value.size() == gmoM(gmo));
      assert((HighsInt)sol.row_dual.size() == gmoM(gmo));

      const HighsBasis &basis = highs->getBasis();
      assert(!basis.valid || (HighsInt )basis.col_status.size() == gmoN(gmo));
      assert(!basis.valid || (HighsInt )basis.row_status.size() == gmoM(gmo));

      for( HighsInt i = 0; i < gmoN(gmo); ++i )
      {
         gmoVarEquBasisStatus basisstat;
         if( basis.valid )
            basisstat = translateBasisStatus(basis.col_status[i]);
         else
            basisstat = gmoBstat_Super;

         gmoVarEquStatus stat = gmoCstat_OK;

         gmoSetSolutionVarRec(gmo, i, sol.col_value[i], sol.col_dual[i], basisstat, stat);
      }

      for( HighsInt i = 0; i < gmoM(gmo); ++i )
      {
         gmoVarEquBasisStatus basisstat;
         if( basis.valid )
            basisstat = translateBasisStatus(basis.row_status[i]);
         else
            basisstat = gmoBstat_Super;

         gmoVarEquStatus stat = gmoCstat_OK;

         gmoSetSolutionEquRec(gmo, i, sol.row_value[i], sol.row_dual[i], basisstat, stat);
      }

      // if there were =N= rows (lp08), then gmoCompleteObjective wouldn't get their activity right
      // gmoCompleteObjective(gmo, highs->getInfo().objective_function_value);
      gmoCompleteSolution(gmo);
   }

   return 0;
}

#if 0  // in this one, output closely follows writeRangingFile() in HighsRanging.cpp
static
void doRanging(
   gamshighs_t* gh
)
{
   char buffer[2*GMS_SSSIZE];
   char name[GMS_SSSIZE] = "n/a";

   if( !gh->ranging )
      return;

   if( !gh->highs->getBasis().valid )
   {
      gevLogStat(gh->gev, "No basis available. Cannot do sensitivity analysis.\n");
      return;
   }

   HighsRanging ranging;
   if( gh->highs->getRanging(ranging) != HighsStatus::kOk )
      return;

   if( !ranging.valid )
      return;

   double* objcoefs = new double[gmoN(gh->gmo)];
   gmoGetObjVector(gh->gmo, objcoefs, NULL);

   gevStatCon(gh->gev);
   gevStatPChar(gh->gev, "\nSensitivity analysis results:\n");

   gevStatPChar(gh->gev, "\n                                 Objective coefficients ranging\n");
   gevStatPChar(gh->gev, "Column Status  DownObj    Down                  Value                 Up         UpObj      Name\n");
   for( HighsInt iCol = 0; iCol < gmoN(gh->gmo); ++iCol )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetVarNameOne(gh->gmo, iCol, name);
      std::string statstr = statusToString(translateBasisStatus((enum gmoVarEquBasisStatus) gmoGetVarStatOne(gh->gmo, iCol)), gmoGetVarLowerOne(gh->gmo, iCol), gmoGetVarUpperOne(gh->gmo, iCol));
      sprintf(buffer, "%6d   %4s  %-10.4g %-10.4g            %-10.4g            %-10.4g %-10.4g %-s\n", (int)iCol,
         statstr.c_str(),
         ranging.col_cost_dn.objective_[iCol],
         ranging.col_cost_dn.value_[iCol],
         objcoefs[iCol],
         ranging.col_cost_up.value_[iCol],
         ranging.col_cost_up.objective_[iCol], name);
      gevStatPChar(gh->gev, buffer);
   }
   delete[] objcoefs;

   gevStatPChar(gh->gev, "\n                                      Variable bounds ranging\n");
   gevStatPChar(gh->gev, "Column Status  DownObj    Down       Lower      Value      Upper      Up         UpObj      Name\n");
   for( HighsInt iCol = 0; iCol < gmoN(gh->gmo); ++iCol )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetVarNameOne(gh->gmo, iCol, name);
      std::string statstr = statusToString(translateBasisStatus((enum gmoVarEquBasisStatus) gmoGetVarStatOne(gh->gmo, iCol)), gmoGetVarLowerOne(gh->gmo, iCol), gmoGetVarUpperOne(gh->gmo, iCol));
      sprintf(buffer, "%6d   %4s  %-10.4g %-10.4g %-10.4g %-10.4g %-10.4g %-10.4g %-10.4g %-s\n", (int)iCol,
         statstr.c_str(),
         ranging.col_bound_dn.objective_[iCol],
         ranging.col_bound_dn.value_[iCol],
         gmoGetVarLowerOne(gh->gmo, iCol), gmoGetVarLOne(gh->gmo, iCol), gmoGetVarUpperOne(gh->gmo, iCol),
         ranging.col_bound_up.value_[iCol],
         ranging.col_bound_up.objective_[iCol], name);
      gevStatPChar(gh->gev, buffer);
   }

   gevStatPChar(gh->gev, "\n                                      Equation sides ranging\n");
   gevStatPChar(gh->gev, "   Row Status  DownObj    Down       Lower      Value      Upper      Up         UpObj      Name\n");
   for( HighsInt iRow = 0; iRow < gmoM(gh->gmo); ++iRow )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetEquNameOne(gh->gmo, iRow, name);

      double lhs, rhs;
      switch( gmoGetEquTypeOne(gh->gmo, iRow) )
      {
         case gmoequ_E:
            lhs = rhs = gmoGetRhsOne(gh->gmo, iRow);
            break;

         case gmoequ_G:
            lhs = gmoGetRhsOne(gh->gmo, iRow);
            rhs = kHighsInf;
            break;

         case gmoequ_L:
            lhs = -kHighsInf;
            rhs = gmoGetRhsOne(gh->gmo, iRow);
            break;

         default:
            return;
      }
      std::string statstr = statusToString(translateBasisStatus((enum gmoVarEquBasisStatus) gmoGetEquStatOne(gh->gmo, iRow)), 0.0, gmoGetEquTypeOne(gh->gmo, iRow) == gmoequ_E ? 0.0 : 1.0);

      sprintf(buffer, "%6d   %4s  %-10.4g %-10.4g %-10.4g %-10.4g %-10.4g %-10.4g %-10.4g %-s\n", (int)iRow,
         statstr.c_str(),
         ranging.row_bound_dn.objective_[iRow],
         ranging.row_bound_dn.value_[iRow],
         lhs, gmoGetEquLOne(gh->gmo, iRow), rhs,
         ranging.row_bound_up.value_[iRow],
         ranging.row_bound_up.objective_[iRow], name);
      gevStatPChar(gh->gev, buffer);
   }

   gevStatCoff(gh->gev);
   gevLogPChar(gh->gev, "Sensitivity analysis printed to listing file.\n");
}
#endif

// printing closely follows GAMS/CPLEX output to listing file
static
void doRanging(
   gamshighs_t* gh
)
{
   char buffer[2*GMS_SSSIZE];
   char name[GMS_SSSIZE];

   if( !gh->ranging )
      return;

   if( !gh->highs->getBasis().valid )
   {
      gevLogStat(gh->gev, "No basis available. Cannot do sensitivity analysis.\n");
      return;
   }

   HighsRanging ranging;
   if( gh->highs->getRanging(ranging) != HighsStatus::kOk )
   {
      gevLogStat(gh->gev, "Sensitivity analysis failed.\n");
      return;
   }

   if( !ranging.valid )
   {
      gevLogStat(gh->gev, "Sensitivity analysis did not return valid results.\n");
      return;
   }

   gevStatCon(gh->gev);
   gevStatPChar(gh->gev, "\nSensitivity analysis results:\n");

   gevStatPChar(gh->gev, "\nRight-hand-side ranging:\n");
   gevStatPChar(gh->gev, "EQUATION NAME                            LOWER           CURRENT             UPPER\n");
   gevStatPChar(gh->gev, "-------------                            -----           -------             -----\n");
   for( HighsInt iRow = 0; iRow < gmoM(gh->gmo); ++iRow )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetEquNameOne(gh->gmo, iRow, name);
      else
         sprintf(name, "n/a (index: %d)", iRow);

      sprintf(buffer, "%-28s %17g %17g %17g\n", name,
         ranging.row_bound_dn.value_[iRow],
         gmoGetRhsOne(gh->gmo, iRow),
         ranging.row_bound_up.value_[iRow]);
      gevStatPChar(gh->gev, buffer);
   }

   gevStatPChar(gh->gev, "\nLower bound ranging:\n");
   gevStatPChar(gh->gev, "VARIABLE NAME                            LOWER           CURRENT             UPPER\n");
   gevStatPChar(gh->gev, "-------------                            -----           -------             -----\n");
   for( HighsInt iCol = 0; iCol < gmoN(gh->gmo); ++iCol )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetVarNameOne(gh->gmo, iCol, name);
      else
         sprintf(name, "n/a (index: %d)", iCol);

      if( (enum gmoVarEquBasisStatus) gmoGetVarStatOne(gh->gmo, iCol) == gmoBstat_Lower )
      {
         // if variable is at lower bound, then we have ranging info for the lower bound
         sprintf(buffer, "%-28s %17g %17g %17g\n", name,
            ranging.col_bound_dn.value_[iCol],
            gmoGetVarLowerOne(gh->gmo, iCol),
            ranging.col_bound_up.value_[iCol]);
      }
      else
      {
         // if variable is not at lower bound, then the lower bound can be decreased until -inf
         // and it can be increased until its current value (maybe more, but I don't think that we get this value from HiGHS)
         sprintf(buffer, "%-28s %17g %17g %17g\n", name,
            -std::numeric_limits<double>::infinity(),
            gmoGetVarLowerOne(gh->gmo, iCol),
            gmoGetVarLOne(gh->gmo, iCol));
      }
      gevStatPChar(gh->gev, buffer);
   }

   gevStatPChar(gh->gev, "\nUpper bound ranging:\n");
   gevStatPChar(gh->gev, "VARIABLE NAME                            LOWER           CURRENT             UPPER\n");
   gevStatPChar(gh->gev, "-------------                            -----           -------             -----\n");
   for( HighsInt iCol = 0; iCol < gmoN(gh->gmo); ++iCol )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetVarNameOne(gh->gmo, iCol, name);
      else
         sprintf(name, "n/a (index: %d)", iCol);

      if( (enum gmoVarEquBasisStatus) gmoGetVarStatOne(gh->gmo, iCol) == gmoBstat_Upper )
      {
         // if variable is at upper bound, then we have ranging info for the upper bound
         sprintf(buffer, "%-28s %17g %17g %17g\n", name,
            ranging.col_bound_dn.value_[iCol],
            gmoGetVarUpperOne(gh->gmo, iCol),
            ranging.col_bound_up.value_[iCol]);
      }
      else
      {
         // if variable is not at upper bound, then the upper bound can be increased until +inf
         // and it can be decreased until its current value (maybe more, but I don't think that we get this value from HiGHS)
         sprintf(buffer, "%-28s %17g %17g %17g\n", name,
            gmoGetVarLOne(gh->gmo, iCol),
            gmoGetVarUpperOne(gh->gmo, iCol),
            std::numeric_limits<double>::infinity());
      }
      gevStatPChar(gh->gev, buffer);
   }

   double* objcoefs = new double[gmoN(gh->gmo)];
   gmoGetObjVector(gh->gmo, objcoefs, NULL);

   gevStatPChar(gh->gev, "\nObjective ranging:\n");
   gevStatPChar(gh->gev, "VARIABLE NAME                            LOWER           CURRENT             UPPER\n");
   gevStatPChar(gh->gev, "-------------                            -----           -------             -----\n");
   for( HighsInt iCol = 0; iCol < gmoN(gh->gmo); ++iCol )
   {
      if( gmoDict(gh->gmo) != NULL )
         gmoGetVarNameOne(gh->gmo, iCol, name);
      else
         sprintf(name, "n/a (index: %d)", iCol);

      sprintf(buffer, "%-28s %17g %17g %17g\n", name,
         ranging.col_cost_dn.value_[iCol],
         objcoefs[iCol],
         ranging.col_cost_up.value_[iCol]);
      gevStatPChar(gh->gev, buffer);
   }
   delete[] objcoefs;

   gevStatCoff(gh->gev);
   gevLogPChar(gh->gev, "Sensitivity analysis printed to listing file.\n");
}

// https://gurobi-optimization-gurobi-modelanalyzer.readthedocs-hosted.com/en/latest/quickstart_illcond.html
static
void doIllConditioningAnalysis(
   gamshighs_t* gh
)
{
   if( !gh->illconditioning )
      return;

   // the result looks more useful when column/rownames are available
   if( gmoDict(gh->gmo) != NULL )
   {
      char name[GMS_SSSIZE];
      for( int i = 0; i < gmoN(gh->gmo); ++i )
      {
         gmoGetVarNameOne(gh->gmo, i, name);
         gh->highs->passColName(i, name);
      }
      for( int i = 0; i < gmoM(gh->gmo); ++i )
      {
         gmoGetEquNameOne(gh->gmo, i, name);
         gh->highs->passRowName(i, name);
      }
   }

   // for now we just let HiGHS print result to log
   HighsIllConditioning illness;
   gh->highs->getIllConditioning(illness, gh->illconditioning_constraint, gh->illconditioning_method, gh->illconditioning_bound);
}

static
HighsStatus doIIS(
   gamshighs_t* gh,
   int          setsolveattribs
)
{
   HighsStatus status;
   HighsIis iis;
   char msgbuf[2*GMS_SSSIZE + 100];
   char name[GMS_SSSIZE];

   if( setsolveattribs )
   {
      gmoModelStatSet(gh->gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gh->gmo, gmoSolveStat_SolverErr);
   }

   if( gmoNDisc(gh->gmo) > 0 )
   {
      /* I think IIS is only available for LP at the moment */
      gevLogPChar(gh->gev, "\nIrreducible Inconsistent Subsystem (IIS) only available for LP so far.\n");

      if( setsolveattribs )
         gmoSolveStatSet(gh->gmo, gmoSolveStat_Capability);
      return HighsStatus::kOk;
   }

   gevLogPChar(gh->gev, "\nStarting Irreducible Inconsistent Subsystem (IIS) computation...\n");

   status = gh->highs->getIis(iis);
   if( status == HighsStatus::kError )
      return HighsStatus::kError;

   if( setsolveattribs )
   {
      /* store time and iterations of IIS */
      double iistime = 0.0;
      int iisiter = 0;
      for( size_t i = 0; i < iis.info_.size(); ++i )
      {
         iistime += iis.info_[i].simplex_time;
         iisiter += iis.info_[i].simplex_iterations;
      }
      gmoSetHeadnTail(gh->gmo, gmoHresused, iistime);
      gmoSetHeadnTail(gh->gmo, gmoHiterused, iisiter);
   }

   if( !iis.valid_ )
   {
      gevLogStatPChar(gh->gev, "\nNo IIS found.\n");
      if( setsolveattribs )
      {
         gmoModelStatSet(gh->gmo, gmoModelStat_NoSolutionReturned);  /* we have no feasible-nosolution status :( */
         gmoSolveStatSet(gh->gmo, gmoSolveStat_Normal);
      }
      return HighsStatus::kOk;
   }

   assert(iis.col_index_.size() == iis.col_bound_.size());
   assert(iis.row_index_.size() == iis.row_bound_.size());

   gevStatCon(gh->gev); /* without this, IIS doesn't show up in listing file, which we need for some test */

   gevLogStatPChar(gh->gev, "\nIIS found.\n");
   if( setsolveattribs )
   {
      gmoModelStatSet(gh->gmo, gmoModelStat_InfeasibleNoSolution);
      gmoSolveStatSet(gh->gmo, gmoSolveStat_Normal);
   }

   /* print equations in IIS */
   snprintf(msgbuf, sizeof(msgbuf), "Number of equations in IIS: %d\n", (int)iis.row_index_.size());
   gevLogStatPChar(gh->gev, msgbuf);

   for( size_t i = 0; i < iis.row_index_.size(); ++i )
   {
      int rowidx = iis.row_index_[i];
      if( gmoDict(gh->gmo) != NULL )
         gmoGetEquNameOne(gh->gmo, rowidx, name);
      else
         sprintf(name, "row%d", rowidx);

      switch( (enum IisBoundStatus)iis.row_bound_[i] )
      {
         case kIisBoundStatusDropped:
         case kIisBoundStatusNull:
         case kIisBoundStatusFree:
            // these should not happen in a final IIS
            break;
         case kIisBoundStatusLower:
            snprintf(msgbuf, sizeof(msgbuf), "  Lower: %s >= %g\n", name, gmoGetRhsOne(gh->gmo, rowidx));
            gevLogStatPChar(gh->gev, msgbuf);
            break;
         case kIisBoundStatusUpper:
            snprintf(msgbuf, sizeof(msgbuf), "  Upper: %s <= %g\n", name, gmoGetRhsOne(gh->gmo, rowidx));
            gevLogStatPChar(gh->gev, msgbuf);
            break;
         case kIisBoundStatusBoxed:
            snprintf(msgbuf, sizeof(msgbuf), "  Both:  %s  = %g\n", name, gmoGetRhsOne(gh->gmo, rowidx));
            gevLogStatPChar(gh->gev, msgbuf);
            break;
      }
   }

   /* print variable bounds in IIS */
   snprintf(msgbuf, sizeof(msgbuf), "Number of variables in IIS: %d\n", (int)iis.col_index_.size());
   gevLogStatPChar(gh->gev, msgbuf);

   for( size_t i = 0; i < iis.col_index_.size(); ++i )
   {
      int colidx = iis.col_index_[i];
      if( gmoDict(gh->gmo) != NULL )
         gmoGetVarNameOne(gh->gmo, colidx, name);
      else
         sprintf(name, "col%d", colidx);

      switch( (enum IisBoundStatus)iis.col_bound_[i] )
      {
         case kIisBoundStatusDropped:
         case kIisBoundStatusNull:
         case kIisBoundStatusFree:
            // these should not happen in a final IIS
            break;
         case kIisBoundStatusLower:
            snprintf(msgbuf, sizeof(msgbuf), "  Lower: %s >= %g\n", name, gmoGetVarLowerOne(gh->gmo, colidx));
            gevLogStatPChar(gh->gev, msgbuf);
            break;
         case kIisBoundStatusUpper:
            snprintf(msgbuf, sizeof(msgbuf), "  Upper: %s <= %g\n", name, gmoGetVarUpperOne(gh->gmo, colidx));
            gevLogStatPChar(gh->gev, msgbuf);
            break;
         case kIisBoundStatusBoxed:
            if( gmoGetVarLowerOne(gh->gmo, colidx) == gmoGetVarUpperOne(gh->gmo, colidx) )
               snprintf(msgbuf, sizeof(msgbuf), "  Both:  %s  = %g\n", name, gmoGetVarLowerOne(gh->gmo, colidx));
            else
               snprintf(msgbuf, sizeof(msgbuf), "  Lower: %s >= %g\n  Upper: %s <= %g\n", name, gmoGetVarLowerOne(gh->gmo, colidx), name, gmoGetVarUpperOne(gh->gmo, colidx));
            gevLogStatPChar(gh->gev, msgbuf);
            break;
      }
   }

   gevStatCoff(gh->gev);

   return HighsStatus::kOk;
}

extern "C" DllExport int hisCreate(void** Cptr, char* msgBuf, int msgBufLen);
int hisCreate(
   void**   Cptr,
   char*    msgBuf,
   int      msgBufLen
)
{
   assert(Cptr != NULL);
   assert(msgBufLen > 0);
   assert(msgBuf != NULL);

   *Cptr = calloc(1, sizeof(gamshighs_t));
   ((gamshighs_t*)*Cptr)->solvetracefile = new std::string;

   msgBuf[0] = 0;

   if( !gmoGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !gevGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !palGetReady(msgBuf, msgBufLen) )
      return 1;

   return 1;
}

extern "C" DllExport void hisFree(void** Cptr);
void hisFree(
   void** Cptr
)
{
   gamshighs_t* gh;

   assert(Cptr != NULL);
   assert(*Cptr != NULL);

   gh = (gamshighs_t*) *Cptr;
   assert(gh->solvetrace == NULL);

   delete gh->highs;
   delete gh->solvetracefile;

#ifdef GAMS_BUILD
   if( gh->cupdlplib_handle != NULL )
   {
      char buf[GMS_SSSIZE];
      buf[0] = '\0';
      unloadLibrary(gh->cupdlplib_handle, buf, GMS_SSSIZE);
      if( buf[0] != '\0' )
         gevLogStat(gh->gev, buf);
   }
#endif

   free(gh);

   /* free some thread-local global data */
   Highs::resetGlobalScheduler();

   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();
}

extern "C" DllExport int hisReadyAPI(void* Cptr, gmoHandle_t Gptr);
int hisReadyAPI(
   void*       Cptr,
   gmoHandle_t Gptr
)
{
   gamshighs_t* gh;
   char buffer[GMS_SSSIZE];
   char auditLine[GMS_SSSIZE];
   int rc = 1;

   assert(Cptr != NULL);
   assert(Gptr != NULL);

   gh = (gamshighs_t*) Cptr;
   gh->gmo = Gptr;
   gh->gev = (gevHandle_t) gmoEnvironment(gh->gmo);

   palHandle_t pal = NULL;
   /* initialize auditing/licensing library */
   if( !palCreate(&pal, buffer, sizeof(buffer)) )
   {
      gevLogStatPChar(gh->gev, "*** Could not create licensing object: ");
      gevLogStat(gh->gev, buffer);
      goto TERMINATE;
   }

   /* print GAMS audit line */
   palSetSystemName(pal, "HIGHS");
   sprintf(buffer, "\n%s\n", palGetAuditLine(pal, auditLine));
   gevLog(gh->gev, buffer);
   gevStatAudit(gh->gev, palGetAuditLine(pal,auditLine));

#ifdef GAMS_BUILD
   /* do some license check */
   GAMSinitLicensing(gh->gmo, pal);
   if( !GAMScheckHiGHSLicense(pal, false) )
   {
      gevLogStat(gh->gev, "*** No GAMS/HiGHS license available.");
      gevLogStat(gh->gev, "*** Please contact sales@gams.com to activate HiGHS in your license file.");
      gmoSolveStatSet(gh->gmo, gmoSolveStat_License);
      gmoModelStatSet(gh->gmo, gmoModelStat_LicenseError);
      return 1;
   }
#endif

   /* get the problem into a normal form */
   gmoObjStyleSet(gh->gmo, gmoObjType_Fun);
   gmoObjReformSet(gh->gmo, 1);
   gmoIndexBaseSet(gh->gmo, 0);
   gmoSetNRowPerm(gh->gmo); /* hide =N= rows */
   gmoMinfSet(gh->gmo, -kHighsInf);
   gmoPinfSet(gh->gmo, kHighsInf);

   gmoModelStatSet(gh->gmo, gmoModelStat_NoSolutionReturned);
   gmoSolveStatSet(gh->gmo, gmoSolveStat_SystemErr);

   if( setupProblem(gh) )
      goto TERMINATE;

   rc = 0;

TERMINATE:
   if( pal != NULL )
      palFree(&pal);

   return rc;
}

extern "C" DllExport int hisCallSolver(void* Cptr);
int hisCallSolver(
   void* Cptr
)
{
   gamshighs_t* gh;
   HighsStatus status;
   int rc = 1;

   gh = (gamshighs_t*) Cptr;
   assert(gh->gmo != NULL);
   assert(gh->gev != NULL);
   assert(gh->solvetrace == NULL);

   /* if we detected in readyAPI that HiGHS cannot handle the problem, then do nothing */
   if( gmoSolveStat(gh->gmo) == gmoSolveStat_Capability )
      return 0;

   /* get the problem into a normal form */
   gmoObjStyleSet(gh->gmo, gmoObjType_Fun);
   /* gmoObjReformSet(gh->gmo, 1); */
   gmoIndexBaseSet(gh->gmo, 0);
   // gmoSetNRowPerm(gh->gmo); /* hide =N= rows */
   gmoMinfSet(gh->gmo, -kHighsInf);
   gmoPinfSet(gh->gmo, kHighsInf);

   if( setupOptions(gh) )
      return 1;

   if( !gh->solvetracefile->empty() && gmoNDisc(gh->gmo) > 0 )
   {
      char buffer[GMS_SSSIZE];
      if( GAMSsolvetraceCreate(&gh->solvetrace, gh->solvetracefile->c_str(), "HiGHS", gmoOptFile(gh->gmo), gmoNameInput(gh->gmo, buffer), kHighsInf, gh->solvetracenodefreq, gh->solvetracetimefreq) != 0 )
      {
         gevLogStat(gh->gev, "Error: Could not create solvetrace file for writing.\n");
         goto TERMINATE;
      }
   }

   gevTimeSetStart(gh->gev);

   if( gh->iis == 2 )
   {
      if( doIIS(gh, 1) == HighsStatus::kOk )
         rc = 0;
      goto TERMINATE;
   }

   /* solve the problem */
   gevLogPChar(gh->gev, "\n");
   status = gh->highs->run();
   if( status == HighsStatus::kError )
      goto TERMINATE;

   /* pass solution, status, etc back to GMO */
   if( processSolve(gh) )
      goto TERMINATE;

   if( gh->iis == 1 && (gmoModelStat(gh->gmo) == gmoModelStat_InfeasibleGlobal || gmoModelStat(gh->gmo) == gmoModelStat_InfeasibleNoSolution) )
      if( doIIS(gh, 0) == HighsStatus::kError )
         goto TERMINATE;

   doRanging(gh);

   doIllConditioningAnalysis(gh);

   rc = 0;

 TERMINATE:
   if( gh->solvetrace != NULL )
      GAMSsolvetraceFree(&gh->solvetrace);

   return rc;
}

extern "C" DllExport int hisHaveModifyProblem(void* Cptr);
int hisHaveModifyProblem(
   void* Cptr
)
{
   return 0;
}

extern "C" DllExport int hisModifyProblem(void* Cptr);
int hisModifyProblem(
   void* Cptr
)
{
   gamshighs_t *gh = (gamshighs_t*) Cptr;
   assert(gh != NULL);

   /* set the GMO styles, in case someone changed this */
   gmoObjStyleSet(gh->gmo, gmoObjType_Fun);
   gmoObjReformSet(gh->gmo, 1);
   gmoIndexBaseSet(gh->gmo, 0);
   gmoSetNRowPerm(gh->gmo); /* hide =N= rows */
   gmoMinfSet(gh->gmo, -kHighsInf);
   gmoPinfSet(gh->gmo, kHighsInf);

   HighsInt maxsize = std::max(gmoN(gh->gmo), gmoM(gh->gmo));

   HighsInt jacnz = -1;
   gmoGetJacUpdate(gh->gmo, NULL, NULL, NULL, &jacnz);
   if( jacnz + 1 > maxsize )
      maxsize = jacnz + 1;

   HighsInt *colidx = new int[maxsize];
   HighsInt *rowidx = new int[maxsize];
   double *array1 = new double[maxsize];
   double *array2 = new double[maxsize];

   Highs *highs = gh->highs;
   assert(highs != NULL);

   // update objective coefficients
   HighsInt nz;
   HighsInt nlnz;
   gmoGetObjSparse(gh->gmo, colidx, array1, NULL, &nz, &nlnz);
   assert(nz == gmoObjNZ(gh->gmo));
   highs->changeColsCost(nz, colidx, array1);

   // update objective offset
   highs->changeObjectiveOffset(gmoObjConst(gh->gmo));

   // update variable bounds
   gmoGetVarLower(gh->gmo, array1);
   gmoGetVarUpper(gh->gmo, array2);
   highs->changeColsBounds(0, gmoN(gh->gmo)-1, array1, array2);

   // update constraint sides
   for( HighsInt i = 0; i < gmoM(gh->gmo); ++i )
   {
      double rhs = gmoGetRhsOne(gh->gmo, i);
      rowidx[i] = 1;
      switch( gmoGetEquTypeOne(gh->gmo, i) )
      {
         case gmoequ_E:
            array1[i] = rhs;
            array2[i] = rhs;
            break;

         case gmoequ_G:
            array1[i] = rhs;
            array2[i] = kHighsInf;
            break;

         case gmoequ_L:
            array1[i] = -kHighsInf;
            array2[i] = rhs;
            break;

         case gmoequ_N:
         case gmoequ_X:
         case gmoequ_C:
         case gmoequ_B:
            /* these should not occur */
            rowidx[i] = 0;
            break;
      }
   }
   highs->changeRowsBounds(rowidx, array1, array2);

   // update constraint matrix
   gmoGetJacUpdate(gh->gmo, rowidx, colidx, array1, &jacnz);
   for( HighsInt i = 0; i < jacnz; ++i )
      highs->changeCoeff(rowidx[i], colidx[i], array1[i]);

   delete[] array2;
   delete[] array1;
   delete[] rowidx;
   delete[] colidx;

   return 0;
}

#ifdef __GNUC__
__attribute__((constructor))
#endif
static void hisinit(void)
{
   gmoInitMutexes();
   gevInitMutexes();
   palInitMutexes();
}

#ifdef __GNUC__
__attribute__((destructor))
#endif
static void hisfini(void)
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
         hisinit();
         break;
      case DLL_PROCESS_DETACH:
         hisfini();
         break;
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
         break;
   }
   return TRUE;
}
#endif
