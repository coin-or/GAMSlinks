// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsLinksConfig.h"
#include "GamsCbc.hpp"

#include <cstdlib>
#include <cstring>
#include <climits>
#include <list>
#include <string>

#ifdef _MSC_VER
#define strdup _strdup
#endif

#include "gclgms.h"
#include "gmomcc.h"
#include "gevmcc.h"
#include "optcc.h"
#include "gdxcc.h"
#include "palmcc.h"
#include "cfgmcc.h"

#include "GamsMessageHandler.hpp"
#include "GamsOsiHelper.hpp"
#include "GamsSolveTrace.h"
#include "GamsHelper.h"

// For Cbc
#include "CbcConfig.h"
#include "CbcSolver.hpp"
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp"  // for CbcSOS
#include "CbcBranchLotsize.hpp" // for CbcLotsize
#include "CbcOrClpParam.hpp"

#include "OsiClpSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"

//#if defined(__linux) && defined(GAMSLINKS_HAS_CPLEX)
//#include "cplex.h"
//#endif

/** CBC callback, used for updating model pointer
 * forward declaration, so we can define a static function as friend of special message handler
 */
static
int cbcCallBack(
   CbcModel*          model,              /**< CBC model that calls this function */
   int                whereFrom           /**< indicator at which time in the CBC algorithm it is called */
);

/** specification of GamsMessageHandler that omits messages from mini B&B in Cbc */
class GamsCbcMessageHandler : public GamsMessageHandler
{
   friend int cbcCallBack(
      CbcModel*          model,              /**< CBC model that calls this function */
      int                whereFrom           /**< indicator at which time in the CBC algorithm it is called */
   );

private:
   CbcModel*             model;              /**< CBC model, or NULL */

public:
   GamsCbcMessageHandler(
      struct gevRec*     gev_,               /**< GAMS environment */
      CbcModel*          model_ = NULL       /**< CBC model */
   )
   : GamsMessageHandler(gev_),
     model(model_)
   { }

   /** call print message of upper class, if not in mini B&B */
   int print()
   {
      if( model != NULL && model->waitingForMiniBranchAndBound() )
         return 0;
      return GamsMessageHandler::print();
   }

   /** creates a copy of this message handler */
   CoinMessageHandler* clone() const
   {
      return new GamsCbcMessageHandler(gev, model);
   }
};

/** CbcEventHandler that reports bounds to a GAMS solve trace object */
class GamsSolveTraceEventHandler : public CbcEventHandler
{
   friend int cbcCallBack(
      CbcModel*          model,              /**< CBC model that calls this function */
      int                whereFrom           /**< indicator at which time in the CBC algorithm it is called */
   );

private:
   GAMS_SOLVETRACE*      solvetrace;         /**< GAMS solve trace data structure */
   double                objfactor;          /**< multiplier for objective function values */
   CbcModel*             mainmodel;          /**< the CbcModel for which we want to trace the solving process */

public:
   GamsSolveTraceEventHandler(
      GAMS_SOLVETRACE*   solvetrace_,        /**< GAMS solve trace data structure */
      double             objfactor_ = 1.0    /**< multiplier for objective function values */
   )
   : solvetrace(solvetrace_),
     objfactor(objfactor_),
     mainmodel(NULL)
   { }

   CbcEventHandler* clone() const
   {
      // TODO clone only if model_ != mainmodel, so we don't clone ourself into sub-cbc?
      return new GamsSolveTraceEventHandler(solvetrace, objfactor);
   }

   CbcAction event(
      CbcEvent           whichEvent
   )
   {
      if( model_ == mainmodel && (whichEvent == node || whichEvent == solution || whichEvent == heuristicSolution) )
      {
         GAMSsolvetraceAddLine(solvetrace, model_->getNodeCount(),
            objfactor * model_->getBestPossibleObjValue(),
            model_->getSolutionCount() > 0 ? objfactor * model_->getObjValue() : (objfactor > 0.0 ? 1.0 : -1.0) * model_->getObjSense() * model_->getInfinity());
      }

      return noAction;
   }
};

static
int cbcCallBack(
   CbcModel*          model,              /**< CBC model that calls this function */
   int                whereFrom           /**< indicator at which time in the CBC algorithm it is called */
)
{
   if( whereFrom == 3 ) /* just before B&B */
   {
      /* reset model in message handler  */
      GamsCbcMessageHandler* msghandler = dynamic_cast<GamsCbcMessageHandler*>(model->messageHandler());
      assert(msghandler != NULL);
      msghandler->model = model;

      /* update mainmodel in event handler */
      GamsSolveTraceEventHandler* eventhandler = dynamic_cast<GamsSolveTraceEventHandler*>(model->getEventHandler());
      if( eventhandler != NULL )
         eventhandler->mainmodel = model;
   }
   else if( whereFrom == 4 ) /* just after B&B */
   {
      /* clear model in message handler */
      GamsCbcMessageHandler* handler = dynamic_cast<GamsCbcMessageHandler*>(model->messageHandler());
      assert(handler != NULL);
      handler->model = NULL;

      /* clear mainmodel in event handler */
      GamsSolveTraceEventHandler* eventhandler = dynamic_cast<GamsSolveTraceEventHandler*>(model->getEventHandler());
      if( eventhandler != NULL )
         eventhandler->mainmodel = NULL;
   }

   return 0;
}

GamsCbc::~GamsCbc()
{
   delete model;
   delete msghandler;
   for( int i = 0; i < cbc_argc; ++i )
      free(cbc_args[i]);
   delete[] cbc_args;

   free(writemps);
   free(solvetrace);
   free(dumpsolutions);
   free(dumpsolutionsmerged);
}

