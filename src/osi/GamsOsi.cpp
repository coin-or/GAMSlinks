// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsLinksConfig.h"
#include "GamsOsi.hpp"

#include "OsiConfig.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

#include "gclgms.h"
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gevlice.h"
#include "GamsLicensing.h"
#endif
#include "palmcc.h"

#include "GamsMessageHandler.hpp"
#include "GamsOsiHelper.hpp"

#include "OsiSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinPackedVector.hpp"
#include "CoinWarmStartBasis.hpp"

#ifdef GAMSLINKS_HAS_OSICPX
#include "OsiCpxSolverInterface.hpp"
#include "cplex.h"

static int CPXPUBLIC cpxinfocallback(CPXCENVptr cpxenv, void* cbdata, int wherefrom, void* cbhandle)
{
   return gevTerminateGet((gevHandle_t)cbhandle);
}
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
#include "OsiGrbSolverInterface.hpp"
extern "C"
{
#include "gurobi_c.h"
}

static int __stdcall grbcallback(GRBmodel* model, void* qcbdata, int where, void* usrdata)
{
   if( where == GRB_CB_MESSAGE )
   {
      char* msg;
      GRBcbget(qcbdata, where, GRB_CB_MSG_STRING, &msg);
      gevLogPChar((gevHandle_t)usrdata, msg);
   }

   if( gevTerminateGet((gevHandle_t)usrdata) )
      GRBterminate(model);

   return 0;
}
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
#include "OsiMskSolverInterface.hpp"
#include "mosek.h"

static int MSKAPI mskcallback(MSKtask_t task, MSKuserhandle_t handle, MSKcallbackcodee caller,
   const MSKrealt * douinf,
   const MSKint32t * intinf,
   const MSKint64t * lintinf
)
{
   return gevTerminateGet((gevHandle_t)handle);
}
#endif

#ifdef GAMSLINKS_HAS_OSIXPR
#include "OsiXprSolverInterface.hpp"
#include "xprs.h"
extern "C" void XPRS_CC XPRScommand(XPRSprob, char*);

static int XPRS_CC xprcallback(XPRSprob prob, void* vUserDat)
{
   if( gevTerminateGet((gevHandle_t)vUserDat) )
      XPRSinterrupt(prob, XPRS_STOP_CTRLC);
   return 0;
}
#endif

/** streambuf implementation that directs output to gev streams, taken from CP Optimizer link */
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

GamsOsi::~GamsOsi()
{
   delete osi;
   delete msghandler;
}

