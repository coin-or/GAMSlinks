// Copyright (C) GAMS Development and others 2016
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske
//
// TODO:
// - Ctrl^C
// - make use of high-precision / exact arithmetic solver (?)

#include "GamsLinksConfig.h"
#include "GamsSoPlex.hpp"
#include "GamsLicensing.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <stdexcept>
#include <algorithm>

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#include "palmcc.h"

#include "soplex/spxgithash.h"

using namespace soplex;


/** streambuf implementation that directs output to gev streams, taken from GamsOsi (which took it from CP Optimizer link) */
/* #define BUFFERSIZE GMS_SSSIZE */
#define BUFFERSIZE 1
class GamsOutputStreamBuf : public std::streambuf
{
private:
   gevHandle_t gev;
   char buffer[BUFFERSIZE+2];
   bool tostat;

public:
   GamsOutputStreamBuf(
      gevHandle_t gev_,
      bool        tostat_ = false
   )
   : gev(gev_), tostat(tostat_)
   {
      setp(buffer, buffer+BUFFERSIZE);
   }

   ~GamsOutputStreamBuf()
   {
      overflow(traits_type::eof());
   }

   int_type overflow(int_type c = traits_type::eof())
   {
      if( c != traits_type::eof() )
      {
         *pptr() = traits_type::to_char_type(c);
         *(pptr()+1) = '\0';
      }
      else
         *pptr() = '\0';

      if( tostat )
         gevLogStatPChar(gev, pbase());
      else
         gevLogPChar(gev, pbase());

      setp(buffer, buffer+BUFFERSIZE);

      return 0;
   }
};

static
const SPxSolver::VarStatus mapBasisStatus(
   enum gmoVarEquBasisStatus gmostatus,
   bool haslb,
   bool hasub
   )
{
   switch( gmostatus )
   {
      case gmoBstat_Lower:
         return SPxSolver::ON_LOWER;
      case gmoBstat_Upper:
         return SPxSolver::ON_UPPER;
      case gmoBstat_Basic:
         return SPxSolver::BASIC;
      case gmoBstat_Super:
         return (haslb ? SPxSolver::ON_LOWER : hasub ? SPxSolver::ON_UPPER : SPxSolver::ZERO);
      default:
         return SPxSolver::UNDEFINED;
   }
}

void GamsSoPlex::setupLP()
{
   assert(soplex != NULL);

   // clear old problem, if any
   soplex->clearLPReal();

   double* objcoefs = new double[gmoN(gmo)];
   double* rowcoefs = new double[gmoM(gmo)];
   int* rowidx = new int[gmoM(gmo)];
   int nz, nlnz;

   // get objective
   gmoGetObjVector(gmo, objcoefs, NULL);

   // setup variables: objective coefficients, bounds, and row coefficients
   for( int i = 0; i < gmoN(gmo); ++i )
   {
      gmoGetColSparse(gmo, i, rowidx, rowcoefs, NULL, &nz, &nlnz);
      assert(nz >= 0);
      assert(nz <= gmoM(gmo));
      // nlnz should be 0, but within GUSS, this doesn't seem to work correctly (GMO bug?)
      //assert(nlnz == 0);

      DSVector col(nz);
      for( int j = 0; j < nz; ++j )
         if( rowcoefs[j] != 0.0 )
            col.add(rowidx[j], rowcoefs[j]);
      soplex->addColReal(LPCol(objcoefs[i], col, gmoGetVarUpperOne(gmo, i), gmoGetVarLowerOne(gmo, i)));
   }

   // if there were empty rows at the end of the instance, they were not created when adding columns
   for( int j = soplex->numRowsReal(); j < gmoM(gmo); ++j )
      soplex->addRowReal(LPRow());
   assert(soplex->numRowsReal() == gmoM(gmo));

   // setup row sides
   // SoPlex sometimes adds a default side of 0 for rows when putting in a column, so change both sides here in any case
   for( int j = 0; j < gmoM(gmo); ++j )
   {
      double rhs = gmoGetRhsOne(gmo, j);
      switch( gmoGetEquTypeOne(gmo, j) )
      {
         case gmoequ_E :
            soplex->changeRangeReal(j, rhs, rhs);
            break;
         case gmoequ_G :
            soplex->changeRangeReal(j, rhs, infinity);
            break;
         case gmoequ_L :
            soplex->changeRangeReal(j, -infinity, rhs);
            break;
         default:
            throw std::runtime_error("Unexpected equation type.");
      }
   }

   // objective sense will be setup with parameters

   delete[] objcoefs;
   delete[] rowcoefs;
   delete[] rowidx;

   // setup initial basis
   if( gmoHaveBasis(gmo) )
   {
      SPxSolver::VarStatus* colstatus = new SPxSolver::VarStatus[soplex->numColsReal()];
      SPxSolver::VarStatus* rowstatus = new SPxSolver::VarStatus[soplex->numRowsReal()];

      for( int i = 0; i < gmoN(gmo); ++i )
         colstatus[i] = mapBasisStatus((enum gmoVarEquBasisStatus)gmoGetVarStatOne(gmo, i), gmoGetVarLowerOne(gmo, i) != gmoMinf(gmo), gmoGetVarUpperOne(gmo, i) != gmoPinf(gmo));

      for( int j = 0; j < gmoM(gmo); ++j )
         rowstatus[j] = mapBasisStatus((enum gmoVarEquBasisStatus)gmoGetEquStatOne(gmo, j), gmoGetEquTypeOne(gmo, j) != gmoequ_L, gmoGetEquTypeOne(gmo, j) != gmoequ_G);

      soplex->setBasis(rowstatus, colstatus);

      delete[] colstatus;
      delete[] rowstatus;
   }
}