int GamsCbc::readyAPI(
   struct gmoRec*     gmo_,               /**< GAMS modeling object */
   struct optRec*     opt_                /**< GAMS options object */
)
{
   struct palRec* pal;
   char buffer[GMS_SSSIZE];

   gmo = gmo_;
   assert(gmo != NULL);
   opt = opt_;

   delete model;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   if( !palCreate(&pal, buffer, sizeof(buffer)) )
   {
      gevLogStat(gev, buffer);
      return 1;
   }

#if PALAPIVERSION >= 3
   palSetSystemName(pal, "COIN-OR CBC");
   palGetAuditLine(pal, buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

   palFree(&pal);

   gevLogStatPChar(gev, "\nCOIN-OR Branch and Cut (CBC Library " CBC_VERSION ")\nwritten by J. Forrest\n");

   if( msghandler == NULL )
   {
      msghandler = new GamsCbcMessageHandler(gev);
   }

   return 0;
}

int GamsCbc::callSolver()
{
   assert(gmo != NULL);
   assert(gev != NULL);

   delete model;
   model = NULL;

   gmoSolveStatSet(gmo, gmoSolveStat_Solver);
   gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);

   /* set MIP */
   if( !setupProblem() )
   {
      gevLogStat(gev, "Error setting up problem. Aborting...");
      return -1;
   }

   /* initialize Cbc */
   CbcSolverUsefulData cbcData;
   CbcMain0(*model, cbcData);

   if( !setupParameters() )
   {
      gevLogStat(gev, "Error setting up CBC parameters. Aborting...");
      return -1;
   }
   assert(cbc_args != NULL);
   assert(cbc_argc > 0);

   if( !setupStartingPoint() )
   {
      gevLogStat(gev, "Error setting up starting point. Aborting...");
      return -1;
   }

   if( writemps != NULL )
   {
      gevLogStatPChar(gev, "\nWriting MPS file ");
      gevLogStat(gev, writemps);
      model->solver()->writeMps(writemps, "", 1.0);
   }

   /* initialize solvetrace
    * do this almost immediately before calling solve, so the timing value is accurate
    */
   GAMS_SOLVETRACE* solvetrace_ = NULL;
   if( solvetrace != NULL && solvetrace[0] )
   {
      char buffer[GMS_SSSIZE];
      int rc;

      gmoNameInput(gmo, buffer);
      rc = GAMSsolvetraceCreate(&solvetrace_, solvetrace, "CBC", gmoOptFile(gmo), buffer, model->getInfinity(), solvetracenodefreq, solvetracetimefreq);
      if( rc != 0 )
      {
         gevLogStat(gev, "Initializing solvetrace failed.");
         GAMSsolvetraceFree(&solvetrace_);
      }
      else
      {
         GamsSolveTraceEventHandler traceeventhdlr(solvetrace_);
         model->passInEventHandler(&traceeventhdlr);
      }
   }

   gevLogStat(gev, "\nCalling CBC main solution routine...");

   double start_cputime  = CoinCpuTime();
   double start_walltime = CoinWallclockTime();

   CbcMain1(cbc_argc, const_cast<const char**>(cbc_args), *model, cbcCallBack, cbcData);

   double end_cputime  = CoinCpuTime();
   double end_walltime = CoinWallclockTime();

   if( solvetrace_ != NULL )
   {
      GAMSsolvetraceAddEndLine(solvetrace_, model->getNodeCount(),
         model->getBestPossibleObjValue(),
         model->getSolutionCount() > 0 ? model->getObjValue() : model->getObjSense() * model->getInfinity());
      GAMSsolvetraceFree(&solvetrace_);
   }

   writeSolution(end_cputime - start_cputime, end_walltime - start_walltime);

   // solve again with fixed noncontinuous variables and original bounds on continuous variables
   if( !isLP() && model->bestSolution() && solvefinal )
   {
      gevLog(gev, "\nResolve with fixed discrete variables.");

      double* varlow = new double[gmoN(gmo)];
      double* varup  = new double[gmoN(gmo)];
      enum gmoVarType vartype;
      gmoGetVarLower(gmo, varlow);
      gmoGetVarUpper(gmo, varup);
      for( int i = 0; i < gmoN(gmo); ++i )
      {
         vartype = (enum gmoVarType)gmoGetVarTypeOne(gmo, i);
         /* if SOS, only fix if at 0: gives better duals on SOS2a
          * TODO if semicontinuous/integer, do something similar?
          */
         if( vartype == gmovar_B ||
             vartype == gmovar_I ||
             (vartype == gmovar_S1 && fabs(model->bestSolution()[i]) < 1e-9) ||
             (vartype == gmovar_S2 && fabs(model->bestSolution()[i]) < 1e-9) ||
             vartype == gmovar_SC || vartype == gmovar_SI )
         {
            varlow[i] = varup[i] = model->bestSolution()[i];
         }
      }
      model->solver()->setColLower(varlow);
      model->solver()->setColUpper(varup);

      model->solver()->messageHandler()->setLogLevel(1, 1);
      // model->solver()->setHintParam(OsiDoPresolveInResolve, false, OsiHintTry);
      model->solver()->resolve();
      if( model->solver()->isProvenOptimal() )
      {
         if( !gamsOsiStoreSolution(gmo, *model->solver()) )
         {
            gevLogStat(gev, "Failed to store LP solution. Only primal solution values will be available in GAMS solution file.\n");
         }
      }
      else
      {
         gevLog(gev, "Resolve failed, values for dual variables will not be available.");
      }

      delete[] varlow;
      delete[] varup;
   }

   delete model;
   model = NULL;

   return 0;
}