int GamsOsi::readyAPI(
   struct gmoRec*        gmo_                /**< GAMS modeling object */
)
{
   char buffer[1024];

   gmo = gmo_;
   assert(gmo != NULL);

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   // print GAMS audit line
   struct palRec* pal;
   if( !palCreate(&pal, buffer, sizeof(buffer)) )
   {
      gevLogStat(gev, buffer);
      return 1;
   }

#if PALAPIVERSION >= 3
   switch( solverid )
   {
      case CPLEX:
         palSetSystemName(pal, "OSI CPLEX");
         break;
      case GUROBI:
         palSetSystemName(pal, "OSI GUROBI");
         break;
      case MOSEK:
         palSetSystemName(pal, "OSI MOSEK");
         break;
      case XPRESS:
         palSetSystemName(pal, "OSI XPRESS");
         break;
      default:
         gevLogStat(gev, "Error: Do not know systemname for solver\n");
         return -1;
   }
   palGetAuditLine(pal, buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

#ifdef GAMS_BUILD
   GAMSinitLicensing(gmo, pal);
#endif

   // print Osi and solver version information
   gevLogStat(gev, "");
   switch (solverid)
   {
#ifdef GAMSLINKS_HAS_OSICPX
      case CPLEX:
         sprintf(buffer, "OsiCplex (Osi library " OSI_VERSION ", CPLEX library %.2f)\nOsi link written by T. Achterberg. Osi is part of COIN-OR.\n", CPX_VERSION/100.);
         break;
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
      case GUROBI:
         sprintf(buffer, "OsiGurobi (Osi library " OSI_VERSION ", GUROBI library %d.%d.%d)\nOsi link written by S. Vigerske. Osi is part of COIN-OR.\n", GRB_VERSION_MAJOR, GRB_VERSION_MINOR, GRB_VERSION_TECHNICAL);
         break;
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
      case MOSEK:
         sprintf(buffer, "OsiMosek (Osi library " OSI_VERSION ", MOSEK library %d.%d)\nwritten by Bo Jensen. Osi is part of COIN-OR.\n", MSK_VERSION_MAJOR, MSK_VERSION_MINOR);
         break;
#endif

#ifdef GAMSLINKS_HAS_OSIXPR
      case XPRESS:
         sprintf(buffer, "OsiXpress (Osi library " OSI_VERSION ", FICO Xpress-Optimizer library %d)\nOsi is part of COIN-OR.\n", XPVERSION);
         break;
#endif

      default:
         sprintf(buffer, "Osi library " OSI_VERSION ".\n");
   }
   gevLogStat(gev, buffer);

   // delete old osi, if any
   delete osi;
   osi = NULL;

   // initialize Osi and licenses for academic and commercial solvers
   try
   {
      switch( solverid )
      {
         case CPLEX:
         {
#ifdef GAMSLINKS_HAS_OSICPX
            OsiCpxSolverInterface* osicpx;
#ifdef GAMS_BUILD
            if( !GAMScheckCPLEXLicense(pal, false) )
            {
               gevLogStat(gev,"***");
               gevLogStat(gev,"*** LICENSE ERROR:");
               gevLogStat(gev,"*** See https://support.gams.com/doku.php?id=solver:gams_cplex-link_and_osicplex-link_licenses for OsiCplex licensing information.");
               gevLogStat(gev,"***");
               gmoSolveStatSet(gmo, gmoSolveStat_License);
               gmoModelStatSet(gmo, gmoModelStat_LicenseError);
               return 1;
            }
#endif
            osicpx = new OsiCpxSolverInterface;
            osi = osicpx;
#else
            gevLogStat(gev, "GamsOsi compiled without Osi/CPLEX interface.\n");
            return 1;
#endif
            break;
         }

         case GUROBI:
         {
#ifdef GAMSLINKS_HAS_OSIGRB
            GRBenv* grbenv = NULL;
#ifdef GAMS_BUILD
            int status;
            GUlicenseInit_t initType;

            /* Gurobi license setup */
            if( (status=gevgurobilice(gev, pal, NULL, NULL, (void**)&grbenv, NULL, 1, &initType)) )
            {
               if( GRB_ERROR_NO_LICENSE == status )
                  sprintf(buffer, "Failed to create Gurobi environment. Missing license.");
               else
                  sprintf(buffer, "Failed to create Gurobi environment (status=%d).", status);
               gevLogStat(gev, buffer);
               gmoSolveStatSet(gmo, gmoSolveStat_License);
               gmoModelStatSet(gmo, gmoModelStat_LicenseError);
               return 1;
            } 
            
            // disable Gurobi output here, so we don't get messages from model setup to stdout
            if( grbenv != NULL )
               GRBsetintparam(grbenv, GRB_INT_PAR_OUTPUTFLAG, 0);
#endif
            // this lets OsiGrb take over ownership of grbenv (if not NULL), so it will be freed when osi is deleted
            osi = new OsiGrbSolverInterface(grbenv);

            // disable Gurobi output here again, in case grbenv was NULL above
            OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
            assert(osigrb != NULL);
            grbenv = GRBgetenv(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL));
            if( grbenv != NULL )
               GRBsetintparam(grbenv, GRB_INT_PAR_OUTPUTFLAG, 0);
#else
            gevLogStat(gev, "GamsOsi compiled without Osi/Gurobi interface.\n");
            return 1;
#endif
            break;
         }

         case MOSEK:
         {
#ifdef GAMSLINKS_HAS_OSIMSK
#ifdef GAMS_BUILD
            MSKenv_t mskenv;
            MKlicenseInit_t initType;

            if( MSK_makeenv(&mskenv,NULL) )
            {
               gevLogStat(gev, "Failed to create Mosek environment.");
               gmoSolveStatSet(gmo, gmoSolveStat_License);
               gmoModelStatSet(gmo, gmoModelStat_LicenseError);
               return 1;
            }
            if( gevmoseklice(gev, pal, mskenv, gmoM(gmo), gmoN(gmo), 0, &initType) )
               gevLogStat(gev, "Trying to use Mosek standalone license.\n");

            osi = new OsiMskSolverInterface(mskenv);
#else
            osi = new OsiMskSolverInterface();
#endif
#else
            gevLogStat(gev, "GamsOsi compiled without Osi/MOSEK interface.\n");
            return 1;
#endif
            break;
         }

         case XPRESS:
         {
#ifdef GAMSLINKS_HAS_OSIXPR
            char msg[1024];
#ifdef GAMS_BUILD
            XPlicenseInit_t initType;
            int initRC;

            /* Xpress license initialization: calls XPRSinit() in a thread-safe way and passes in GAMS/Xpress license */
            if( gevxpressliceInitTS(gev, pal, gmoM(gmo), gmoN(gmo), gmoNLNZ(gmo), gmoNDisc(gmo), 0, &initType, &initRC, msg, sizeof(msg)) )
            {
               if( *msg != '\0' )
                  gevLogStat(gev, msg);
               gevLogStat(gev, "Trying to use Xpress standalone license.\n");
            }
#endif
            OsiXprSolverInterface* osixpr = new OsiXprSolverInterface(0,0);  /* also calls XPRSinit() */

#ifdef GAMS_BUILD
            /* Xpress license finalize: calls XPRSfree() in a thread-safe way, if it was called by gevxpressliceInitTS, which is indicated by initRC >= 0
             * However, not much should happen if the call to XPRSinit() was successful in OsiXprSolverInterface.
             * If it wasn't, then we terminate anyway (return 1 below).
             */
            if( initRC >= 0 )
               gevxpressliceFreeTS();
#endif
#if 0 // getNumInstances() is always 0 now
            if( !osixpr->getNumInstances() )
            {
               gevLogStat(gev, "Failed to setup XPRESS instance. Maybe you do not have a license?\n");
               gmoSolveStatSet(gmo, gmoSolveStat_License);
               gmoModelStatSet(gmo, gmoModelStat_LicenseError);
               return 1;
            }
#endif
            osi = osixpr;

            XPRSgetbanner(msg);
            gevLog(gev, msg);
#else
            gevLogStat(gev, "GamsOsi compiled without Osi/XPRESS interface.\n");
            return 1;
#endif
            break;
         }

         default:
            gevLogStat(gev, "Unsupported solver id\n");
            return 1;
      }
   }
   catch( CoinError& error )
   {
      gevLogStatPChar(gev, "Exception caught when creating Osi interface: ");
      gevLogStat(gev, error.message().c_str());
      if( solverid == CPLEX || solverid == GUROBI || solverid == MOSEK || solverid == XPRESS )
      {
         palFree(&pal);
         gmoSolveStatSet(gmo, gmoSolveStat_License);
         gmoModelStatSet(gmo, gmoModelStat_LicenseError);
      }
      else
      {
         gmoSolveStatSet(gmo, gmoSolveStat_SetupErr);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      }
      return 1;
   }
   catch (...)
   {
      gevLogStat(gev, "Unknown exception caught when creating Osi interface\n");
      return 1;
   }
   palFree(&pal);

   // setup message handler
   if( msghandler == NULL )
   {
      msghandler = new GamsMessageHandler(gev);
      msghandler->setPrefix(false);
   }
   osi->passInMessageHandler(msghandler);
   osi->setHintParam(OsiDoReducePrint, true,  OsiHintTry);

   /* for gurobi we print messages via our own callback, which means to disable its usual output
      also if output is disabled in general, we can set the loglevel to 0
    */
   if( solverid == GUROBI || gevGetIntOpt(gev, gevLogOption) == 0 )
      msghandler->setLogLevel(0);

   return 0;
}

int GamsOsi::callSolver()
{
   assert(gev != NULL);
   assert(gmo != NULL);
   assert(osi != NULL);
   assert(msghandler != NULL);

   if( !setupProblem() )
   {
      gevLogStat(gev, "Error setting up problem. Aborting...");
      return -1;
   }

   if( !setupCallbacks() )
   {
      gevLogStat(gev, "Error setting up callbacks. Aborting...");
      return -1;
   }

   if( gmoN(gmo) > 0 && !setupStartingPoint() )
   {
      gevLogStat(gev, "Error setting up starting point. Continue...");
   }

   if( !setupParameters() )
   {
      gevLogStat(gev, "Error setting up OSI parameters. Aborting...");
      return -1;
   }

   gamsOsiWriteProblem(gmo, *osi, gevGetIntOpt(gev, gevInteger3));

   double start_cputime  = CoinCpuTime();
   double start_walltime = CoinWallclockTime();
   bool cpxsubproberror = false;
   bool failure = false;

   try
   {
      if( isLP() )
         osi->initialSolve();
      else
         osi->branchAndBound();
   }
   catch( CoinError& error )
   {
      // if cplex terminates because a subproblem could not be solved (error 3019),
      // then we want writeSolution to draw its conclusions from the subproblem status
      if( error.message() == "CPXmipopt returned error 3019" )
      {
         cpxsubproberror = true;
      }
      else
      {
         gevLogStatPChar(gev, "Exception caught when solving problem: ");
         gevLogStat(gev, error.message().c_str());
         failure = true;
      }

      gmoSolveStatSet(gmo, gmoSolveStat_Solver);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
   }

   double end_cputime  = CoinCpuTime();
   double end_walltime = CoinWallclockTime();

   if( !failure )
   {
      gevLogStat(gev, "");
      bool solwritten;
      writeSolution(end_cputime - start_cputime, end_walltime - start_walltime, cpxsubproberror, solwritten);

      if( !isLP() && gevGetIntOpt(gev, gevInteger1) && solwritten )
         solveFixed();
   }

   clearCallbacks();

   return failure ? 1 : 0;
}

