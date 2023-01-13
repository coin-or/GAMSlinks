// Copyright (C) GAMS Development and others 2019-2022
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
#ifdef GAMS_BUILD
#include "GamsLicensing.h"
#endif

/* HiGHS API */
#include "Highs.h"

typedef struct
{
   gmoHandle_t   gmo;
   gevHandle_t   gev;

   Highs*        highs;
   bool          ranging;
   bool          mipstart;

   bool          interrupted;
} gamshighs_t;

static
void gevlog(
   HighsLogType type,
   const char*  msg,
   void*        msgcb_data
)
{
   gamshighs_t* gh = (gamshighs_t*) msgcb_data;
   assert(gh != NULL);

   if( gh->interrupted )
   {
      // replace "Time limit reached" in status string by "User interrupt"
      char* timelim = strstr((char*)msg, "Time limit reached");
      if( timelim != NULL )
         memcpy(timelim, "User interrupt    ", sizeof("Time limit reached")-1);  // -1 to skip trailing \0
   }

   if( type == HighsLogType::kInfo )
      gevLogPChar(gh->gev, msg);
   else
      gevLogStatPChar(gh->gev, msg);
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

#if GMOAPIVERSION >= 22
   if( gmoNZ64(gh->gmo) > INT_MAX )
   {
      gevLogStat(gh->gev, "ERROR: Problems with more than 2^31 nonzeros not supported by HiGHS link.");
      gmoSolveStatSet(gh->gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gh->gmo, gmoModelStat_NoSolutionReturned);
      return 0;
   }
#endif

   gh->highs = new Highs();
   gh->highs->setLogCallback(gevlog, gh);

   const_cast<HighsOptions&>(gh->highs->getOptions()).records.push_back(
      new OptionRecordBool("sensitivity",
         "Whether to run sensitivity analysis after solving an LP with a simplex method",
         false, &gh->ranging, false) );

   const_cast<HighsOptions&>(gh->highs->getOptions()).records.push_back(
      new OptionRecordBool("mipstart",
         "Whether to pass initial level values as starting point to MIP solver",
         false, &gh->mipstart, false) );

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

   // pass initial solution, if LP and rows are present (work around segfault on lp11, https://github.com/ERGO-Code/HiGHS/issues/1072)
   // for a MIP, we consider setting a starting point in setupOptions()
   // so for an LP, the users starting point is set only once and not for every solve after a problem modification
   // for a MIP, we pass the users starting point to each solve
   if( gmoNDisc(gh->gmo) == 0 && numRow > 0 )
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
   if( gevGetIntOpt(gh->gev, gevUseCutOff) )
      gh->highs->setOptionValue("objective_bound", gevGetDblOpt(gh->gev, gevCutOff));
   if( gevGetIntOpt(gh->gev, gevNodeLim) > 0 )
      gh->highs->setOptionValue("mip_max_nodes", gevGetIntOpt(gh->gev, gevNodeLim));
   gh->highs->setOptionValue("mip_rel_gap", gevGetDblOpt(gh->gev, gevOptCR));
   gh->highs->setOptionValue("mip_abs_gap", gevGetDblOpt(gh->gev, gevOptCA));
   gh->highs->setOptionValue("threads", gevThreads(gh->gev));
   if( gevGetIntOpt(gh->gev, gevLogOption) == 0 )
      gh->highs->setOptionValue("output_flag", false);

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

   // write problem to file, if requested
   bool writemodel;
   gh->highs->getOptionValue("write_model_to_file", writemodel);
   if( writemodel )
   {
      std::string modelfile;

      gh->highs->getOptionValue("write_model_file", modelfile);
      if( modelfile.empty() )
      {
         // if no filename specified, make up one from inputname
         gmoNameInput(gh->gmo, buf);
         modelfile = buf;
         modelfile += ".lp";
      }
      sprintf(buf, "Writing model instance to %s.\n", modelfile.c_str());
      gevLogPChar(gh->gev, buf);

      gh->highs->writeModel(modelfile);
   }

   // pass initial solution if mipstart set and MIP without semicontinuous/integer variables (workaround bug https://github.com/ERGO-Code/HiGHS/issues/1074)
   if( gh->mipstart && gmoNDisc(gh->gmo) > 0 && gmoGetVarTypeCnt(gh->gmo, gmovar_SC) == 0 && gmoGetVarTypeCnt(gh->gmo, gmovar_SI) == 0 )
   {
      HighsSolution sol;
      sol.col_value.resize(gmoN(gh->gmo));
      sol.col_dual.resize(gmoN(gh->gmo));
      sol.row_value.resize(gmoM(gh->gmo));
      sol.row_dual.resize(gmoM(gh->gmo));
      gmoGetVarL(gh->gmo, &sol.col_value[0]);
      gmoGetVarM(gh->gmo, &sol.col_dual[0]);
      if( gmoM(gh->gmo) )
      {
         gmoGetEquL(gh->gmo, &sol.row_value[0]);
         gmoGetEquM(gh->gmo, &sol.row_dual[0]);
      }
      gh->highs->setSolution(sol);
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
         gmoModelStatSet(gmo, sol.value_valid ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         break;

      case HighsModelStatus::kTimeLimit:
         gmoModelStatSet(gmo, sol.value_valid ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
         if( gh->interrupted )
            gmoSolveStatSet(gmo, gmoSolveStat_User);
         else
            gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         break;

      case HighsModelStatus::kIterationLimit:
         // may also mean node limit
         gmoModelStatSet(gmo, sol.value_valid ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         break;

      case HighsModelStatus::kUnknown:
         gmoModelStatSet(gmo, sol.value_valid ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
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

   // write solution to extra file, if requested
   if( sol.value_valid )
   {
      bool writesol;
      gh->highs->getOptionValue("write_solution_to_file", writesol);
      if( writesol )
      {
         char buf[GMS_SSSIZE+50];
         std::string solfile;
         HighsInt solstyle;

         gh->highs->getOptionValue("solution_file", solfile);
         if( solfile.empty() )
         {
            // if no filename specified, make up one from inputname
            gmoNameInput(gh->gmo, buf);
            solfile = buf;
            solfile += ".sol";
         }

         gh->highs->getOptionValue("write_solution_style", solstyle);

         sprintf(buf, "Writing solution to %s using style %d.\n", solfile.c_str(), solstyle);
         gevLogPChar(gh->gev, buf);

         gh->highs->writeSolution(solfile, solstyle);
      }
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

#define GAMSSOLVER_ID his
#define GAMSSOLVER_HAVEMODIFYPROBLEM
#include "GamsEntryPoints_tpl.c"

#define XQUOTE(x) QUOTE(x)
#define QUOTE(x) #x

void hisInitialize(void)
{
   gmoInitMutexes();
   gevInitMutexes();
   palInitMutexes();
}

void hisFinalize(void)
{
   gmoFiniMutexes();
   gevFiniMutexes();
   palFiniMutexes();
}

DllExport int STDCALL hisCreate(
   void**   Cptr,
   char*    msgBuf,
   int      msgBufLen
)
{
   assert(Cptr != NULL);
   assert(msgBufLen > 0);
   assert(msgBuf != NULL);

   *Cptr = calloc(1, sizeof(gamshighs_t));

   msgBuf[0] = 0;

   if( !gmoGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !gevGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !palGetReady(msgBuf, msgBufLen) )
      return 1;

   return 1;
}

DllExport void STDCALL hisFree(
   void** Cptr
)
{
   gamshighs_t* gh;

   assert(Cptr != NULL);
   assert(*Cptr != NULL);

   gh = (gamshighs_t*) *Cptr;

   delete gh->highs;
   free(gh);

   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();
}

DllExport int STDCALL hisReadyAPI(
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

static thread_local gamshighs_t* gh_terminate = NULL;
static
void interruptHighs(void)
{
   assert(gh_terminate != NULL);

   // there is no proper functionality to interrupt HiGHS and it isn't coming soon (https://github.com/ERGO-Code/HiGHS/issues/903)
   // so we reset the timelimit instead

   if( gh_terminate->interrupted ) // somehow we are called twice
      return;

   gh_terminate->highs->setOptionValue("time_limit", DBL_MIN);
   gh_terminate->interrupted = true;
}

DllExport int STDCALL hisCallSolver(
   void* Cptr
)
{
   gamshighs_t* gh;
   HighsStatus status;

   gh = (gamshighs_t*) Cptr;
   assert(gh->gmo != NULL);
   assert(gh->gev != NULL);

   // if we detected in readyAPI that HiGHS cannot handle the problem, then do nothing */
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

   gh_terminate = gh;
   gh->interrupted = false;
   gevTerminateSet(gh->gev, NULL, (void*)interruptHighs);

   gevTimeSetStart(gh->gev);

   /* solve the problem */
   gevLogPChar(gh->gev, "\n");
   status = gh->highs->run();
   if( status == HighsStatus::kError )
      return 1;

   /* pass solution, status, etc back to GMO */
   if( processSolve(gh) )
      return 1;

   doRanging(gh);

   return 0;
}

DllExport int STDCALL hisModifyProblem(
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

   HighsInt jacnz;
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
