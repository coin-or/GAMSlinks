// Copyright (C) GAMS Development and others 2019-2022
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/* GAMS API */
#include "gevmcc.h"
#include "gmomcc.h"

/* HiGHS API */
#include "Highs.h"

typedef struct
{
   gmoHandle_t   gmo;
   gevHandle_t   gev;

   Highs*        highs;
} gamshighs_t;

static
void gevprint(
   HighsInt    level,
   const char* msg,
   void*       msgcb_data
)
{
   gevHandle_t gev = (gevHandle_t) msgcb_data;
   gevLogPChar(gev, msg);
}

static
void gevlog(
   HighsLogType type,
   const char*  msg,
   void*        msgcb_data
)
{
   gevHandle_t gev = (gevHandle_t) msgcb_data;
   if( type == HighsLogType::kInfo )
      gevLogPChar(gev, msg);
   else
      gevLogStatPChar(gev, msg);
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
   HighsInt i;

   assert(gh != NULL);
   assert(gh->highs == NULL);

   gh->highs = new Highs();

   // need to set log callbacks in options struct, since HiGHS::run() calls highsSetLogCallback(options)
   // TODO we should not need to uncast const-ness, but there is no method in HiGHS class to set log callbacks
   HighsOptions& options = const_cast<HighsOptions&>(gh->highs->getOptions());
   options.printmsgcb = gevprint;
   options.logmsgcb = gevlog;
   options.msgcb_data = (void*) gh->gev;
   highsSetLogCallback(options);

   numCol = gmoN(gh->gmo);
   numRow = gmoM(gh->gmo);
   numNz = gmoNZ(gh->gmo);

   /* columns */
   col_lower.resize(numCol);
   col_upper.resize(numCol);
   gmoGetVarLower(gh->gmo, col_lower.data());
   gmoGetVarUpper(gh->gmo, col_upper.data());

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
      astart.data(), aindex.data(), avalue.data());

   // gh->highs->writeModel("highs.lp");
   // gh->highs->writeModel("highs.mps");

   // pass initial solution
   HighsSolution sol;
   sol.col_value.resize(numCol);
   sol.col_dual.resize(numCol);
   sol.row_value.resize(numRow);
   sol.row_dual.resize(numRow);
   gmoGetVarL(gh->gmo, &sol.col_value[0]);
   gmoGetVarM(gh->gmo, &sol.col_dual[0]);
   gmoGetEquL(gh->gmo, &sol.row_value[0]);
   gmoGetEquM(gh->gmo, &sol.row_dual[0]);
   gh->highs->setSolution(sol);

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
   assert(gh != NULL);
   assert(gh->highs != NULL);

   gh->highs->resetOptions();

   gh->highs->setOptionValue("time_limit", gevGetDblOpt(gh->gev, gevResLim));
   gh->highs->setOptionValue("simplex_iteration_limit", gevGetIntOpt(gh->gev, gevIterLim));

   if( gevGetIntOpt(gh->gev, gevUseCutOff) )
      gh->highs->setOptionValue("objective_bound", gevGetDblOpt(gh->gev, gevCutOff));

   if( gmoOptFile(gh->gmo) > 0 )
   {
      char optfilename[GMS_SSSIZE];
      gmoNameOptFile(gh->gmo, optfilename);
      gh->highs->readOptions(optfilename);
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

   // figure out model and solution status and whether we should have a solution
   // to be written
   bool writesol = false;
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
         gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         writesol = true;
         break;

      case HighsModelStatus::kInfeasible:
         // TODO is there an infeasible solution to write?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_InfeasibleGlobal :
         // gmoModelStat_InfeasibleNoSolution);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         break;

      case HighsModelStatus::kUnboundedOrInfeasible:
         // TODO is there a (feasible) solution to write?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_Unbounded :
         // gmoModelStat_UnboundedNoSolution);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         break;

      case HighsModelStatus::kUnbounded:
         // TODO is there a (feasible) solution to write?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_Unbounded :
         // gmoModelStat_UnboundedNoSolution);
         gmoModelStatSet(gmo, gmoModelStat_UnboundedNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         break;

      case HighsModelStatus::kObjectiveBound:
         // TODO is there a solution to write and is it feasible?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_InfeasibleIntermed :
         // gmoModelStat_NoSolutionReturned);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         break;

      case HighsModelStatus::kObjectiveTarget:
         // TODO is there a solution to write and is it feasible?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_InfeasibleIntermed :
         // gmoModelStat_NoSolutionReturned);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         break;

      case HighsModelStatus::kTimeLimit:
         // TODO is there an (feasible) solution to write?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_InfeasibleIntermed :
         // gmoModelStat_NoSolutionReturned);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         break;

      case HighsModelStatus::kIterationLimit:
         // TODO is there an (feasible) solution to write?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_InfeasibleIntermed :
         // gmoModelStat_NoSolutionReturned);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         break;

      case HighsModelStatus::kUnknown:
         // TODO is there an (feasible) solution to write?
         // gmoModelStatSet(gmo, havesol ? gmoModelStat_InfeasibleIntermed :
         // gmoModelStat_NoSolutionReturned);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         break;
   }

   if( writesol )
   {
      const HighsSolution &sol = highs->getSolution();
      assert((HighsInt )sol.col_value.size() == gmoN(gmo));
      assert((HighsInt )sol.col_dual.size() == gmoN(gmo));
      assert((HighsInt )sol.row_value.size() == gmoM(gmo));
      assert((HighsInt )sol.row_dual.size() == gmoM(gmo));

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

         // TODO change when we can process infeasible or unbounded solutions
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

         // TODO change when we can process infeasible or unbounded solutions
         gmoVarEquStatus stat = gmoCstat_OK;

         gmoSetSolutionEquRec(gmo, i, sol.row_value[i], sol.row_dual[i], basisstat, stat);
      }

      // if there were =N= rows (lp08), then gmoCompleteObjective wouldn't get
      // their activity right
      // gmoCompleteObjective(gmo,
      // highs->getInfo().objective_function_value);
      gmoCompleteSolution(gmo);
   }

   return 0;
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
}