bool GamsOsi::setupProblem()
{
   assert(gmo != NULL);
   assert(gev != NULL);
   assert(osi != NULL);

   gmoPinfSet(gmo,  osi->getInfinity());
   gmoMinfSet(gmo, -osi->getInfinity());
   gmoObjReformSet(gmo, 1);
   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoIndexBaseSet(gmo, 0);

   if( gmoGetVarTypeCnt(gmo, gmovar_SC) || gmoGetVarTypeCnt(gmo, gmovar_SI) || gmoGetVarTypeCnt(gmo, gmovar_S1) || gmoGetVarTypeCnt(gmo, gmovar_S2) )
   {
      gevLogStat(gev, "SOS, semicontinuous, and semiinteger variables not supported by OSI.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return false;
   }

   // get rid of free rows
   if( gmoGetEquTypeCnt(gmo, gmoequ_N) )
      gmoSetNRowPerm(gmo);

   if( !gamsOsiLoadProblem(gmo, *osi, gevGetIntOpt(gev, gevInteger2)) )
      return false;

   return true;
}

bool GamsOsi::setupStartingPoint()
{
   if( gmoHaveBasis(gmo) )
   {
      CoinWarmStartBasis basis;
      basis.setSize(gmoN(gmo), gmoM(gmo));

      int nbas = 0;
      for( int j = 0; j < gmoN(gmo); ++j )
      {
         switch( gmoGetVarStatOne(gmo, j) )
         {
            case gmoBstat_Basic:
               if( nbas < gmoM(gmo) )
               {
                  basis.setStructStatus(j, CoinWarmStartBasis::basic);
                  ++nbas;
               }
               else if( gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) )
                  basis.setStructStatus(j, CoinWarmStartBasis::isFree);
               else if( gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) || gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j) < gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j) )
                  basis.setStructStatus(j, CoinWarmStartBasis::atLowerBound);
               else
                  basis.setStructStatus(j, CoinWarmStartBasis::atUpperBound);
               break;
            case gmoBstat_Lower:
               if( gmoGetVarLowerOne(gmo, j) > gmoMinf(gmo) )
                  basis.setStructStatus(j, CoinWarmStartBasis::atLowerBound);
               else
                  basis.setStructStatus(j, CoinWarmStartBasis::isFree);
               break;
            case gmoBstat_Upper:
               if( gmoGetVarUpperOne(gmo, j) < gmoPinf(gmo) )
                  basis.setStructStatus(j, CoinWarmStartBasis::atUpperBound);
               else
                  basis.setStructStatus(j, CoinWarmStartBasis::isFree);
               break;
            case gmoBstat_Super:
               basis.setStructStatus(j, CoinWarmStartBasis::isFree);
               break;
            default:
               gevLogStat(gev, "Error: invalid basis indicator for column.");
               return false;
         }
      }

      for( int j = 0; j< gmoM(gmo); ++j )
      {
         switch( gmoGetEquStatOne(gmo, j) )
         {
            case gmoBstat_Basic:
               if( nbas < gmoM(gmo) )
               {
                  basis.setArtifStatus(j, CoinWarmStartBasis::basic);
                  ++nbas;
               }
               else
                  basis.setArtifStatus(j, gmoGetEquTypeOne(gmo, j) == gmoequ_G ? CoinWarmStartBasis::atUpperBound : CoinWarmStartBasis::atLowerBound);
               break;
            case gmoBstat_Lower:
               basis.setArtifStatus(j, CoinWarmStartBasis::atUpperBound);
               break;
            case gmoBstat_Upper:
               basis.setArtifStatus(j, CoinWarmStartBasis::atLowerBound);
               break;
            case gmoBstat_Super:
               basis.setArtifStatus(j, CoinWarmStartBasis::isFree);
               break;
            default:
               gevLogStat(gev, "Error: invalid basis indicator for row.");
               return false;
         }
      }

      try
      {
         // some solvers choke if they get an incomplete basis
         if( (solverid != GUROBI && solverid != MOSEK) || nbas == gmoM(gmo) )
         {
            if( !osi->setWarmStart(&basis) )
            {
               gevLogStat(gev, "Failed to set initial basis.");
               gmoHaveBasisSet(gmo, 0);
               return false;
            }
            else if( solverid == GUROBI )
            {
               gevLog(gev, "Registered advanced basis. This turns off presolve!");
               gevLog(gev, "In case of poor performance consider turning off advanced basis registration via GAMS option BRatio=1.");
            }
            else
            {
               gevLog(gev, "Registered advanced basis.");
            }
         }
         else
         {
            gevLog(gev, "Did not attempt to register incomplete basis.\n");
            gmoHaveBasisSet(gmo, 0);
         }
      }
      catch( CoinError& error )
      {
         gevLogStatPChar(gev, "Exception caught when setting initial basis: ");
         gevLogStat(gev, error.message().c_str());
         gmoHaveBasisSet(gmo, 0);
         return false;
      }
   }

   /* pass column solution for mipstart */
   if( !isLP() && (solverid == CPLEX || solverid == GUROBI || solverid == XPRESS) && gevGetIntOpt(gev, gevInteger4) )
   {
      double* varlevel = new double[gmoN(gmo)];
      gmoGetVarL(gmo, varlevel);
      osi->setColSolution(varlevel);
      delete[] varlevel;
   }

   return true;
}

