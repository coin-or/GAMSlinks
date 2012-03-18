// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsScip.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// GAMS
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gmspal.h"  /* for audit line */
#endif

#include "GAMSlinksConfig.h"
#include "GamsCompatibility.h"

#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "reader_gmo.h"
#include "event_bbtrace.h"
#include "prop_defaultbounds.h"

static
SCIP_DECL_MESSAGEERROR(GamsScipPrintLogStat)
{
   assert(SCIPmessagehdlrGetData(messagehdlr) != NULL);
   assert(file != NULL);
   if( file != stderr )
      fprintf(file, msg);
   else
      gevLogStatPChar((gevHandle_t)SCIPmessagehdlrGetData(messagehdlr), msg);
}

static
SCIP_DECL_MESSAGEINFO(GamsScipPrintLog)
{
   assert(SCIPmessagehdlrGetData(messagehdlr) != NULL);
   assert(file != NULL);
   if( file != stdout )
      fprintf(file, msg);
   else
      gevLogPChar((gevHandle_t)SCIPmessagehdlrGetData(messagehdlr), msg);
}

//static
//SCIP_DECL_MESSAGEINFO(GamsScipPrintStdout)
//{
//   assert(file != NULL);
//   fputs(msg, file);
//   fflush(file);
//}

GamsScip::~GamsScip()
{
   SCIP_CALL_ABORT( freeSCIP() );
   assert(scipmsghandler == NULL);
}

int GamsScip::readyAPI(
   struct gmoRec*     gmo_,               /**< GAMS modeling object */
   struct optRec*     opt_                /**< GAMS options object */
)
{
   gmo = gmo_;
   assert(gmo != NULL);

   if( getGmoReady() || getGevReady() )
      return 1;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   char buffer[512];

#ifdef GAMS_BUILD
#include "coinlibdCL5svn.h"
   auditGetLine(buffer, sizeof(buffer));
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

   // check for academic license, or if we run in demo mode
   if( !checkAcademicLicense(gmo, isDemo) )
   {
      gevLogStat(gev, "*** Use of SCIP is limited to academic users.");
      gevLogStat(gev, "*** Please contact koch@zib.de to arrange for a license.");
      gmoSolveStatSet(gmo, gmoSolveStat_License);
      gmoModelStatSet(gmo, gmoModelStat_LicenseError);
      return 1;
   }

   // print version info and copyright
   if( SCIPsubversion() > 0 )
      sprintf(buffer, "SCIP version %d.%d.%d.%d\n", SCIPmajorVersion(), SCIPminorVersion(), SCIPtechVersion(), SCIPsubversion());
   else
      sprintf(buffer, "SCIP version %d.%d.%d\n", SCIPmajorVersion(), SCIPminorVersion(), SCIPtechVersion());
   gevLogStatPChar(gev, buffer);
   gevLogStatPChar(gev, SCIP_COPYRIGHT"\n\n");

   // setup (or reset) SCIP instance
   SCIP_RETCODE scipret;
   scipret = setupSCIP();

   if( scipret != SCIP_OKAY )
   {
      snprintf(buffer, sizeof(buffer), "Error %d in call of SCIP function\n", scipret);
      gevLogStatPChar(gev, buffer);
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return 1;
   }

   assert(scip != NULL);

   // print info on used external codes
   SCIPprintExternalCodes(scip, NULL);

   return 0;
}