bool GamsCbc::setupProblem()
{
   {
      OsiClpSolverInterface solver;

      gmoPinfSet(gmo,  solver.getInfinity());
      gmoMinfSet(gmo, -solver.getInfinity());
      gmoObjReformSet(gmo, 1);
      gmoObjStyleSet(gmo, gmoObjType_Fun);
      gmoIndexBaseSet(gmo, 0);

      if( !gamsOsiLoadProblem(gmo, solver, true) )
         return false;

      if( gmoN(gmo) == 0 )
      {
         gevLog(gev, "Problem has no columns. Adding fake column...");
         CoinPackedVector vec(0);
         solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.0);
      }

      // setup CbcModel
      model = new CbcModel(solver);
   }

   model->passInMessageHandler(msghandler);
   // disable RINS initialization output for loglevel <= 2
   assert(model->messages().numberMessages_ > CBC_FPUMP1);
   model->messages().message_[CBC_FPUMP1]->setDetail(3);

   // assemble integer variable branching priorities
   // range of priority values
   double minprior =  model->solver()->getInfinity();
   double maxprior = -model->solver()->getInfinity();
   if( gmoPriorOpt(gmo) && gmoNDisc(gmo) > 0 )
   {
      // first check which range of priorities is given
      for (int i = 0; i < gmoN(gmo); ++i)
      {
         if( gmoGetVarTypeOne(gmo, i) == gmovar_X )
            continue; // gams forbids branching priorities for continuous variables
         if( gmoGetVarPriorOne(gmo,i) < minprior )
            minprior = gmoGetVarPriorOne(gmo,i);
         if( gmoGetVarPriorOne(gmo,i) > maxprior )
            maxprior = gmoGetVarPriorOne(gmo,i);
      }
      if( minprior != maxprior )
      {
         int* cbcprior = new int[gmoNDisc(gmo)];
         int j = 0;
         for( int i = 0; i < gmoN(gmo); ++i )
         {
            if( gmoGetVarTypeOne(gmo, i) == gmovar_X )
               continue; // gams forbids branching priorities for continuous variables
            // we map gams priorities into the range {1,..,1000}
            // (1000 is standard priority in Cbc, and 1 is highest priority)
            assert(j < gmoNDisc(gmo));
            cbcprior[j++] = 1 + (int)(999* (gmoGetVarPriorOne(gmo,i) - minprior) / (maxprior - minprior));
         }
         assert(j == gmoNDisc(gmo));

         model->passInPriorities(cbcprior, false);
         delete[] cbcprior;
      }
   }

   // assemble SOS of type 1 or 2
   int numSos1, numSos2, nzSos;
   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
   if( nzSos > 0 )
   {
      int numSos = numSos1 + numSos2;
      CbcObject** sosobjects = new CbcObject*[numSos];

      int* sostype  = new int[numSos];
      int* sosbeg   = new int[numSos+1];
      int* sosind   = new int[nzSos];
      double* soswt = new double[nzSos];

      gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);

      int*    which   = new int[CoinMin(gmoN(gmo), nzSos)];
      double* weights = new double[CoinMin(gmoN(gmo), nzSos)];

      int cnt = 0;
      for( int i = 0; i < numSos; ++i )
      {
         // skip trivial SOS (SOS1 with at most one variable and SOS2 with at most two variables); not necessary, but also easy to do
         if( sosbeg[i+1] - sosbeg[i] <= sostype[i] )
            continue;

         int k = 0;
         for( int j = sosbeg[i]; j < sosbeg[i+1]; ++j, ++k )
         {
            which[k]   = sosind[j];
            weights[k] = soswt[j];
            assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? gmovar_S1 : gmovar_S2));
         }
         sosobjects[cnt] = new CbcSOS(model, k, which, weights, i, sostype[i]);
         sosobjects[cnt]->setPriority(gmoN(gmo)-k); // branch on long sets first
         ++cnt;
      }

      model->addObjects(cnt, sosobjects);

      for( int i = 0; i < cnt; ++i )
         delete sosobjects[i];
      delete[] sosobjects;
      delete[] which;
      delete[] weights;
      delete[] sostype;
      delete[] sosbeg;
      delete[] sosind;
      delete[] soswt;
   }

   // assemble semicontinuous and semiinteger variables
   int numSemi = gmoGetVarTypeCnt(gmo, gmovar_SC) + gmoGetVarTypeCnt(gmo, gmovar_SI);
   if( numSemi > 0)
   {
      CbcObject** semiobjects = new CbcObject*[numSemi];
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
            semiobjects[object_nr] = new CbcLotsize(model, i, len, points2, false);
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
               semiobjects[object_nr] = new CbcLotsize(model, i, 2, points+1, false);
            else // var. can be 0 or in the range between low and upper
               semiobjects[object_nr] = new CbcLotsize(model, i, 2, points,   true );
         }

         // set actual lower bound of variable in solver
         model->solver()->setColLower(i, 0.0);

         ++object_nr;
      }
      assert(object_nr == numSemi);

      model->addObjects(numSemi, semiobjects);

      for( int i = 0; i < numSemi; ++i )
         delete semiobjects[i];
      delete[] semiobjects;
   }

   return true;
}