bool GamsOsi::setupParameters()
{
   int    iterlim = gevGetIntOpt(gev, gevIterLim);
   double reslim  = gevGetDblOpt(gev, gevResLim);
   int    nodelim = gevGetIntOpt(gev, gevNodeLim);
   double optcr   = gevGetDblOpt(gev, gevOptCR);
   double optca   = gevGetDblOpt(gev, gevOptCA);

   osi->setIntParam(OsiMaxNumIteration, iterlim);
   
   /* default is to try doing dual in initial, but a user might want to
    * overwrite this setting with its solver specific options file;
    * so we change the OsiDoDualInInitial hint from OsiHintTry to OsiHintIgnore,
    * so Osi*::initialSolve should ignore this hint and let the solver choose
    */
   osi->setHintParam(OsiDoDualInInitial,1,OsiHintIgnore);

   switch( solverid )
   {
#ifdef GAMSLINKS_HAS_OSICPX
      case CPLEX:
      {
         OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
         assert(osicpx != NULL);
         CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_TILIM, reslim);
         if( !isLP() && nodelim > 0 )
            CPXsetintparam(osicpx->getEnvironmentPtr(), CPX_PARAM_NODELIM, nodelim);
         CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_EPGAP, optcr);
         CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_EPAGAP, optca);
         CPXsetintparam(osicpx->getEnvironmentPtr(), CPX_PARAM_THREADS, gevThreads(gev));
         osicpx->setMipStart(gevGetIntOpt(gev, gevInteger4));

         if( gmoOptFile(gmo) > 0 )
         {
            char buffer[GMS_SSSIZE];
            gmoNameOptFile(gmo, buffer);
            gevLogStatPChar(gev, "Let CPLEX read option file "); gevLogStat(gev, buffer);
            int ret = CPXreadcopyparam(osicpx->getEnvironmentPtr(), buffer);
            if( ret )
            {
               const char* errstr = CPXgeterrorstring(osicpx->getEnvironmentPtr(), ret, buffer);
               gevLogStatPChar(gev, "Reading option file failed: ");
               if( errstr )
                  gevLogStat(gev, errstr);
               else
                  gevLogStat(gev, "unknown error code");
            }
         }

         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
      case GUROBI:
      {
         OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
         assert(osigrb != NULL);
         GRBenv* grbenv = GRBgetenv(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL));
         GRBsetdblparam(grbenv, GRB_DBL_PAR_TIMELIMIT, reslim);
         if( !isLP() && nodelim > 0 )
            GRBsetdblparam(grbenv, GRB_DBL_PAR_NODELIMIT, (double)nodelim);
         GRBsetdblparam(grbenv, GRB_DBL_PAR_MIPGAP, optcr);
         GRBsetdblparam(grbenv, GRB_DBL_PAR_MIPGAPABS, optca);
         GRBsetintparam(grbenv, GRB_INT_PAR_THREADS, gevThreads(gev));
         osigrb->setMipStart(gevGetIntOpt(gev, gevInteger4));

         if( gmoOptFile(gmo) > 0 )
         {
            char buffer[GMS_SSSIZE];
            gmoNameOptFile(gmo, buffer);
            gevLogStatPChar(gev, "Let GUROBI read option file "); gevLogStat(gev, buffer);
            int ret = GRBreadparams(grbenv, buffer);
            if( ret )
            {
               const char* errstr = GRBgeterrormsg(grbenv);
               gevLogStatPChar(gev, "Reading option file failed: ");
               if (errstr)
                  gevLogStat(gev, errstr);
               else
                  gevLogStat(gev, "unknown error");
            }
         }

         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
      case MOSEK:
      {
         OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
         assert(osimsk != NULL);
         MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_OPTIMIZER_MAX_TIME, reslim);
         if( !isLP() && nodelim > 0 )
            MSK_putintparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_IPAR_MIO_MAX_NUM_RELAXS, nodelim);
         MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_MIO_TOL_REL_GAP, optcr);
         MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_MIO_TOL_ABS_GAP, optca);
         MSK_putintparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_IPAR_NUM_THREADS, gevThreads(gev));

         if( gmoOptFile(gmo) > 0 )
         {
            char buffer[GMS_SSSIZE];
            gmoNameOptFile(gmo, buffer);
            gevLogStatPChar(gev, "Let MOSEK read option file "); gevLogStat(gev, buffer);
            MSK_readparamfile(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), buffer);
         }

         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIXPR
      case XPRESS:
      {
         OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
         assert(osixpr != NULL);

         /* need to set XPRS_MAXTIME to a negative number to have it working also for LP solves and before the first solution is found in MIP solves */
         XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_MAXTIME, -(int)reslim);
         if( !isLP() && nodelim > 0 )
            XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_MAXNODE, nodelim);
         XPRSsetdblcontrol(osixpr->getLpPtr(), XPRS_MIPRELSTOP, optcr);
         XPRSsetdblcontrol(osixpr->getLpPtr(), XPRS_MIPABSSTOP, optca);
         /* reduce relative mip cutoff applied by xpress, if optcr is below its default */
         if( optcr < 1e-4 ) // 1e-4 is the default for XPRS_MIPRELCUTOFF in xpress 31.01
            XPRSsetdblcontrol(osixpr->getLpPtr(), XPRS_MIPRELCUTOFF, optcr);
         XPRSsetdblcontrol(osixpr->getLpPtr(), XPRS_MIPADDCUTOFF, -gevGetDblOpt(gev, gevCheat));
         XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_THREADS, gevThreads(gev));
         osixpr->setMipStart(gevGetIntOpt(gev, gevInteger4));

         if( gmoOptFile(gmo) > 0 )
         {
            char buffer[GMS_SSSIZE];
            gmoNameOptFile(gmo, buffer);
            gevLogStatPChar(gev, "Let XPRESS process option file "); gevLogStat(gev, buffer);
            std::ifstream optfile(buffer);
            if( !optfile.good() )
               gevLogStat(gev, "Failed reading option file.");
            else
               do
               {
                  optfile.getline(buffer, 4096);
                  XPRScommand(osixpr->getLpPtr(), buffer);
               }
               while( optfile.good() );
         }

         break;
      }
#endif

      default:
         gevLogStat(gev, "Encountered unsupported solver id in setupParameters.");
         return false;
   }

   return true;
}