int GamsScip::callSolver()
{
   assert(gmo  != NULL);
   assert(scip != NULL);

   if( gmoGetEquTypeCnt(gmo, gmoequ_C) || gmoGetEquTypeCnt(gmo, gmoequ_B) || gmoGetEquTypeCnt(gmo, gmoequ_X) )
   {
      gevLogStat(gev, "ERROR: Conic and logic constraints and external functions not supported by SCIP interface.\n");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 1;
   }

   // set number of threads for linear algebra routines used in Ipopt
   setNumThreadsBlas(gev, gevThreads(gev));

   SCIP_RETCODE scipret;

   // let GMO reader setup SCIP parameters and read options file
   // do this here already so we know how to assemble dialog
   scipret = SCIPreadParamsReaderGmo(scip);
   if( scipret != SCIP_OKAY )
   {
      char buffer[256];
      sprintf(buffer, "Error %d in call of SCIP function\n", scipret);
      gevLogStatPChar(gev, buffer);
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      return 1;
   }

   SCIP_Bool interactive;
   SCIP_CALL_ABORT( SCIPgetBoolParam(scip, "gams/interactive", &interactive) );
   if( interactive && isDemo )
   {
      gevLogStat(gev, "SCIP shell not available in demo mode.\n");
      interactive = FALSE;
   }

   SCIP_Bool printstat;
   SCIP_CALL_ABORT( SCIPgetBoolParam(scip, "gams/printstatistics", &printstat) );

   char* attrfile = NULL;
#if 0
   SCIP_CALL( SCIPgetStringParam(scip, "constraints/attrfile", &attrfile) );
#endif

   // setup commands to be executed by SCIP
   SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, "readgams") );              // setup model

   if( attrfile != NULL && *attrfile != '\0' )
   {
      char buffer[SCIP_MAXSTRLEN + 10];
      size_t len;

      len = strlen(attrfile);
      if( len >= 3 && strcmp(&attrfile[len-3], ".ca") == 0 )
         (void) SCIPsnprintf(buffer, sizeof(buffer), "read %g", attrfile);
      else
         (void) SCIPsnprintf(buffer, sizeof(buffer), "read %g ca", attrfile);
      SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, buffer) );               // process constraints attribute file
   }

   SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, "disp param") );            // display non-default parameter settings

   if( !interactive )
   {
      SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, "optimize") );           // solve model

      if( printstat )
      {
         SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, "disp statistics") ); // display solution statistics
      }
      SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, "write gamssol") );      // pass solution to GMO

      SCIP_CALL_ABORT( SCIPaddDialogInputLine(scip, "quit") );               // quit shell
   }


   // run SCIP
   scipret = SCIPstartInteraction(scip);

   // evaluate SCIP return code
   switch( scipret )
   {
      case SCIP_OKAY:
         break;

      case SCIP_READERROR:
         /* if it's readerror, then we guess that it comes from encountering an unsupported gams instruction in the gmo readers makeExprtree method
          * we still return with zero then
          */
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Capability);
         break;

      case SCIP_LPERROR:
      case SCIP_MAXDEPTHLEVEL:
         /* if SCIP failed due to internal error (forced LP solve failed, max depth level reached), also return zero */
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         break;

      case SCIP_NOMEMORY:
         /* there is no extra solver status for running out of memory, but memory is a resource, so return this */
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         break;

      default:
      {
         char buffer[256];
         sprintf(buffer, "Error %d in call of SCIP function\n", scipret);
         gevLogStatPChar(gev, buffer);

         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         return 1;
      }
   }

   return 0;
}

SCIP_RETCODE GamsScip::setupSCIP()
{
   if( scipmsghandler == NULL && gev != NULL )
   {
      /* print dialog messages to stdout if lo=1 or lo=3, but through gev if lo=0 or lo=2 */
      SCIP_CALL_ABORT( SCIPcreateMessagehdlr(&scipmsghandler, FALSE,
         GamsScipPrintLogStat, GamsScipPrintLogStat, GamsScipPrintLog, GamsScipPrintLog,
         (SCIP_MESSAGEHDLRDATA*)gev) );
      SCIP_CALL_ABORT( SCIPsetMessagehdlr(scipmsghandler) );
   }

   if( scip == NULL )
   {
      // if called first time, create a new SCIP instance and include all plugins that we need and setup interface parameters

      SCIP_CALL( SCIPcreate(&scip) );

      SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
      SCIP_CALL( SCIPincludeReaderGmo(scip) );
      SCIP_CALL( SCIPincludeEventHdlrBBtrace(scip) );
      /* SCIP_CALL( SCIPincludePropDefaultBounds(scip) ); */

      /* SCIP_CALL( SCIPaddBoolParam(scip, "gams/solvefinal",
       * "whether the problem should be solved with fixed discrete variables to get dual values",
       * NULL, FALSE, TRUE,  NULL, NULL) );
       */
      SCIP_CALL( SCIPaddBoolParam(scip, "gams/printstatistics",
         "whether to print statistics on a MIP solve",
         NULL, FALSE, FALSE, NULL, NULL) );
      SCIP_CALL( SCIPaddBoolParam(scip, "gams/interactive",
         "whether a SCIP shell should be opened instead of issuing a solve command",
         NULL, FALSE, FALSE, NULL, NULL) );
#if 0
      SCIP_CALL( SCIPaddStringParam(scip, "constraints/attrfile",
         "name of file that specifies constraint attributes",
         NULL, FALSE, "", NULL, NULL) );
#endif
   }
   else
   {
      // if called before, only clear up problem and reset parameters
      SCIP_CALL( SCIPfreeProb(scip) );
      SCIP_CALL( SCIPresetParams(scip) );
   }

   /** pass current GMO into GMO reader, so it does not read instance from file */
   SCIPsetGMOReaderGmo(scip, gmo);

   return SCIP_OKAY;
}

SCIP_RETCODE GamsScip::freeSCIP()
{
   if( scip != NULL )
   {
      SCIP_CALL( SCIPfree(&scip) );
   }

   if( scipmsghandler != NULL )
   {
      SCIP_CALL( SCIPsetDefaultMessagehdlr() );
      SCIP_CALL( SCIPfreeMessagehdlr(&scipmsghandler) );
   }

   return SCIP_OKAY;
}

#define GAMSSOLVERC_ID         scp
#define GAMSSOLVERC_CLASS      GamsScip
#include "GamsSolverC_tpl.cpp"