bool GamsCbc::setupStartingPoint()
{
   assert(gmo != NULL);
   assert(model != NULL);

   bool    retcode = true;
   double* varlevel = NULL;
   double* rowprice = NULL;
   int*    cstat    = NULL;
   int*    rstat    = NULL;

   if( gmoHaveBasis(gmo) )
   {
      varlevel = new double[gmoN(gmo)];
      rowprice = new double[gmoM(gmo)];
      cstat    = new int[gmoN(gmo)];
      rstat    = new int[gmoM(gmo)];

      gmoGetVarL(gmo, varlevel);
      gmoGetEquM(gmo, rowprice);

      int nbas = 0;
      for( int j = 0; j < gmoN(gmo); ++j )
      {
         switch( gmoGetVarStatOne(gmo, j) )
         {
            case gmoBstat_Basic:
               if( nbas < gmoM(gmo) )
               {
                  cstat[j] = 1; // basic
                  ++nbas;
               }
               else if( gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) )
                  cstat[j] = 0; // superbasic = free
               else if( gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) || gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j) < gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j) )
                  cstat[j] = 3; // nonbasic at lower bound
               else
                  cstat[j] = 2; // nonbasic at upper bound
               break;

            case gmoBstat_Lower:
            case gmoBstat_Upper:
            case gmoBstat_Super:
               if( gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) )
                  cstat[j] = 0; // superbasic = free
               if( gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) || gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j) < gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j) )
                  cstat[j] = 3; // nonbasic at lower bound
               else
                  cstat[j] = 2; // nonbasic at upper bound
               break;

            default:
               gevLogStat(gev, "Error: invalid basis indicator for column.");
               retcode = false;
               goto TERMINATE;
         }
      }

      for( int j = 0; j< gmoM(gmo); ++j )
      {
         switch( gmoGetEquStatOne(gmo, j) )
         {
            case gmoBstat_Basic:
               if( nbas < gmoM(gmo) )
               {
                  rstat[j] = 1; // basic
                  ++nbas;
               }
               else
               {
                  rstat[j] = gmoGetEquTypeOne(gmo, j) == gmoequ_L ? 3 : 2; // nonbasic at some bound
               }
               break;

            case gmoBstat_Lower:
            case gmoBstat_Upper:
            case gmoBstat_Super:
               rstat[j] = gmoGetEquTypeOne(gmo, j) == gmoequ_L ? 3 : 2; // nonbasic at some bound
               break;

            default:
               gevLogStat(gev, "Error: invalid basis indicator for row.");
               retcode = false;
               goto TERMINATE;
         }
      }

      model->solver()->setColSolution(varlevel);
      model->solver()->setRowPrice(rowprice);

      // this call initializes ClpSimplex data structures, which can produce an error if CLP does not like the model
      if( model->solver()->setBasisStatus(cstat, rstat) ) {
         gevLogStat(gev, "Failed to set initial basis. Probably CLP abandoned the model.");
         // ignoring this error for now, we probably can still solve, #3610
         // retcode = false;
         // goto TERMINATE;
      }
   }

   if( mipstart && gmoModelType(gmo) != gmoProc_lp && gmoModelType(gmo) != gmoProc_rmip )
   {
      double objval;
      model->solver()->getDblParam(OsiObjOffset, objval);
      objval = -objval;
      if( varlevel == NULL )
      {
         varlevel = new double[gmoN(gmo)];
         gmoGetVarL(gmo, varlevel);
      }
      const double* objcoeff = model->solver()->getObjCoefficients();
      for( int i = 0; i < gmoN(gmo); ++i )
         objval += objcoeff[i] * varlevel[i];
      gevLogPChar(gev, "Letting CBC try user-given column levels as initial MIP solution: ");
      model->setBestSolution(varlevel, gmoN(gmo), objval, true);
   }

   TERMINATE:
   delete[] varlevel;
   delete[] rowprice;
   delete[] cstat;
   delete[] rstat;

   return retcode;
}