bool GamsOsi::setupCallbacks()
{
   assert(gev != NULL);
   assert(osi != NULL);

   switch( solverid )
   {
#ifdef GAMSLINKS_HAS_OSICPX
      case CPLEX:
      {
         OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
         assert(osicpx != NULL);
         CPXsetinfocallbackfunc(osicpx->getEnvironmentPtr(), cpxinfocallback, (void*)gev);
         gevTerminateInstall(gev);
         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
      case GUROBI:
      {
         OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
         assert(osigrb != NULL);
         if( GRBsetcallbackfunc(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), grbcallback, (void*)gev) )
            gevLogStat(gev, GRBgeterrormsg(osigrb->getEnvironmentPtr()));
         gevTerminateInstall(gev);
         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
      case MOSEK:
      {
         OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
         assert(osimsk != NULL);
         MSK_putcallbackfunc(osimsk->getLpPtr(), mskcallback, (void*)gev);
         gevTerminateInstall(gev);
         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIXPR
      case XPRESS:
      {
         OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
         assert(osixpr != NULL);
         XPRSsetcbgloballog(osixpr->getLpPtr(), xprcallback, (void*)gev);
         XPRSsetcbcutlog(osixpr->getLpPtr(), xprcallback, (void*)gev);
         XPRSsetcblplog(osixpr->getLpPtr(), xprcallback, (void*)gev);
         gevTerminateInstall(gev);
         break;
      }
#endif

      default: ;
   }

   return true;
}

bool GamsOsi::clearCallbacks()
{
#if 0
   switch( solverid )
   {

      default: ;
   }
#endif

   return true;
}

/** writes solution from OSI into GMO */
bool GamsOsi::writeSolution(
   double             cputime,            /**< CPU time spend by solver */
   double             walltime,           /**< wallclock time spend by solver */
   bool               cpxsubproberr,      /**< has an error in solving a CPLEX subproblem occured? */
   bool&              solwritten          /**< whether a solution has been passed to GMO */
)
{
   assert(gmo != NULL);
   assert(gev != NULL);
   assert(osi != NULL);

   double objest = GMS_SV_NA;
   int nnodes = 0;

   solwritten = false;

   switch( solverid )
   {
#ifdef GAMSLINKS_HAS_OSICPX
      case CPLEX:
      {
         OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
         assert(osicpx != NULL);
         if( !isLP() )
         {
            CPXgetbestobjval(osicpx->getEnvironmentPtr(), osicpx->getLpPtr(OsiCpxSolverInterface::KEEPCACHED_ALL), &objest);
            objest += gmoObjConst(gmo);
            nnodes = CPXgetnodecnt(osicpx->getEnvironmentPtr(), osicpx->getLpPtr(OsiCpxSolverInterface::KEEPCACHED_ALL));
         }
         int stat;
         if( cpxsubproberr )
            stat = CPXgetsubstat( osicpx->getEnvironmentPtr(), osicpx->getLpPtr(OsiCpxSolverInterface::KEEPCACHED_ALL) );
         else
            stat = CPXgetstat( osicpx->getEnvironmentPtr(), osicpx->getLpPtr(OsiCpxSolverInterface::KEEPCACHED_ALL) );
         switch( stat )
         {
            case CPX_STAT_OPTIMAL:
            case CPX_STAT_OPTIMAL_INFEAS:
            case CPXMIP_OPTIMAL:
            case CPXMIP_OPTIMAL_INFEAS:
               solwritten = true;
               objest = osicpx->getObjValue();
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
               if( stat == CPX_STAT_OPTIMAL_INFEAS || stat == CPXMIP_OPTIMAL_INFEAS )
                  gevLogStat(gev, "Solved to optimality, but solution has infeasibilities after unscaling.");
               else
                  gevLogStat(gev, "Solved to optimality.");
               break;

            case CPX_STAT_UNBOUNDED:
            case CPXMIP_UNBOUNDED:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_Unbounded);
               gevLogStat(gev, "Model unbounded.");
               break;

            case CPX_STAT_INFEASIBLE:
            case CPX_STAT_INForUNBD:
            case CPXMIP_INFEASIBLE:
            case CPXMIP_INForUNBD:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleGlobal);
               gevLogStat(gev, "Model infeasible.");
               break;

            case CPX_STAT_NUM_BEST:
            case CPX_STAT_FEASIBLE:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_Feasible);
               gevLogStat(gev, "Feasible solution found.");
               break;

            case CPX_STAT_ABORT_IT_LIM:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Iteration limit reached.");
               break;

            case CPX_STAT_ABORT_TIME_LIM:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Time limit reached.");
               break;

            case CPX_STAT_ABORT_OBJ_LIM:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Objective limit reached.");
               break;

            case CPX_STAT_ABORT_USER:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Stopped on user interrupt.");
               break;

            case CPXMIP_OPTIMAL_TOL:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Solved to optimality within gap tolerances.");
               break;

            case CPXMIP_SOL_LIM:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Solution limit reached.");
               break;

            case CPXMIP_NODE_LIM_FEAS:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Node limit reached, have feasible solution.");
               break;

            case CPXMIP_NODE_LIM_INFEAS:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Node limit reached, do not have feasible solution.");
               break;

            case CPXMIP_TIME_LIM_FEAS:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Time limit reached, have feasible solution.");
               break;

            case CPXMIP_TIME_LIM_INFEAS:
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Time limit reached, do not have feasible solution.");
               break;

            case CPXMIP_ABORT_FEAS:
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Solving interrupted, but have feasible solution.");
               break;

            case CPXMIP_ABORT_INFEAS:
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Solving interrupted, do not have feasible solution.");
               break;

            case CPXMIP_FAIL_FEAS:
            case CPXMIP_FAIL_FEAS_NO_TREE:
            case CPXMIP_MEM_LIM_FEAS:
            case CPXMIP_FEASIBLE:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Solving failed, but have feasible solution.");
               break;

            case CPXMIP_FAIL_INFEAS:
            case CPXMIP_FAIL_INFEAS_NO_TREE:
            case CPXMIP_MEM_LIM_INFEAS:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Solving failed, do not have feasible solution.");
               break;
         }
         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
      case GUROBI:
      {
         OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
         assert(osigrb != NULL);
         int stat, nrsol;
         GRBgetintattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_INT_ATTR_SOLCOUNT, &nrsol);
         GRBgetintattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_INT_ATTR_STATUS, &stat);
         solwritten = nrsol;
         if( !isLP() )
         {
            double nodecount;
            GRBgetdblattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_DBL_ATTR_OBJBOUND, &objest);
            objest += gmoObjConst(gmo);
            GRBgetdblattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_DBL_ATTR_NODECOUNT, &nodecount);
            nnodes = (int)nodecount;
         }
         switch( stat )
         {
            case GRB_OPTIMAL:
               assert(nrsol);
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               if( !isLP() )
               {
                  gmoSetHeadnTail(gmo, gmoTmipbest, objest);
                  gmoSetHeadnTail(gmo, gmoHobjval, osi->getObjValue());
               }
               if( isLP() || gmoGetRelativeGap(gmo) < 1e-9 )
               {
                  gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
                  gevLogStat(gev, "Solved to optimality.");
               }
               else
               {
                  gevLogStat(gev, "Solved to optimality within tolerances.");
                  gmoModelStatSet(gmo, gmoModelStat_Integer);
               }
               break;

            case GRB_INFEASIBLE:
               assert(!nrsol);

            case GRB_INF_OR_UNBD:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleGlobal);
               gevLogStat(gev, "Model infeasible.");
               break;

            case GRB_UNBOUNDED:
               /* TODO OsiGrb does not seem to report a ray when proven a problem to be unbounded */
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_UnboundedNoSolution);
               gevLogStat(gev, "Model unbounded.");
               break;

            case GRB_CUTOFF:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Objective limit reached.");
               break;

            case GRB_ITERATION_LIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               if( nrsol )
               {
                  gmoModelStatSet(gmo, isLP() ? gmoModelStat_Feasible : gmoModelStat_Integer);
                  gevLogStat(gev, "Iteration limit reached, but have feasible solution.");
               }
               else
               {
                  /* OsiGrb does not report any solution when the iterlim is hit */
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Iteration limit reached.");
               }
               break;

            case GRB_NODE_LIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               assert(!isLP());
               if( nrsol )
               {
                  gmoModelStatSet(gmo, gmoModelStat_Integer);
                  gevLogStat(gev, "Node limit reached, but have feasible solution.");
               }
               else
               {
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Node limit reached.");
               }
               break;

            case GRB_TIME_LIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               if( nrsol )
               {
                  gmoModelStatSet(gmo, isLP() ? gmoModelStat_Feasible : gmoModelStat_Integer);
                  gevLogStat(gev, "Time limit reached, but have feasible solution.");
               }
               else
               {
                  /* OsiGrb does not report any solution when the time limit is hit */
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Time limit reached.");
               }
               break;

            case GRB_SOLUTION_LIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               if( nrsol )
               {
                  gmoModelStatSet(gmo, isLP() ? gmoModelStat_Feasible : gmoModelStat_Integer);
                  gevLogStat(gev, "Solution limit reached.");
               }
               else
               {
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Solution limit reached, but do not have solution.");
               }
               break;

            case GRB_INTERRUPTED:
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               if( nrsol )
               {
                  gmoModelStatSet(gmo, isLP() ? gmoModelStat_Feasible : gmoModelStat_Integer);
                  gevLogStat(gev, "User interrupt, have feasible solution.");
               }
               else
               {
                  /* OsiGrb does not report any solution when interrupted */
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "User interrupt.");
               }
               break;

            case GRB_NUMERIC:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               if( nrsol )
               {
                  gmoModelStatSet(gmo, isLP() ? gmoModelStat_Feasible : gmoModelStat_Integer);
                  gevLogStat(gev, "Stopped on numerical difficulties, but have feasible solution.");
               }
               else
               {
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Stopped on numerical difficulties.");
               }
               break;

            case GRB_SUBOPTIMAL:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               if( nrsol )
               {
                  gmoModelStatSet(gmo, isLP() ? gmoModelStat_Feasible : gmoModelStat_Integer);
                  gevLogStat(gev, "Stopped on suboptimal but feasible solution.");
               }
               else
               {
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Stopped at suboptimal point.");
               }
               break;

            case GRB_LOADED:
            default:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Solving failed, unknown status.");
               break;
         }
         break;
      }
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
      case MOSEK:
      {
         OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
         assert(osimsk);

         if( osimsk->isLicenseError() )
         {
            gmoSolveStatSet(gmo, gmoSolveStat_License);
            gmoModelStatSet(gmo, gmoModelStat_LicenseError);
            gevLogStat(gev, "Stopped with license error.");
            break;
         }

         MSKprostae probstatus;
         MSKsolstae solstatus;
         MSKsoltypee solution;

         int res;
         if( isLP() )
         {
            solution = MSK_SOL_BAS;
            MSK_solutiondef(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &res);
            if( res == MSK_RES_OK )
            {
               solution = MSK_SOL_ITR;
               MSK_solutiondef(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &res);
               if( res == MSK_RES_OK )
               {
                  gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
                  gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
                  gevLogStat(gev, "Failure retrieving solution status.");
                  break;
               }
            }
         }
         else
         {
            solution = MSK_SOL_ITG;
            MSK_solutiondef(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &res);
            if( res == MSK_RES_OK )
            {
               gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
               gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
               gevLogStat(gev, "Failure retrieving solution status.");
               break;
            }
            int nrelax;
            MSK_getintinf(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_IINF_MIO_NUM_RELAX, &nrelax);
            if( nrelax > 0 )
            {
               MSK_getdouinf(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DINF_MIO_OBJ_BOUND, &objest);
               objest += gmoObjConst(gmo);
            }
         }

         MSK_getsolution(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &probstatus, &solstatus,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

         switch( solstatus )
         {
            case MSK_SOL_STA_PRIM_FEAS:
            case MSK_SOL_STA_PRIM_AND_DUAL_FEAS:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               if( isLP() )
                  gmoModelStatSet(gmo, gmoModelStat_Feasible);
               else
                  gmoModelStatSet(gmo, gmoModelStat_Integer);
               gevLogStat(gev, "Solved to feasibility.");
               break;

            case MSK_SOL_STA_INTEGER_OPTIMAL:
               objest = osimsk->getObjValue(); /* in case instance was solved in presolve, i.e., without solving a relaxation */
            case MSK_SOL_STA_OPTIMAL:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
               gevLogStat(gev, "Solved to optimality.");
               break;

            case MSK_SOL_STA_PRIM_INFEAS_CER:
               // TODO OsiMsk does not seem to report an infeasible solution
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
               gevLogStat(gev, "Model is infeasible.");
               break;

            case MSK_SOL_STA_DUAL_INFEAS_CER:
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_Unbounded);
               gevLogStat(gev, "Model is unbounded.");
               break;

            case MSK_SOL_STA_DUAL_FEAS:
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Stopped before feasibility or infeasibility proven.");
               break;

            case MSK_SOL_STA_UNKNOWN:
            default:
               switch( probstatus )
               {
                  case MSK_PRO_STA_DUAL_FEAS:
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                     gevLogStat(gev, "Model is dual feasible.");
                     break;

                  case MSK_PRO_STA_DUAL_INFEAS:
                     solwritten = true;
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_Unbounded);
                     gevLogStat(gev, "Model is dual infeasible (probably unbounded).");
                     break;

                  case MSK_PRO_STA_ILL_POSED:
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                     gevLogStat(gev, "Model is ill posed.");
                     break;

                  case MSK_PRO_STA_PRIM_AND_DUAL_FEAS:
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_Feasible);
                     gevLogStat(gev, "Model is primal and dual feasible.");
                     break;

                  case MSK_PRO_STA_PRIM_AND_DUAL_INFEAS:
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
                     gevLogStat(gev, "Model is primal and dual infeasible.");
                     break;

                  case MSK_PRO_STA_PRIM_FEAS:
                     solwritten = true;
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_Feasible);
                     gevLogStat(gev, "Model is primal feasible.");
                     break;

                  case MSK_PRO_STA_PRIM_INFEAS:
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
                     gevLogStat(gev, "Model is primal infeasible.");
                     break;

                  case MSK_PRO_STA_PRIM_INFEAS_OR_UNBOUNDED:
                     gmoSolveStatSet(gmo, gmoSolveStat_Normal);
                     gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                     gevLogStat(gev, "Model is infeasible or unbounded.");
                     break;

                  case MSK_PRO_STA_UNKNOWN:
                  default:
                     gmoSolveStatSet(gmo, gmoSolveStat_Solver);
                     gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                     gevLogStat(gev, "Solve status unknown.");
                     break;
               }
         }

         switch( osimsk->getRescode() )
         {
            case MSK_RES_TRM_MAX_ITERATIONS:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gevLogStat(gev, "Stopped due to iterations limit.");
               break;

            case MSK_RES_TRM_MIO_NUM_RELAXS:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gevLogStat(gev, "Stopped due to node limit.");
               break;

            case MSK_RES_TRM_MAX_TIME:
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               gevLogStat(gev, "Stopped due to time limit.");
               break;

            case MSK_RES_TRM_USER_CALLBACK:
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               gevLogStat(gev, "Stopped due to user interrupt.");
               break;
            /*
            MSK_RES_TRM_OBJECTIVE_RANGE                    = 4002,
            MSK_RES_TRM_MIO_NEAR_REL_GAP                   = 4003,
            MSK_RES_TRM_MIO_NEAR_ABS_GAP                   = 4004,
            MSK_RES_TRM_STALL                              = 4006,
            MSK_RES_TRM_MIO_NUM_RELAXS                     = 4008,
            MSK_RES_TRM_MIO_NUM_BRANCHES                   = 4009,
            MSK_RES_TRM_NUM_MAX_NUM_INT_SOLUTIONS          = 4015,
            MSK_RES_TRM_MAX_NUM_SETBACKS                   = 4020,
            MSK_RES_TRM_NUMERICAL_PROBLEM                  = 4025,
            MSK_RES_TRM_INTERNAL                           = 4030,
            MSK_RES_TRM_INTERNAL_STOP                      = 4031
            */
         }

         break;
      }