void GamsSoPlex::setupParameters()
{
   soplex->resetSettings(true, true);

   // set iterlimit, if not at maximal value
   if( gevGetIntOpt(gev, gevIterLim) < INT_MAX )
      soplex->setIntParam(SoPlex::ITERLIMIT, gevGetIntOpt(gev, gevIterLim), true);

   // set timelimit
   soplex->setRealParam(SoPlex::TIMELIMIT, gevGetDblOpt(gev, gevResLim), true);

   // set clock type to wallclock time
   soplex->setIntParam(SoPlex::TIMER, SoPlex::TIMER_WALLCLOCK, true);

   // reduce soplex verbosity when log is disabled anyway
   if( gevGetIntOpt(gev, gevLogOption) == 0 )
      soplex->spxout.setVerbosity(soplex::SPxOut::ERROR);

   // set objective sense
   soplex->setIntParam(SoPlex::OBJSENSE, gmoSense(gmo) == gmoObj_Min ? SoPlex::OBJSENSE_MINIMIZE : SoPlex::OBJSENSE_MAXIMIZE, true);

   // set objective offset
   soplex->setRealParam(SoPlex::OBJ_OFFSET, gmoObjConst(gmo));

   // set cutoff
   if( gevGetIntOpt(gev, gevUseCutOff) )
      soplex->setRealParam(gmoSense(gmo) == gmoObj_Min ? SoPlex::OBJLIMIT_UPPER : SoPlex::OBJLIMIT_LOWER, gevGetDblOpt(gev, gevCutOff), true);

   // read options file
   if( gmoOptFile(gmo) > 0 )
   {
      char buffer[GMS_SSSIZE];

      gmoNameOptFile(gmo, buffer);
      soplex->loadSettingsFile(buffer);
   }

   gevLog(gev, "Parameter settings:");
   soplex->printUserSettings();
}