bool GamsCbc::setupParameters()
{
   assert(gmo != NULL);
   assert(gev != NULL);

   char buffer[2*GMS_SSSIZE];
   bool createdopt = false;

   if( opt == NULL )
   {
      /* get the Option File Handling set up */
      if( !optCreate(&opt, buffer, sizeof(buffer)) )
      {
         gevLogStatPChar(gev, "\n*** Could not create optionfile handle: ");
         gevLogStat(gev, buffer);
         return false;
      }
      createdopt = true;

      // get definition file name from cfg object
      char deffile[2*GMS_SSSIZE+20];
      cfgHandle_t cfg;
      cfg = (cfgHandle_t)gevGetALGX(gev);
      assert(cfg != NULL);
      gevGetCurrentSolver(gev, gmo, buffer);
      deffile[0] = '\0';
      cfgDefFileName(cfg, buffer, deffile);
      if( deffile[0] != '/' && deffile[1] != ':' )
      {
         // if deffile is not absolute path, then prefix with sysdir
         gevGetStrOpt(gev, gevNameSysDir, buffer);
         strcat(buffer, deffile);
         strcpy(deffile, buffer);
      }
      if( optReadDefinition(opt, deffile) )
      {
         int itype;
         for( int i = 1; i <= optMessageCount(opt); ++i )
         {
            optGetMessage(opt, i, buffer, &itype);
            if( itype <= optMsgFileLeave || itype == optMsgUserError )
               gevLogStat(gev, buffer);
         }
         optClearMessages(opt);
         optEchoSet(opt, 0);
         if( createdopt )
            optFree(&opt);
         return false;
      }
      optEOLOnlySet(opt, 1);

      // read user options file
      if( gmoOptFile(gmo) > 0 )
      {
         gmoNameOptFile(gmo, buffer);

         /* read option file */
         optEchoSet(opt, 1);
         optReadParameterFile(opt, buffer);
         if( optMessageCount(opt) )
         {
            int itype;
            for( int i = 1; i <= optMessageCount(opt); ++i )
            {
               optGetMessage(opt, i, buffer, &itype);
               if( itype <= optMsgFileLeave || itype == optMsgUserError )
                  gevLogStat(gev, buffer);
            }
            optClearMessages(opt);
            optEchoSet(opt, 0);
         }
         else
         {
            optEchoSet(opt, 0);
         }
      }
   }

   // overwrite Cbc defaults with values from GAMS options, if not set in options file
   if( !optGetDefinedStr(opt, "reslim") )
      optSetDblStr(opt, "reslim", gevGetDblOpt(gev, gevResLim));
   if( !optGetDefinedStr(opt, "iterlim") && gevGetIntOpt(gev, gevIterLim) < INT_MAX )
      optSetIntStr(opt, "iterlim", gevGetIntOpt(gev, gevIterLim));
   if( !optGetDefinedStr(opt, "nodlim") && gevGetIntOpt(gev, gevNodeLim) > 0 )
      optSetIntStr(opt, "nodlim", gevGetIntOpt(gev, gevNodeLim));
   if( !optGetDefinedStr(opt, "optca") )
      optSetDblStr(opt, "optca", gevGetDblOpt(gev, gevOptCA));
   if( !optGetDefinedStr(opt, "optcr") )
      optSetDblStr(opt, "optcr", gevGetDblOpt(gev, gevOptCR));
   // Cbc assumes a minimization problem, so set or correct cutoff value accordingly
   if( !optGetDefinedStr(opt, "cutoff") )
   {
      if( gevGetIntOpt(gev, gevUseCutOff) )
         optSetDblStr(opt, "cutoff", model->solver()->getObjSense() * gevGetDblOpt(gev, gevCutOff));
   }
   else if( model->solver()->getObjSense() == - 1)
      optSetDblStr(opt, "cutoff", -optGetDblStr(opt, "cutoff"));
   if( !optGetDefinedStr(opt, "increment") && gevGetIntOpt(gev, gevUseCheat) )
      optSetDblStr(opt, "increment", gevGetDblOpt(gev, gevCheat));
   if( CbcModel::haveMultiThreadSupport() && !optGetDefinedStr(opt, "threads") )
      optSetIntStr(opt, "threads", gevThreads(gev));

   // MIP parameters
   optca = optGetDblStr(opt, "optca");
   optcr = optGetDblStr(opt, "optcr");
   model->setPrintFrequency(optGetIntStr(opt, "printfrequency"));

   std::vector<CbcOrClpParam> cbcparams;
   if( optCount(opt) > 0 )
      establishParams(cbcparams);

   std::list<std::string> par_list;

   if( optGetIntStr(opt, "conflictcuts") )
   {
      par_list.push_back("-constraint");
      par_list.push_back("conflict");
   }

   /* Apply Cbc options */
   for( int i = 1; i <= optCount(opt); ++i )
   {
      int idefined;
      int idummy;
      int irefnr;
      int itype; /* data type */
      int iotype; /* option type */
      int ival;
      int pos;
      double dval;
      char sname[GMS_SSSIZE];
      char sval[GMS_SSSIZE];

      optGetInfoNr(opt, i, &idefined, &idummy, &irefnr, &itype, &iotype, &idummy);
      if( itype == optDataNone || irefnr < 0 || 0 == idefined )
         continue;

      optGetValuesNr(opt, i, sname, &ival, &dval, sval);

      /* first 3 "parameters" are ?, ???, and - */
      pos = whichParam(CbcOrClpParameterType(irefnr), cbcparams);
      if( pos > (int)cbcparams.size() )
      {
         sprintf(buffer, "ERROR: Option %s not found in CBC (invalid reference number %d?)\n", sname, irefnr);
         gevLogStatPChar(gev, buffer);
         if( createdopt )
            optFree(&opt);
         return false;
      }
      assert(cbcparams.at(pos).type() == irefnr);

      // skip threads here, will handle this below
      if( irefnr == CBC_PARAM_INT_THREADS )
         continue;

      sprintf(buffer, "-%s", cbcparams.at(pos).name().c_str());
      par_list.push_back(buffer);

      switch( iotype )
      {
         case optTypeBoolean:
            assert(itype == optDataInteger);
            par_list.push_back(ival == 0 ? "off" : "on");
            break;

         case optTypeInteger:
            assert(itype == optDataInteger);
            sprintf(buffer, "%d", ival);
            par_list.push_back(buffer);
            break;

         case optTypeDouble:
            assert(itype == optDataDouble);
            sprintf(buffer, "%g", dval);
            par_list.push_back(buffer);
            break;

         case optTypeEnumStr:
            assert(itype == optDataString);

            // for backward compatibility
            if( strcmp(sval, "binaryfirst") == 0 )
            {
               sprintf(buffer, "WARNING: Value 'binaryfirst' for option %s is deprecated. Use '01first' instead.\n", sname);
               gevLogStatPChar(gev, buffer);
               par_list.push_back("01first");
            }
            else if( strcmp(sval, "binarylast") == 0 )
            {
               sprintf(buffer, "WARNING: Value 'binarylast' for option %s is deprecated. Use '01first' instead.\n", sname);
               gevLogStatPChar(gev, buffer);
               par_list.push_back("01last");
            }
            else if( strcmp(sval, "1") == 0 )
            {
               sprintf(buffer, "WARNING: Option %s is no longer of boolean type. Use value 'on' instead.\n", sname);
               gevLogStatPChar(gev, buffer);
               par_list.push_back("on");
            }
            else if( strcmp(sval, "0") == 0 )
            {
               sprintf(buffer, "WARNING: Option %s is no longer of boolean type. Use value 'off' instead.\n", sname);
               gevLogStatPChar(gev, buffer);
               par_list.push_back("off");
            }
            else if( strcmp(sval, "auto") == 0 )
            {
               sprintf(buffer, "WARNING: Value 'auto' for option %s is deprecated. Use value 'automatic' instead.\n", sname);
               gevLogStatPChar(gev, buffer);
               par_list.push_back("off");
            }
            else
               par_list.push_back(sval);
            break;

         default:
         {
            sprintf(buffer, "ERROR: Option %s is of unknown type %d\n", sname, iotype);
            gevLogStatPChar(gev, buffer);
            if( createdopt )
               optFree(&opt);
            return false;
         }
      }
   }

   // switch to wallclock-time in Cbc, if not requested otherwise
   char clocktype[GMS_SSSIZE];
   if( optGetStrStr(opt, "clocktype", clocktype) == NULL || strcmp(clocktype, "wall") == 0 )
      model->setUseElapsedTime(true);

   nthreads = CbcModel::haveMultiThreadSupport() ? optGetIntStr(opt, "threads") : 1;
   if( nthreads > 1 && CbcModel::haveMultiThreadSupport() )
   {
      // Cbc runs deterministic when 100 is added to nthreads
      char* value = optGetStrStr(opt, "parallelmode", buffer);
      if( value != NULL && strcmp(value, "deterministic") == 0 )
      {
         snprintf(buffer, sizeof(buffer), "\nParallel mode: deterministic, using %d threads\n", nthreads);
         nthreads += 100;
      }
      else
      {
         if( value == NULL )
            gevLogStat(gev, "Cannot read value for option 'parallelmode'. Ignoring this option");
         snprintf(buffer, sizeof(buffer), "\nParallel mode: opportunistic, using %d threads\n", nthreads);
      }
      gevLogStatPChar(gev, buffer);

      par_list.push_back("-threads");
      sprintf(buffer, "%d", nthreads);
      par_list.push_back(buffer);

      // no linear algebra multithreading if Cbc is doing multithreading
      GAMSsetNumThreads(gev, 1);
   }
   else
   {
      if( nthreads > 1 )
      {
         gevLogStat(gev, "Warning: Multithreading support not available in CBC.");
         snprintf(buffer, sizeof(buffer), "\nParallel mode: none, using %d threads in linear algebra\n", nthreads);
         gevLogStatPChar(gev, buffer);
      }
      //else
      //   gevLogStatPChar(gev, "Parallel mode: none, using 1 thread\n");

      // allow linear algebra multithreading
      GAMSsetNumThreads(gev, nthreads);
      nthreads = 1;
   }

   // special options set by user and passed unseen to CBC
   if( optGetDefinedStr(opt, "special") )
   {
      char* value = optGetStrStr(opt, "special", buffer);
      if( value == NULL )
      {
         gevLogStatPChar(gev, "Cannot read value for option 'special'. Ignoring this option.\n");
      }
      else
      {
         char* tok = strtok_r(value, " ", &value);
         while( tok != NULL )
         {
            par_list.push_back(tok);
            tok = strtok_r(NULL, " ", &value);
         }
      }
   }

   // algorithm for root node and solve command
   if( optGetDefinedStr(opt, "startalg") )
   {
      char* value = optGetStrStr(opt, "startalg", buffer);
      if( value == NULL )
      {
         gevLogStat(gev, "Cannot read value for option 'startalg'. Ignoring this option");
      }
      else if( strcmp(value, "primal") == 0 )
      {
         par_list.push_back("-primalSimplex");
      }
      else if( strcmp(value, "dual") == 0 )
      {
         par_list.push_back("-dualSimplex");
      }
      else if( strcmp(value, "barrier") == 0 )
      {
         par_list.push_back("-barrier");
      }
      else
      {
         gevLogStat(gev, "Unsupported value for option 'startalg'. Ignoring this option");
      }
      if( !isLP() )
         par_list.push_back("-solve");
   }
   else
      par_list.push_back("-solve");

   size_t par_list_length = par_list.size();
   if( cbc_args != NULL ) {
      for( int i = 0; i < cbc_argc; ++i )
         free(cbc_args[i]);
      delete[] cbc_args;
   }
   cbc_argc = (int)par_list_length+2;
   cbc_args = new char*[cbc_argc];
   cbc_args[0] = strdup("GAMS/CBC");
   int i = 1;
   for( std::list<std::string>::iterator it(par_list.begin()); it != par_list.end(); ++it, ++i )
   {
      cbc_args[i] = strdup(it->c_str());
      //std::cout << cbc_args[i] << std::endl;
   }
   cbc_args[i++] = strdup("-quit");

   mipstart = optGetIntStr(opt, "mipstart") != 0;
   solvefinal = optGetIntStr(opt, "solvefinal") != 0;

   // whether to write MPS file
   free(writemps);
   writemps = NULL;
   if( optGetDefinedStr(opt, "writemps") )
   {
      optGetStrStr(opt, "writemps", buffer);
      writemps = strdup(buffer);
   }

   // whether to write MIP trace file
   free(solvetrace);
   solvetrace = NULL;
   if( optGetDefinedStr(opt, "solvetrace") )
   {
      optGetStrStr(opt, "solvetrace", buffer);
      solvetrace = strdup(buffer);
   }
   solvetracenodefreq = optGetIntStr(opt, "solvetracenodefreq");
   solvetracetimefreq = optGetDblStr(opt, "solvetracetimefreq");

   // when to print which messages
   // loglevel default is 1, so enable CBC output by default, CoinUtils and CGL if loglevel >= 3, and CLP if loglevel >= 5 or >= 1 if problem is an LP
   int loglevel = optGetIntStr(opt, "loglevel");
   msghandler->setLogLevel(0, loglevel);   // CBC output
   msghandler->setLogLevel(1, isLP() ? loglevel : loglevel-4); // CLP output
   msghandler->setLogLevel(2, loglevel-2); // CoinUtils output, like COIN_PRESOLVE_STATS
   msghandler->setLogLevel(3, loglevel-2); // CGL output, like CGL_PROCESS_STATS2

   // whether to write other found solutions, and how many of them
   free(dumpsolutions);
   dumpsolutions = NULL;
   if( optGetDefinedStr(opt, "dumpsolutions") )
   {
      optGetStrStr(opt, "dumpsolutions", buffer);
      dumpsolutions = strdup(buffer);
   }
   free(dumpsolutionsmerged);
   dumpsolutionsmerged = NULL;
   if( optGetDefinedStr(opt, "dumpsolutionsmerged") )
   {
      optGetStrStr(opt, "dumpsolutionsmerged", buffer);
      dumpsolutionsmerged = strdup(buffer);
   }

   if( createdopt )
      optFree(&opt);

   return true;
}