#endif


#ifdef GAMSLINKS_HAS_OSIXPR
      case XPRESS:
      {
         OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
         assert(osixpr);
         int status;

         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         if( isLP() )
         {
            XPRSgetintattrib(osixpr->getLpPtr(), XPRS_LPSTATUS, &status);

            switch( status )
            {
               case XPRS_LP_OPTIMAL:
                  solwritten = true;
                  gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
                  gevLogStat(gev, "Solved to optimality.");
                  break;

               case XPRS_LP_INFEAS:
                  // TODO OsiXpr does not seem to report an infeasible solution
                  gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
                  gevLogStat(gev, "Model is infeasible.");
                  break;

               case XPRS_LP_CUTOFF:
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Objective limit reached.");
                  break;

               case XPRS_LP_UNBOUNDED:
                  // TODO OsiXpr does not implement getPrimalRay (throws CoinError)
                  gmoModelStatSet(gmo, gmoModelStat_UnboundedNoSolution);
                  gevLogStat(gev, "Model is unbounded.");
                  break;

               case XPRS_LP_CUTOFF_IN_DUAL:
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Dual objective limit reached.");
                  break;

               case XPRS_LP_UNFINISHED:
               case XPRS_LP_UNSOLVED:
               case XPRS_LP_NONCONVEX:
                  // Xpress does not seem to report an intermediate solution
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gmoSolveStatSet(gmo, gmoSolveStat_Iteration); /* we just guess that it was the iteration limit */
                  gevLogStat(gev, "LP solve not finished.");
                  break;

               default:
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Solution status unknown.");
                  break;
            }
         }
         else
         {
            XPRSgetintattrib(osixpr->getLpPtr(), XPRS_MIPSTATUS, &status);
            XPRSgetdblattrib(osixpr->getLpPtr(), XPRS_BESTBOUND, &objest);
            XPRSgetintattrib(osixpr->getLpPtr(), XPRS_NODES,     &nnodes);
            objest += gmoObjConst(gmo);
            switch( status )
            {
               case XPRS_MIP_LP_NOT_OPTIMAL:
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "LP relaxation not solved to optimality.");
                  break;

               case XPRS_MIP_LP_OPTIMAL:
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Only LP relaxation solved.");
                  break;

               case XPRS_MIP_NO_SOL_FOUND:
                  gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
                  gevLogStat(gev, "No solution found.");
                  break;

               case XPRS_MIP_SOLUTION:
                  solwritten = true;
                  gmoModelStatSet(gmo, gmoModelStat_Integer);
                  gevLogStat(gev, "Found integer feasible solution.");
                  break;

               case XPRS_MIP_INFEAS:
                  gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
                  gevLogStat(gev, "Model is infeasible.");
                  break;

               case XPRS_MIP_OPTIMAL:
                  solwritten = true;
                  gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
                  if( fabs(objest - osixpr->getObjValue()) < 1e-9 )
                     gevLogStat(gev, "Solved to optimality.");
                  else
                     gevLogStat(gev, "Solved to optimality. Warning: Some gap remains.");
                  break;
            }
         }

         XPRSgetintattrib(osixpr->getLpPtr(), XPRS_STOPSTATUS, &status);
         switch( status )
         {
            case 0:
               break;

            case XPRS_STOP_TIMELIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Resource);
               gevLogStat(gev, "Timelimit reached.");
               break;

            case XPRS_STOP_CTRLC:
            case XPRS_STOP_USER:
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               gevLogStat(gev, "User interrupted.");
               break;

            case XPRS_STOP_NODELIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gevLogStat(gev, "Nodelimit reached.");
               break;

            case XPRS_STOP_ITERLIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gevLogStat(gev, "Iterationlimit reached.");
               break;

            case XPRS_STOP_MIPGAP:
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               break;

            case XPRS_STOP_SOLLIMIT:
               gmoSolveStatSet(gmo, gmoSolveStat_User);
               gevLogStat(gev, "Solutionlimit reached.");
               break;

            default:
               gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
               gevLogStat(gev, "Unknown stop status.");
               break;
         }

         break;
      }