void GamsSoPlex::writeInstance()
{
   // setup variable/equation names if desired and available
   NameSet* colnames = NULL;
   NameSet* rownames = NULL;
   if( gevGetIntOpt(gev, gevInteger2) && gmoDict(gmo) != NULL )
   {
      char buffer[GMS_SSSIZE];
      colnames = new NameSet(soplex->numColsReal());
      rownames = new NameSet(soplex->numRowsReal());

      for( int i = 0; i < gmoN(gmo); ++i )
         colnames->add(gmoGetVarNameOne(gmo, i, buffer));

      for( int j = 0; j < gmoM(gmo); ++j )
         rownames->add(gmoGetEquNameOne(gmo, j, buffer));
   }

   // write mps file, if requested
   if( gevGetIntOpt(gev, gevInteger3) & 0x1 )
   {
      char buffer[GMS_SSSIZE+30];

      gmoNameInput(gmo, buffer);
      strcat(buffer, ".mps");

      gevLogStatPChar(gev, "Writing MPS file ");
      gevLogStat(gev, buffer);

      soplex->writeFileReal(buffer, rownames, colnames);
   }

   // write lp file, if requested
   if( gevGetIntOpt(gev, gevInteger3) & 0x2 )
   {
      char buffer[GMS_SSSIZE+30];

      gmoNameInput(gmo, buffer);
      strcat(buffer, ".lp");

      gevLogStatPChar(gev, "Writing LP file ");
      gevLogStat(gev, buffer);

      soplex->writeFileReal(buffer, rownames, colnames);
   }

   // write soplex state files, if requested
   if( gevGetIntOpt(gev, gevInteger3) & 0x4 )
   {
      char buffer[GMS_SSSIZE+30];

      gmoNameInput(gmo, buffer);

      gevLogStatPChar(gev, "Writing SoPlex state files ");
      gevLogStatPChar(gev, buffer);
      gevLogStat(gev, ".{bas,lp,set}");

      soplex->writeStateReal(buffer, rownames, colnames, true);
   }

   delete colnames;
   delete rownames;
}

GamsSoPlex::~GamsSoPlex()
{
   if( soplex != NULL )
   {
      soplex->~SoPlex();
      spx_free(soplex);
   }

   delete logstream;
   logstream = NULL;

   delete logstreambuf;
   logstreambuf = NULL;

   if( pal != NULL )
      palFree(&pal);
}