bool GamsCbc::writeSolution(
   double             cputime,            /**< CPU time spend by solver */
   double             walltime            /**< wallclock time spend by solver */
)
{
   bool write_solution = false;

   gmoSetHeadnTail(gmo, gmoHiterused, model->getIterationCount());
   gmoSetHeadnTail(gmo, gmoHresused,  nthreads > 1 ? walltime : cputime);
   gmoSetHeadnTail(gmo, gmoTmipbest,  model->getBestPossibleObjValue());
   gmoSetHeadnTail(gmo, gmoTmipnod,   model->getNodeCount());

   gevLogStat(gev, "");
   if( isLP() )
   {
      // if Cbc solved an LP, the solution status is not correctly stored in the CbcModel, we have to look into the solver
      if( model->solver()->isProvenDualInfeasible() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_Unbounded);
         gevLogStat(gev, "Model unbounded.");
      }
      else if( model->solver()->isProvenPrimalInfeasible() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleGlobal);
         gevLogStat(gev, "Model infeasible.");
      }
      else if( model->solver()->isAbandoned() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gevLogStat(gev, "Model abandoned.");

      }
      else if( model->solver()->isProvenOptimal() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
         gevLogStat(gev, "Solved to optimality.");
      }
      else if( model->isSecondsLimitReached() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         gevLogStat(gev, "Time limit reached.");
      }
      else if( model->solver()->isIterationLimitReached() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         gevLogStat(gev, "Iteration limit reached.");
      }
      else if( model->solver()->isPrimalObjectiveLimitReached() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         gevLogStat(gev, "Primal objective limit reached.");
      }
      else if( model->solver()->isDualObjectiveLimitReached() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
         gevLogStat(gev, "Dual objective limit reached.");
      }
      else
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gevLogStat(gev, "Model status unknown, no feasible solution found.");
      }
   }
   else
   {
      // solved a MIP
      if( model->solver()->isProvenDualInfeasible() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_UnboundedNoSolution);
         gevLogStat(gev, "Model unbounded.");
      }
      else if( model->isAbandoned() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gevLogStat(gev, "Model abandoned.");
      }
      else if( model->isProvenOptimal() )
      {
         write_solution = true;
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         if( (optca > 0.0 || optcr > 0.0) && gmoGetRelativeGap(gmo) > 1e-9 )
         {
            gmoModelStatSet(gmo, gmoModelStat_Integer);
            gevLogStat(gev, "Solved to optimality (within gap tolerances optca and optcr).");
         }
         else
         {
            gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
            gevLogStat(gev, "Solved to optimality.");
         }
      }
      else if( model->isNodeLimitReached() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         if( model->bestSolution() )
         {
            write_solution = true;
            gmoModelStatSet(gmo, gmoModelStat_Integer);
            gevLogStat(gev, "Node limit reached. Have feasible solution.");
         }
         else
         {
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            gevLogStat(gev, "Node limit reached. No feasible solution found.");
         }
      }
      else if( model->isSecondsLimitReached() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         if( model->bestSolution() )
         {
            write_solution = true;
            gmoModelStatSet(gmo, gmoModelStat_Integer);
            gevLogStat(gev, "Time limit reached. Have feasible solution.");
         }
         else
         {
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            gevLogStat(gev, "Time limit reached. No feasible solution found.");
         }
      }
      else if( model->isProvenInfeasible() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
         gevLogStat(gev, "Model infeasible.");
      }
      else if( (model->status() == 1 && model->secondaryStatus() == 3) || model->status() == 5 )
      {
         /* secondary status 3 is actually nodelimit, but indicates user interrupt for cbc < r1777 */
         gmoSolveStatSet(gmo, gmoSolveStat_User);
         if( model->bestSolution() )
         {
            write_solution = true;
            gmoModelStatSet(gmo, gmoModelStat_Integer);
            gevLogStat(gev, "User interrupt. Have feasible solution.");
         }
         else
         {
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            gevLogStat(gev, "User interrupt. No feasible solution found.");
         }
      } /* @todo check other primary and secondary status, e.g., for solution limit */
      else
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         if( model->bestSolution() )
         {
            write_solution = true;
            gmoModelStatSet(gmo, gmoModelStat_Integer);
            gevLogStat(gev, "Model status unknown, but have feasible solution.");
         }
         else
         {
            gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
            gevLogStat(gev, "Model status unknown, no feasible solution found.");
         }
         char buffer[255];
         sprintf(buffer, "CBC primary status: %d secondary status: %d\n", model->status(), model->secondaryStatus());
         gevLogStat(gev, buffer);
      }
   }

   if( write_solution && !gamsOsiStoreSolution(gmo, *model->solver()) )
      return false;

   if( !isLP() )
   {
      char buffer[255];
      if( model->bestSolution() )
      {
         if( nthreads > 1 )
            snprintf(buffer, 255, "MIP solution: %15.6e   (%d nodes, %.2f CPU seconds, %.2f wall clock seconds)\n", model->getObjValue(), model->getNodeCount(), cputime, walltime);
         else
            snprintf(buffer, 255, "MIP solution: %15.6e   (%d nodes, %g seconds)\n", model->getObjValue(), model->getNodeCount(), cputime);
         gevLogStat(gev, buffer);
      }
      snprintf(buffer, 255, "Best possible: %14.6e", model->getBestPossibleObjValue());
      gevLogStat(gev, buffer);
      if( model->bestSolution() )
      {
         snprintf(buffer, 255, "Absolute gap: %15.6e   (absolute tolerance optca: %g)", gmoGetAbsoluteGap(gmo), optca);
         gevLogStat(gev, buffer);
         snprintf(buffer, 255, "Relative gap: %15.6e   (relative tolerance optcr: %g)", gmoGetRelativeGap(gmo), optcr);
         gevLogStat(gev, buffer);
      }
   }

   if( dumpsolutions != NULL && model->numberSavedSolutions() > 1 )
   {
      char buffer[255];
      gdxHandle_t gdx;
      int rc;

      if( !gdxGetReady(buffer, sizeof(buffer)) || !gdxCreate(&gdx, buffer, sizeof(buffer)) )
      {
         gevLogStatPChar(gev, "failed to load GDX I/O library: ");
         gevLogStat(gev, buffer);
         return false;
      }

      snprintf(buffer, 255, "\nDumping %d alternate solutions:\n", model->numberSavedSolutions()-1);
      gevLogPChar(gev, buffer);
      /* create index GDX file */
      if( gdxOpenWrite(gdx, dumpsolutions, "CBC DumpSolutions Index File", &rc) == 0 )
      {
         rc = gdxGetLastError(gdx);
         gdxErrorStr(gdx, rc, buffer);
         gevLogStatPChar(gev, "problem writing GDX file: ");
         gevLogStat(gev, buffer);
      }
      else
      {
         gdxStrIndexPtrs_t keys;
         gdxStrIndex_t     keysX;
         gdxValues_t       vals;
         int sloc;
         int i;

         /* create index file */
         GDXSTRINDEXPTRS_INIT(keysX, keys);
         gdxDataWriteStrStart(gdx, "index", "Dumpsolutions index", 1, dt_set, 0);
         for( i = 1; i < model->numberSavedSolutions(); ++i)
         {
            snprintf(buffer, 255, "soln_cbc_p%d.gdx", i);
            gdxAddSetText(gdx, buffer, &sloc);
            snprintf(keys[0], GMS_SSSIZE, "file%d", i);
            vals[GMS_VAL_LEVEL] = sloc;
            gdxDataWriteStr(gdx, const_cast<const char**>(keys), vals);
         }
         gdxDataWriteDone(gdx);
         gdxClose(gdx);

         /* create point files */
         for( i = 1; i < model->numberSavedSolutions(); ++i)
         {
            snprintf(buffer, 255, "soln_cbc_p%d.gdx", i);
            gmoSetVarL(gmo, model->savedSolution(i));
            if( gmoUnloadSolutionGDX(gmo, buffer, 0, 1, 0) )
            {
               gevLogStat(gev, "Problems creating point file.\n");
            }
            else
            {
               gevLogPChar(gev, "Created point file ");
               gevLog(gev, buffer);
            }
         }
      }

      gdxFree(&gdx);
      gdxLibraryUnload();
   }
   else if( dumpsolutions != NULL && model->numberSavedSolutions() == 1 )
   {
      gevLog(gev, "Only one solution found, skip dumping alternate solutions.");
   }

   if( dumpsolutionsmerged != NULL && model->numberSavedSolutions() > 1 )
   {
      char buffer[GMS_SSSIZE];
      int solnvarsym;

      if( gmoCheckSolPoolUEL(gmo, "soln_cbc_p", &solnvarsym) )
      {
         gevLogStatPChar(gev, "Solution pool scenario label 'soln_cbc_p' contained in model dictionary. Cannot dump merged solutions pool.\n");
      }
      else
      {
         void* handle;

         handle = gmoPrepareSolPoolMerge(gmo, dumpsolutionsmerged, model->numberSavedSolutions()-1, "soln_cbc_p");
         if( handle != NULL )
         {
            for( int k = 0; k < solnvarsym; k++ )
            {
               gmoPrepareSolPoolNextSym(gmo, handle);
               for( int i = 1; i < model->numberSavedSolutions(); ++i )
               {
                  gmoSetVarL(gmo, model->savedSolution(i));
                  if( gmoUnloadSolPoolSolution (gmo, handle, i-1) )
                  {
                     snprintf(buffer, GMS_SSSIZE, "Problems unloading solution point %d symbol %d\n", i, k);
                     gevLogStatPChar(gev, buffer);
                  }
               }
            }
            if( gmoFinalizeSolPoolMerge(gmo, handle) )
            {
               gevLogStatPChar(gev, "Problems finalizing merged solution pool\n");
            }
         }
         else
         {
            gevLogStatPChar(gev, "Problems preparing merged solution pool\n");
         }
      }
   }
   else if( dumpsolutionsmerged != NULL && model->numberSavedSolutions() == 1 )
   {
      gevLog(gev, "Only one solution found, skip dumping alternate solutions.");
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

   return true;
}