#endif

      default:
      {
         try
         {
            gevLogStat(gev, "");
            if( osi->isProvenDualInfeasible() )
            {
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_UnboundedNoSolution);
               gevLogStat(gev, "Model unbounded.");
            }
            else if( osi->isProvenPrimalInfeasible() )
            {
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
               gevLogStat(gev, "Model infeasible.");
            }
            else if( osi->isAbandoned() )
            {
               gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
               gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
               gevLogStat(gev, "Model abandoned.");
            }
            else if( osi->isProvenOptimal() )
            {
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Normal);
               gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
               gevLogStat(gev, "Solved to optimality.");
            }
            else if( osi->isIterationLimitReached() )
            {
               solwritten = true;
               gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
               gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
               gevLogStat(gev, "Iteration limit reached.");
            }
            else if( osi->isPrimalObjectiveLimitReached() )
            {
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Primal objective limit reached.");
            }
            else if( osi->isDualObjectiveLimitReached() )
            {
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
               gevLogStat(gev, "Dual objective limit reached.");
            }
            else
            {
               gmoSolveStatSet(gmo, gmoSolveStat_Solver);
               gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
               gevLogStat(gev, "Model status unknown.");
            }
         }
         catch( CoinError& error )
         {
            gevLogStatPChar(gev, "Exception caught when requesting solution status: ");
            gevLogStat(gev, error.message().c_str());
            gmoSolveStatSet(gmo, gmoSolveStat_Solver);
            gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         }
      }
   }

   try
   {
      gmoSetHeadnTail(gmo, gmoHiterused, osi->getIterationCount());
   }
   catch( CoinError& error )
   {
      gevLogStatPChar(gev, "Exception caught when requesting iteration count: ");
      gevLogStat(gev, error.message().c_str());
   }
   gmoSetHeadnTail(gmo, gmoHresused, cputime);
   gmoSetHeadnTail(gmo, gmoTmipbest, objest);
   gmoSetHeadnTail(gmo, gmoTmipnod,  nnodes);

   if( solwritten )
   {
      if( isLP() )
      {
         if( !gamsOsiStoreSolution(gmo, *osi) )
            solwritten = false;
      }
      else
      {
         try
         {
            const double* colsol = osi->getColSolution();

            // is MIP -> store only primal values here, duals are not available (yet, computed below)
#if GMOAPIVERSION < 12
            double* rowprice = new double[gmoM(gmo)];
            for( int i = 0; i < gmoM(gmo); ++i )
               rowprice[i] = gmoValNA(gmo);

            /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
            gmoSetSolution2(gmo, colsol, rowprice);

            delete[] rowprice;
#else
            /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
            gmoSetSolutionPrimal(gmo, colsol);
#endif
         }
         catch( CoinError& error )
         {
            gevLogStatPChar(gev, "Exception caught when requesting primal solution values: ");
            gevLogStat(gev, error.message().c_str());
            solwritten = false;
         }
      }
   }

   if( !isLP() )
   {
      char buffer[255];
      if( solwritten )
      {
         if( solverid != MOSEK )
            snprintf(buffer, 255, "MIP solution: %15.6e   (%d nodes, %g seconds)", osi->getObjValue(), nnodes, cputime);
         else
            snprintf(buffer, 255, "MIP solution: %15.6e   (%g seconds)", osi->getObjValue(), cputime);
         gevLogStat(gev, buffer);
      }
      if( objest != GMS_SV_NA && objest > -osi->getInfinity() && objest < osi->getInfinity() )
      {
         snprintf(buffer, 255, "Best possible: %14.6e", objest);
         gevLogStat(gev, buffer);
         if( solwritten )
         {
            snprintf(buffer, 255, "Absolute gap: %15.6e   (absolute tolerance optca: %g)", gmoGetAbsoluteGap(gmo), gevGetDblOpt(gev, gevOptCA));
            gevLogStat(gev, buffer);
            snprintf(buffer, 255, "Relative gap: %15.6e   (relative tolerance optcr: %g)", gmoGetRelativeGap(gmo), gevGetDblOpt(gev, gevOptCR));
            gevLogStat(gev, buffer);
         }
      }
      
      /* Mosek may report only primal feasible for a MIP solve even when solved to completion with 0 gap tolerance due to numerical differences in computing the gap */
      if( solverid == MOSEK && gmoModelStat(gmo) == gmoModelStat_Integer && gmoGetRelativeGap(gmo) <= 1e-9 )
         gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
   }

   return true;
}