void hisFinalize(void)
{
   gmoFiniMutexes();
   gevFiniMutexes();
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
}


DllExport int STDCALL hisReadyAPI(
   void*       Cptr,
   gmoHandle_t Gptr
)
{
   gamshighs_t* gh;

   assert(Cptr != NULL);
   assert(Gptr != NULL);

   char msg[256];
   if( !gmoGetReady(msg, sizeof(msg)) )
      return 1;
   if( !gevGetReady(msg, sizeof(msg)) )
      return 1;

   gh = (gamshighs_t*) Cptr;
   gh->gmo = Gptr;
   gh->gev = (gevHandle_t) gmoEnvironment(gh->gmo);

   /* get the problem into a normal form */
   gmoObjStyleSet(gh->gmo, gmoObjType_Fun);
   gmoObjReformSet(gh->gmo, 1);
   gmoIndexBaseSet(gh->gmo, 0);
   gmoSetNRowPerm(gh->gmo); /* hide =N= rows */
   gmoMinfSet(gh->gmo, -kHighsInf);
   gmoPinfSet(gh->gmo, kHighsInf);

   if( setupProblem(gh) )
      return 1;

   return 0;
}

DllExport int STDCALL hisCallSolver(
   void* Cptr
)
{
   int rc = 1;
   gamshighs_t *gh;
   HighsStatus status;

   gh = (gamshighs_t*) Cptr;
   assert(gh->gmo != NULL);
   assert(gh->gev != NULL);

   gevLogStatPChar(gh->gev, "HiGHS " XQUOTE(HIGHS_VERSION_MAJOR) "." XQUOTE(HIGHS_VERSION_MINOR) "." XQUOTE(HIGHS_VERSION_PATCH) " [" HIGHS_GITHASH "]\n");

   gmoModelStatSet(gh->gmo, gmoModelStat_NoSolutionReturned);
   gmoSolveStatSet(gh->gmo, gmoSolveStat_SystemErr);

   /* get the problem into a normal form */
   gmoObjStyleSet(gh->gmo, gmoObjType_Fun);
   gmoObjReformSet(gh->gmo, 1);
   gmoIndexBaseSet(gh->gmo, 0);
   gmoSetNRowPerm(gh->gmo); /* hide =N= rows */
   gmoMinfSet(gh->gmo, -kHighsInf);
   gmoPinfSet(gh->gmo, kHighsInf);

   if( setupOptions(gh) )
      goto TERMINATE;

   gevTimeSetStart(gh->gev);

   /* solve the problem */
   status = gh->highs->run();
   if( status != HighsStatus::kOk )
      goto TERMINATE;

   /* pass solution, status, etc back to GMO */
   if( processSolve(gh) )
      goto TERMINATE;

   rc = 0;

TERMINATE:
   return rc;
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
   assert(nlnz == gmoObjNZ(gh->gmo));
   highs->changeColsCost(nz, colidx, array1);

   // update objective offset
   highs->changeObjectiveOffset(gmoObjConst(gh->gmo));

   // update variable bounds
   gmoGetVarLower(gh->gmo, array1);
   gmoGetVarUpper(gh->gmo, array2);
   highs->changeColsBounds(0, gmoN(gh->gmo), array1, array2);

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
   for( HighsInt i = 0; i < nz; ++i )
      highs->changeCoeff(rowidx[i], colidx[i], array1[i]);

   delete[] array2;
   delete[] array1;
   delete[] rowidx;
   delete[] colidx;

   return 0;
}