bool GamsCbc::isLP()
{
   if( gmoModelType(gmo) == gmoProc_lp )
      return true;
   if( gmoModelType(gmo) == gmoProc_rmip )
      return true;
   if( gmoNDisc(gmo) )
      return false;
   int numSos1, numSos2, nzSos;
   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
   if( nzSos )
      return false;
   return true;
}

#define GAMSSOLVER_ID         cbc
#include "GamsEntryPoints_tpl.c"

DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Initialize)(void)
{
// assuming that CBC was build without CPLEX (or the CPLEX feature not enabled)
//#if defined(__linux) && defined(GAMSLINKS_HAS_CPLEX)
//   CPXinitialize();
//#endif

   gmoInitMutexes();
   gevInitMutexes();
   optInitMutexes();
   gdxInitMutexes();
   palInitMutexes();
}

#ifdef GAMS_BUILD
extern "C" void mkl_finalize(void);
#endif
DllExport void STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,Finalize)(void)
{
//#if defined(__linux) && defined(GAMSLINKS_HAS_CPLEX)
//   CPXfinalize();
//#endif

   gmoFiniMutexes();
   gevFiniMutexes();
   optFiniMutexes();
   gdxFiniMutexes();
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

   if( !optGetReady(msgBuf, msgBufLen) )
      return 0;

   if( !cfgGetReady(msgBuf, msgBufLen) )
      return 0;

   *Cptr = (void*) new GamsCbc();
   if( *Cptr == NULL )
   {
      snprintf(msgBuf, msgBufLen, "Out of memory when creating GamsCbc object.\n");
      if( msgBufLen > 0 )
         msgBuf[msgBufLen] = '\0';
      return 0;
   }

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,free)(void** Cptr)
{
   assert(Cptr != NULL);

   delete (GamsCbc*)*Cptr;
   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   palLibraryUnload();
   optLibraryUnload();
   if( gdxLibraryLoaded() )
      gdxLibraryUnload();
   cfgLibraryUnload();

   return 1;
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,CallSolver)(void* Cptr)
{
   assert(Cptr != NULL);
   return ((GamsCbc*)Cptr)->callSolver();
}

DllExport int STDCALL GAMSSOLVER_CONCAT(GAMSSOLVER_ID,ReadyAPI)(void* Cptr, gmoHandle_t Gptr, optHandle_t Optr)
{
   assert(Cptr != NULL);
   assert(Gptr != NULL);

   return ((GamsCbc*)Cptr)->readyAPI(Gptr, Optr);
}