bool GamsOsi::solveFixed()
{
   gevLogStat(gev, "\nSolving LP obtained from MIP by fixing discrete variables to values in solution.\n");

   for( int i = 0; i < gmoN(gmo); ++i )
      if( gmoGetVarTypeOne(gmo, i) != gmovar_X )
      {
         double solval, dummy;
         int dummy2;
         gmoGetSolutionVarRec(gmo, i, &solval, &dummy, &dummy2, &dummy2);
         osi->setColBounds(i, solval, solval);
      }

   if( gevGetDblOpt(gev, gevReal1) > 0.0 )
   {
      // setup timelimit for fixed solve
      switch( solverid )
      {
#ifdef GAMSLINKS_HAS_OSICPX
         case CPLEX:
         {
            OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
            assert(osicpx != NULL);
            CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_TILIM, gevGetDblOpt(gev, gevReal1));
            break;
         }
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
         case GUROBI:
         {
            OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
            assert(osigrb != NULL);
            GRBsetdblparam(GRBgetenv(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL)), GRB_DBL_PAR_TIMELIMIT, gevGetDblOpt(gev, gevReal1));
            break;
         }
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
         case MOSEK:
         {
            OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
            assert(osimsk != NULL);
            MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_OPTIMIZER_MAX_TIME, gevGetDblOpt(gev, gevReal1));
            break;
         }
#endif

#ifdef GAMSLINKS_HAS_OSIXPR
         case XPRESS:
         {
            OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
            assert(osixpr != NULL);
            XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_MAXTIME, -(int)gevGetDblOpt(gev, gevReal1));
            break;
         }
#endif

         default:
            gevLogStat(gev, "Encountered unsupported solver id in solveFixed, cannot set timelimit.");
      }
   }

   osi->resolve();

   if( osi->isProvenOptimal() )
   {
      if( !gamsOsiStoreSolution(gmo, *osi) )
      {
         gevLogStat(gev, "Failed to store LP solution. Only primal solution values will be available in GAMS solution file.\n");
         return false;
      }
   }
   else
   {
      gevLogStat(gev, "Solve of final LP failed. Only primal solution values will be available in GAMS solution file.\n");
      gevSetIntOpt(gev, gevInteger1, 0);
   }

   // reset variable bounds
   for( int i = 0; i < gmoN(gmo); ++i )
      if( gmoGetVarTypeOne(gmo, i) != gmovar_X )
         osi->setColBounds(i, gmoGetVarLowerOne(gmo, i), gmoGetVarUpperOne(gmo, i));

   return true;
}

bool GamsOsi::isLP() {
   if( gmoModelType(gmo) == gmoProc_lp )
      return true;
   if( gmoModelType(gmo) == gmoProc_rmip )
      return true;
   if( gmoNDisc(gmo) )
      return false;
   return true;
}

static
int oxycreate(void** Cptr, char* msgBuf, int msgBufLen, GamsOsi::OSISOLVER osisolver)
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

   *Cptr = (void*) new GamsOsi(osisolver);
   if( *Cptr == NULL )
   {
      snprintf(msgBuf, msgBufLen, "Out of memory when creating GamsOsi object.\n");
      if( msgBufLen > 0 )
         msgBuf[msgBufLen] = '\0';
      return 0;
   }

   return 1;
}

static
int oxyfree(void** Cptr)
{
   assert(Cptr != NULL);

   delete (GamsOsi*)*Cptr;
   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();

   return 1;
}

static
void oxyInitialize(void)
{
#if defined(__linux) && defined(GAMSLINKS_HAS_OSICPX)
   CPXinitialize();
#endif

   gmoInitMutexes();
   gevInitMutexes();
   palInitMutexes();
}

static
void oxyFinalize(void)
{
#if defined(__linux) && defined(GAMSLINKS_HAS_OSICPX)
   CPXfinalize();
#endif

   gmoFiniMutexes();
   gevFiniMutexes();
   palFiniMutexes();
}

#ifdef GAMSLINKS_HAS_OSICPX
#define GAMSSOLVER_ID ocp
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
   oxyInitialize();
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
   oxyFinalize();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,create)(void** Cptr, char* msgBuf, int msgBufLen)
{
   return oxycreate(Cptr, msgBuf, msgBufLen, GamsOsi::CPLEX);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   return oxyfree(Cptr);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsOsi*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsOsi*)Cptr)->readyAPI(Gptr);
}

#undef GAMSSOLVER_ID
#endif

#ifdef GAMSLINKS_HAS_OSIGRB
#define GAMSSOLVER_ID ogu
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
   oxyInitialize();
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
   oxyFinalize();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,create)(void** Cptr, char* msgBuf, int msgBufLen)
{
   return oxycreate(Cptr, msgBuf, msgBufLen, GamsOsi::GUROBI);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   return oxyfree(Cptr);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsOsi*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsOsi*)Cptr)->readyAPI(Gptr);
}

#undef GAMSSOLVER_ID
#endif

#ifdef GAMSLINKS_HAS_OSIMSK
#define GAMSSOLVER_ID omk
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
   oxyInitialize();
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
   oxyFinalize();

#ifdef GAMSLINKS_HAS_OSIMSK
   MSK_licensecleanup();
#endif
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,create)(void** Cptr, char* msgBuf, int msgBufLen)
{
   return oxycreate(Cptr, msgBuf, msgBufLen, GamsOsi::MOSEK);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   return oxyfree(Cptr);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsOsi*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsOsi*)Cptr)->readyAPI(Gptr);
}

#undef GAMSSOLVER_ID
#endif

#ifdef GAMSLINKS_HAS_OSIXPR
#define GAMSSOLVER_ID oxp
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
   oxyInitialize();
}

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
   oxyFinalize();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,create)(void** Cptr, char* msgBuf, int msgBufLen)
{
   return oxycreate(Cptr, msgBuf, msgBufLen, GamsOsi::XPRESS);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   return oxyfree(Cptr);
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsOsi*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsOsi*)Cptr)->readyAPI(Gptr);
}

#undef GAMSSOLVER_ID
#endif