int GamsSoPlex::readyAPI(
   struct gmoRec*     gmo_                /**< GAMS modeling object */
)
{
   gmo = gmo_;
   assert(gmo != NULL);

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   char buffer[512];
   if( pal == NULL && !palCreate(&pal, buffer, sizeof(buffer)) )
   {
      gevLogStat(gev, buffer);
      return 1;
   }

#if PALAPIVERSION >= 3
   palSetSystemName(pal, "Soplex");
   palGetAuditLine(pal, buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

   GAMSinitLicensing(gmo, pal);

   // check for academic license, or if we run in demo mode
   if( !GAMScheckSCIPLicense(pal, false) )
   {
      gevLogStat(gev, "*** No SoPlex license available.");
      gevLogStat(gev, "*** Please contact sales@gams.com to arrange for a license.");
      gmoSolveStatSet(gmo, gmoSolveStat_License);
      gmoModelStatSet(gmo, gmoModelStat_LicenseError);
      return 1;
   }

   if( gmoModelType(gmo) != gmoProc_lp && gmoModelType(gmo) != gmoProc_rmip )
   {
      gevLogStat(gev, "ERROR: SoPlex can solve only linear programs.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 1;
   }

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);
   gmoPinfSet(gmo,  infinity);
   gmoMinfSet(gmo, -infinity);
   gmoSetNRowPerm(gmo);

   if( logstreambuf != NULL )
   {
      // delete logstream(buf) from previous readyAPI call: gev may have changed
      delete logstreambuf;
      delete logstream;
   }

   // create stream (+buffer) for writing to log
   logstreambuf = new GamsOutputStreamBuf(gev);
   logstream = new std::ostream(logstreambuf);

   if( soplex == NULL )
   {
      // create a new SoPlex instance
      spx_alloc(soplex);
      new (soplex) SoPlex();
   }

   // redirect SoPlex output
   soplex->spxout.setStream(soplex::SPxOut::ERROR, *logstream);
   soplex->spxout.setStream(soplex::SPxOut::WARNING, *logstream);
   soplex->spxout.setStream(soplex::SPxOut::INFO1, *logstream);
   soplex->spxout.setStream(soplex::SPxOut::INFO2, *logstream);
   soplex->spxout.setStream(soplex::SPxOut::INFO3, *logstream);
   soplex->spxout.setStream(soplex::SPxOut::DEBUG, *logstream);

   // setup LP
   setupLP();

   return 0;
}

static
enum gmoVarEquBasisStatus mapBasisStatus(
   const SPxSolver::VarStatus soplexstatus
   )
{
   switch( soplexstatus )
   {
      case SPxSolver::FIXED:  // variable fixed to identical bounds
      case SPxSolver::ON_UPPER:
         return gmoBstat_Upper;
      case SPxSolver::ON_LOWER:
         return gmoBstat_Lower;
      case SPxSolver::ZERO:  // free variable fixed to zero
         return gmoBstat_Super;
      case SPxSolver::BASIC:
         return gmoBstat_Basic;
      case SPxSolver::UNDEFINED:
         return gmoBstat_Super;
      default:
         throw std::runtime_error("Unexpected SoPlex basis status.");
   }
}

int GamsSoPlex::callSolver()
{
   assert(gmo   != NULL);
   assert(gev   != NULL);
   assert(soplex != NULL);

   // print version info and copyright
   char buffer[50];
   sprintf(buffer, "\nSoPlex version %d.%d (%s)\n", SOPLEX_VERSION/100, (SOPLEX_VERSION % 100)/10, getGitHash());
   gevLogStatPChar(gev, buffer);
   gevLogStatPChar(gev, SOPLEX_COPYRIGHT "\n\n");

   // setup parameters
   setupParameters();

   // write problem instance and parameters, if desired
   writeInstance();

   SPxSolver::Status stat;
   try
   {
      // run SoPlex
      stat = soplex->solve();
   }
   catch( const soplex::SPxStatusException& )
   {
      // TODO check whether exception is indeed about singular basis
      stat = SPxSolver::SINGULAR;
   }

   if( stat == SPxSolver::SINGULAR && soplex->numIterations() <= 1 )
   {
      gevLog(gev, "Retry without initial basis.\n");
      soplex->clearBasis();
      stat = soplex->solve();
   }

   // evaluate SoPlex return code
   switch( stat )
   {
      case SPxSolver::ABORT_TIME :
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         gmoModelStatSet(gmo, soplex->hasPrimal() ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
         break;

      case SPxSolver::ABORT_ITER :
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         gmoModelStatSet(gmo, soplex->hasPrimal() ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
         break;

      case SPxSolver::ABORT_CYCLING:
      case SPxSolver::ABORT_VALUE :  // objective limit: GAMS doesn't have a proper model status for this, so return terminated-by-solver
      case SPxSolver::SINGULAR:  // numerical troubles
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         gmoModelStatSet(gmo, soplex->hasPrimal() ? gmoModelStat_Feasible : gmoModelStat_NoSolutionReturned);
         break;

      case SPxSolver::OPTIMAL :
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
         assert(soplex->hasPrimal());
         break;

      case SPxSolver::OPTIMAL_UNSCALED_VIOLATIONS :
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         assert(soplex->hasPrimal());
         break;

      case SPxSolver::UNBOUNDED :
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, soplex->hasPrimal() ? gmoModelStat_Unbounded : gmoModelStat_UnboundedNoSolution);
         break;

      case SPxSolver::INFEASIBLE :
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, soplex->hasPrimal() ? gmoModelStat_InfeasibleGlobal : gmoModelStat_InfeasibleNoSolution);
         break;

      case SPxSolver::INForUNBD:
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
         break;

      default:
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
   }

   if( gevGetIntOpt(gev, gevInteger1) )
      soplex->printStatistics(*logstream);

   int ncols = soplex->numColsReal();
   int nrows = soplex->numRowsReal();
   assert(ncols == gmoN(gmo) || ncols == gmoN(gmo)+1);  /* can be +1 if objective offset */
   assert(nrows == gmoM(gmo));

   // TODO if stopped on iteration or timelimit, can we get the current basis and solution? (soplex doesn't seem to report any)

   if( soplex->hasPrimal() )
   {
      DVector primal(ncols);
      DVector activities(nrows);
      DVector redcost(ncols);
      DVector dual(nrows);
      DVector* primalray = NULL;

      double feastol = soplex->realParam(SoPlex::FEASTOL);
      double opttol = soplex->realParam(SoPlex::OPTTOL);

      soplex->getPrimal(primal);
      soplex->getSlacksReal(activities);

      if( soplex->hasDual() )
      {
         soplex->getRedCost(redcost);
         soplex->getDual(dual);
      }

      if( soplex->hasPrimalRay() )
      {
         primalray = new DVector(ncols);
         soplex->getPrimal(*primalray);
      }

      for( int i = 0; i < gmoN(gmo); ++i )
      {
         gmoVarEquBasisStatus basisstatus;
         gmoVarEquStatus otherstatus;

         // take basis status from SoPlex, if have basis
         if( soplex->hasBasis() )
            basisstatus = mapBasisStatus(soplex->basisColStatus(i));
         else
            basisstatus = gmoBstat_Basic;

         // set redcost to 0, if not given by SoPlex
         if( !soplex->hasDual() )
            redcost[i] = 0.0;

         switch( stat )
         {
            case SPxSolver::INFEASIBLE :
            {
               if( gmoGetVarLowerOne(gmo, i) != gmoMinf(gmo) && primal[i] < gmoGetVarLowerOne(gmo, i) - feastol )
                  otherstatus = gmoCstat_Infeas;
               else if( gmoGetVarUpperOne(gmo, i) != gmoPinf(gmo) && primal[i] > gmoGetVarUpperOne(gmo, i) + feastol )
                  otherstatus = gmoCstat_Infeas;
               else
                  otherstatus = gmoCstat_OK;
               break;
            }

            case SPxSolver::UNBOUNDED :
            {
               if( primalray != NULL && fabs((*primalray)[i]) > opttol )
                  otherstatus = gmoCstat_UnBnd;
               else
                  otherstatus = gmoCstat_OK;
               break;
            }

            case SPxSolver::OPTIMAL :
            case SPxSolver::OPTIMAL_UNSCALED_VIOLATIONS :
               otherstatus = gmoCstat_OK;
               break;

            default:
               /* make up some NonOpt markers */
               if( basisstatus == gmoBstat_Lower && primal[i] > gmoGetVarLowerOne(gmo, i) + feastol )
                  otherstatus = gmoCstat_NonOpt;
               else if( basisstatus == gmoBstat_Upper && primal[i] < gmoGetVarUpperOne(gmo, i) - feastol )
                  otherstatus = gmoCstat_NonOpt;
               else
                  otherstatus = gmoCstat_OK;
         }

         gmoSetSolutionVarRec(gmo, i, primal[i], redcost[i], basisstatus, otherstatus);
      }

      for( int i = 0; i < gmoM(gmo); ++i )
      {
         gmoVarEquBasisStatus basisstatus;
         gmoVarEquStatus otherstatus;

         // take basis status from SoPlex, if have basis
         if( soplex->hasBasis() )
            basisstatus = mapBasisStatus(soplex->basisRowStatus(i));
         else
            basisstatus = gmoBstat_Basic;

         // set dual to 0, if not given by SoPlex
         if( !soplex->hasDual() )
            dual[i] = 0.0;


         switch( stat )
         {
            case SPxSolver::INFEASIBLE :
            {
               if( gmoGetEquTypeOne(gmo, i) != gmoequ_L && activities[i] < gmoGetRhsOne(gmo, i) - feastol )
                  otherstatus = gmoCstat_Infeas;
               else if( gmoGetEquTypeOne(gmo, i) != gmoequ_G && activities[i] > gmoGetRhsOne(gmo, i) + feastol )
                  otherstatus = gmoCstat_Infeas;
               else
                  otherstatus = gmoCstat_OK;
               break;
            }

            case SPxSolver::UNBOUNDED :
            case SPxSolver::OPTIMAL :
            case SPxSolver::OPTIMAL_UNSCALED_VIOLATIONS :
               otherstatus = gmoCstat_OK;
               break;

            default:
               /* make up some NonOpt markers
                * if constraint not active (activity away from lhs/rhs, in basis), then it should have dualvalue 0 in optimal solution
                */
               if( (fabs(activities[i] - gmoGetRhsOne(gmo, i)) > feastol || basisstatus == gmoBstat_Basic) && fabs(dual[i]) > opttol )
                  otherstatus = gmoCstat_NonOpt;
               else
                  otherstatus = gmoCstat_OK;
         }

         gmoSetSolutionEquRec(gmo, i, activities[i], dual[i], basisstatus, otherstatus);
      }

      delete primalray;
   }

   gmoSetHeadnTail(gmo, gmoHiterused, soplex->numIterations());
   gmoSetHeadnTail(gmo, gmoHresused, soplex->solveTime());

   // get the objective value record set
   gmoCompleteSolution(gmo);

   return 0;
}

int GamsSoPlex::modifyProblem()
{
   /* set the GMO styles, in case someone changed this */
   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoObjReformSet(gmo, 1);
   gmoIndexBaseSet(gmo, 0);
   gmoPinfSet(gmo,  infinity);
   gmoMinfSet(gmo, -infinity);
   gmoSetNRowPerm(gmo);

   assert(soplex != NULL);

   /* update objective coefficients */
   DVector objcoefs(soplex->numColsReal());
   gmoGetObjVector(gmo, objcoefs.get_ptr(), NULL);
   soplex->changeObjReal(objcoefs);

   // update objective offset
   soplex->setRealParam(SoPlex::OBJ_OFFSET, gmoObjConst(gmo));

   // update variable bounds
   for( int i = 0; i < gmoN(gmo); ++i )
      soplex->changeBoundsReal(i, gmoGetVarLowerOne(gmo, i), gmoGetVarUpperOne(gmo, i));

   // update constraint sides
   for( int j = 0; j < gmoM(gmo); ++j )
   {
      double rhs = gmoGetRhsOne(gmo, j);
      switch( gmoGetEquTypeOne(gmo, j) )
      {
         case gmoequ_E :
            soplex->changeRangeReal(j, rhs, rhs);
            break;
         case gmoequ_G :
            soplex->changeRangeReal(j, rhs, infinity);
            break;
         case gmoequ_L :
            soplex->changeRangeReal(j, -infinity, rhs);
            break;
         default:
            throw std::runtime_error("Unexpected equation type.");
      }
   }

   // update constraint matrix
   int nz = -1;
   gmoGetJacUpdate(gmo, NULL, NULL, NULL, &nz);
   int* rowidx = new int[nz+1];
   int* colidx = new int[nz+1];
   double* coefs = new double[nz+1];

   gmoGetJacUpdate(gmo, rowidx, colidx, coefs, &nz);
   for( int i = 0; i < nz; ++i )
      soplex->changeElementReal(rowidx[i], colidx[i], coefs[i]);

   delete[] rowidx;
   delete[] colidx;
   delete[] coefs;

   return 0;
}

#define GAMSSOLVER_ID osp
#define GAMSSOLVER_HAVEMODIFYPROBLEM
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

   *Cptr = (void*) new GamsSoPlex();
   if( *Cptr == NULL )
   {
      snprintf(msgBuf, msgBufLen, "Out of memory when creating GamsSoPlex object.\n");
      if( msgBufLen > 0 )
         msgBuf[msgBufLen] = '\0';
      return 0;
   }

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   assert(Cptr != NULL);

   delete (GamsSoPlex*)*Cptr;
   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsSoPlex*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,HaveModifyProblem)(void* Cptr)
{
   return 0;
}

DllExport int  STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ModifyProblem)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsSoPlex*)Cptr)->modifyProblem();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsSoPlex*)Cptr)->readyAPI(Gptr);
}
