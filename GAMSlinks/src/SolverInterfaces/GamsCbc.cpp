// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsCbc.hpp"

#include <cstdlib>
#include <cstring>
#include <list>
#include <string>

#include "gclgms.h"
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gmspal.h"  /* for audit line */
#endif

#include "GamsCompatibility.h"

#include "GAMSlinksConfig.h"
#include "GamsOptions.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsOsiHelper.hpp"
#include "GamsCbcHeurBBTrace.hpp"

// For Branch and bound
#include "CbcConfig.h"
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp"  // for CbcSOS
#include "CbcBranchLotsize.hpp" // for CbcLotsize

#include "OsiClpSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"

GamsCbc::~GamsCbc()
{
   delete model;
   delete msghandler;
   for( int i = 0; i < cbc_argc; ++i )
      free(cbc_args[i]);
   delete[] cbc_args;

   free(writemps);
}

int GamsCbc::readyAPI(
   struct gmoRec*     gmo_,               /**< GAMS modeling object */
   struct optRec*     opt_                /**< GAMS options object */
)
{
   gmo = gmo_;
   assert(gmo != NULL);
   opt = opt_;

   delete model;

   if( getGmoReady() )
      return 1;

   if( getGevReady() )
      return 1;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

#ifdef GAMS_BUILD
#include "coinlibdCL1svn.h"
   char buffer[512];
   auditGetLine(buffer, sizeof(buffer));
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);
#endif

   gevLogStatPChar(gev, "\nCOIN-OR Branch and Cut (CBC Library "CBC_VERSION")\nwritten by J. Forrest\n");

   if( msghandler == NULL )
   {
      msghandler = new GamsMessageHandler(gev);
      msghandler->setLogLevel(0,1);
      msghandler->setLogLevel(2,0);
      msghandler->setLogLevel(3,0);
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

   /* initialize Cbc, I guess */
   CbcMain0(*model);

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

   GAMS_BBTRACE* bbtrace = NULL;
   if( miptrace != NULL && miptrace[0] )
   {
      int rc;
      rc = GAMSbbtraceCreate(&bbtrace, miptrace, "CBC "CBC_VERSION, model->getInfinity(), miptracenodefreq, miptracetimefreq);
      if( rc != 0 )
      {
         gevLogStat(gev, "Initializing miptrace failed.");
         GAMSbbtraceFree(&bbtrace);
      }
      else
      {
         GamsCbcHeurBBTrace traceheur(bbtrace);
         model->addHeuristic(&traceheur, "MIPtrace writing heurisitc", -1);
      }
   }

   gevLogStat(gev, "\nCalling CBC main solution routine...");

   double start_cputime  = CoinCpuTime();
   double start_walltime = CoinWallclockTime();

   CbcMain1(cbc_argc, const_cast<const char**>(cbc_args), *model);

   double end_cputime  = CoinCpuTime();
   double end_walltime = CoinWallclockTime();

   if( bbtrace != NULL )
   {
      GAMSbbtraceAddEndLine(bbtrace, model->getNodeCount(),
         model->getBestPossibleObjValue(),
         model->getSolutionCount() > 0 ? model->getObjValue() : model->getObjSense() * model->getInfinity());
      GAMSbbtraceFree(&bbtrace);
   }

   writeSolution(end_cputime - start_cputime, end_walltime - start_walltime);

   // solve again with fixed noncontinuous variables and original bounds on continuous variables
   if( !isLP() && model->bestSolution() && solvefinal )
   {
      gevLog(gev, "\nResolve with fixed discrete variables.");

      double* varlow = new double[gmoN(gmo)];
      double* varup  = new double[gmoN(gmo)];
      gmoGetVarLower(gmo, varlow);
      gmoGetVarUpper(gmo, varup);
      for( int i = 0; i < gmoN(gmo); ++i )
         if( (enum gmoVarType)gmoGetVarTypeOne(gmo, i) != gmovar_X )
            varlow[i] = varup[i] = model->bestSolution()[i];
      model->solver()->setColLower(varlow);
      model->solver()->setColUpper(varup);

      model->solver()->messageHandler()->setLogLevel(1, 1);
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

   msghandler->setLogLevel(1, isLP()); // we want LP output if we solve an LP
   model->passInMessageHandler(msghandler);

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

      for( int i = 0; i < numSos; ++i )
      {
         int k = 0;
         for( int j = sosbeg[i]; j < sosbeg[i+1]; ++j, ++k )
         {
            which[k]   = sosind[j];
            weights[k] = soswt[j];
            assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? gmovar_S1 : gmovar_S2));
         }
         sosobjects[i] = new CbcSOS(model, k, which, weights, i, sostype[i]);
         sosobjects[i]->setPriority(gmoN(gmo)-k); // branch on long sets first
      }

      model->addObjects(numSos, sosobjects);

      for( int i = 0; i < numSos; ++i )
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
         retcode = false;
         goto TERMINATE;
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

   GamsOptions options(gev, opt);

   if( opt == NULL )
   {
      // initialize options object by getting defaults from options settings file and optionally read user options file
      char* optfilename = NULL;
      if( gmoOptFile(gmo) > 0 )
      {
         char buffer[1024];
         gmoNameOptFile(gmo, buffer);
         optfilename = buffer;
      }
#ifdef GAMS_BUILD
      options.readOptionsFile("cbc", optfilename);
#else
      options.readOptionsFile("mycbc", optfilename);
#endif
   }

   // set GAMS options for those not set in options file
   if( !options.isDefined("reslim") )
      options.setDouble("reslim", gevGetDblOpt(gev, gevResLim));
   if( !options.isDefined("iterlim") && gevGetIntOpt(gev, gevIterLim) != ITERLIM_INFINITY )
      options.setInteger("iterlim", gevGetIntOpt(gev, gevIterLim));
   if( !options.isDefined("nodlim") && gevGetIntOpt(gev, gevNodeLim) > 0 )
      options.setInteger("nodlim", gevGetIntOpt(gev, gevNodeLim));
   if( !options.isDefined("nodelim") && options.isDefined("nodlim") )
      options.setInteger("nodelim", options.getInteger("nodlim"));
   if( !options.isDefined("optca") )
      options.setDouble("optca", gevGetDblOpt(gev, gevOptCA));
   if( !options.isDefined("optcr") )
      options.setDouble ("optcr", gevGetDblOpt(gev, gevOptCR));
   if( !options.isDefined("cutoff") && gevGetIntOpt(gev, gevUseCutOff) )
      options.setDouble("cutoff", gevGetDblOpt(gev, gevCutOff));
   if( !options.isDefined("increment") && gevGetIntOpt(gev, gevUseCheat) )
      options.setDouble("increment", gevGetDblOpt(gev, gevCheat));
   if( !options.isDefined("threads") )
      options.setInteger("threads", gevThreads(gev));

   //note: does not seem to work via Osi: OsiDoPresolveInInitial, OsiDoDualInInitial

   // Some tolerances and limits
   model->setDblParam(CbcModel::CbcMaximumSeconds,  options.getDouble("reslim"));
   model->solver()->setDblParam(OsiPrimalTolerance, options.getDouble("tol_primal"));
   model->solver()->setDblParam(OsiDualTolerance,   options.getDouble("tol_dual"));

   // iteration limit, if set
   if( options.isDefined("iterlim") )
      model->solver()->setIntParam(OsiMaxNumIteration, options.getInteger("iterlim"));

   // MIP parameters
   optca = options.getDouble("optca");
   optcr = options.getDouble("optcr");
   model->setIntParam(CbcModel::CbcMaxNumNode,           options.getInteger("nodelim"));
   model->setDblParam(CbcModel::CbcAllowableGap,         optca);
   model->setDblParam(CbcModel::CbcAllowableFractionGap, optcr);
   model->setDblParam(CbcModel::CbcIntegerTolerance,     options.getDouble ("tol_integer"));
   if( options.isDefined("cutoff") )
      model->setCutoff(model->solver()->getObjSense() * options.getDouble("cutoff")); // Cbc assumes a minimization problem here
   model->setPrintFrequency(options.getInteger("printfrequency"));

   std::list<std::string> par_list;

   char buffer[GMS_SSSIZE];
   std::map<std::string, std::string> stringenummap;

#define CHECKOPT2_INT(NAMEGAMS, NAMECBC) \
   if( options.isDefined(NAMEGAMS) ) \
   { \
      par_list.push_back("-"NAMECBC); \
      sprintf(buffer, "%d", options.getInteger(NAMEGAMS)); \
      par_list.push_back(buffer); \
   }

#define CHECKOPT_INT(NAME) CHECKOPT2_INT(NAME, NAME)

#define CHECKOPT2_DOUBLE(NAMEGAMS, NAMECBC) \
   if( options.isDefined(NAMEGAMS) ) \
   { \
      par_list.push_back("-"NAMECBC); \
      sprintf(buffer, "%g", options.getDouble(NAMEGAMS)); \
      par_list.push_back(buffer); \
   }

#define CHECKOPT_DOUBLE(NAME) CHECKOPT2_DOUBLE(NAME, NAME)

#define CHECKOPT2_BOOL(NAMEGAMS, NAMECBC) \
   if( options.isDefined(NAMEGAMS) ) \
   { \
      par_list.push_back("-"NAMECBC); \
      par_list.push_back(options.getBool(NAMEGAMS) ? "on" : "off"); \
   }

#define CHECKOPT_BOOL(NAME) CHECKOPT2_BOOL(NAME, NAME)

#define CHECKOPT2_STRINGENUM(NAMEGAMS, NAMECBC) \
   if( options.isDefined(NAMEGAMS) ) \
   { \
      char* value = options.getString(NAMEGAMS, buffer); \
      if( value == NULL ) \
      { \
         gevLogStatPChar(gev, "Cannot read value for option '"NAMEGAMS"'. Ignoring this option.\n"); \
      } \
      else \
      { \
         std::map<std::string, std::string>::iterator it(stringenummap.find(value)); \
         if( it == stringenummap.end() ) \
         { \
            gevLogStatPChar(gev, "Unsupported value for option '"NAMEGAMS"'. Ignoring this option.\n"); \
         } \
         else \
         { \
            par_list.push_back("-"NAMECBC); \
            par_list.push_back(it->second.length() > 0 ? it->second : it->first); \
         } \
      } \
   }

#define CHECKOPT_STRINGENUM(NAME) CHECKOPT2_STRINGENUM(NAME, NAME)

   // LP parameters
   CHECKOPT2_INT("idiotcrash",  "idiotCrash")
   CHECKOPT2_INT("sprintcrash", "sprintCrash")
   else
      CHECKOPT2_INT("sifting", "sprintCrash");

   stringenummap.clear();
   stringenummap["on"]  = "on";
   stringenummap["off"] = "off";
   stringenummap["solow_halim"] = "so";
   stringenummap["halim_solow"] = "ha";
   CHECKOPT_STRINGENUM("crash")

   CHECKOPT2_INT("maxfactor", "maxFactor")
   CHECKOPT_BOOL("crossover") // should be revised if we can do quadratic

   stringenummap.clear();
   stringenummap["auto"];
   stringenummap["dantzig"];
   stringenummap["partial"];
   stringenummap["steepest"];
   CHECKOPT2_STRINGENUM("dualpivot", "dualPivot")

   stringenummap.clear();
   stringenummap["auto"];
   stringenummap["exact"];
   stringenummap["dantzig"];
   stringenummap["partial"];
   stringenummap["steepest"];
   stringenummap["change"];
   stringenummap["sprint"];
   CHECKOPT2_STRINGENUM("primalpivot", "primalPivot")

   CHECKOPT2_BOOL("perturbation", "perturb")

   stringenummap.clear();
   stringenummap["auto"];
   stringenummap["off"];
   stringenummap["equilibrium"];
   stringenummap["geometric"];
   CHECKOPT_STRINGENUM("scaling")

   CHECKOPT_BOOL("presolve")
   CHECKOPT2_DOUBLE("tol_presolve", "preTolerance")
   CHECKOPT_INT("passpresolve")

   // MIP parameters

   CHECKOPT_INT("strategy")
   CHECKOPT2_INT("sollim", "maxSolutions")
   CHECKOPT2_INT("strongbranching", "strongBranching")
   CHECKOPT2_INT("trustpseudocosts", "trustPseudoCosts")
   CHECKOPT2_INT("cutdepth", "cutDepth")
   CHECKOPT2_INT("cut_passes_root", "passCuts")
   CHECKOPT2_INT("cut_passes_tree", "passTree")

   stringenummap.clear();
   stringenummap["off"];
   stringenummap["on"];
   stringenummap["root"];
   stringenummap["ifmove"];
   stringenummap["forceon"] = "forceOn";
   CHECKOPT2_STRINGENUM("cuts", "cutsOnOff")
   CHECKOPT2_STRINGENUM("cliquecuts", "cliqueCuts")
   CHECKOPT2_STRINGENUM("flowcovercuts", "flowCoverCuts")
   CHECKOPT2_STRINGENUM("gomorycuts", "gomoryCuts")
   CHECKOPT2_STRINGENUM("knapsackcuts", "knapsackCuts")
   CHECKOPT2_STRINGENUM("liftandprojectcuts", "liftAndProjectCuts")
   CHECKOPT2_STRINGENUM("mircuts", "mixedIntegerRoundingCuts")
   CHECKOPT2_STRINGENUM("reduceandsplitcuts", "reduceAndSplitCuts")
   CHECKOPT2_STRINGENUM("residualcapacitycuts", "residualCapacityCuts")
   CHECKOPT2_STRINGENUM("twomircuts", "twoMirCuts")

   stringenummap["forceonbut"] = "forceOnBut";
   stringenummap["forceonstrong"] = "forceOnStrong";
   stringenummap["forceonbutstrong"] = "forceOnButStrong";
   CHECKOPT2_STRINGENUM("probingcuts", "probingCuts")

   CHECKOPT2_BOOL("heuristics", "heuristicsOnOff")
   CHECKOPT2_BOOL("combinesolutions", "combineSolutions")
   CHECKOPT2_BOOL("dins", "Dins")
   CHECKOPT2_BOOL("divingrandom", "DivingSome")
   CHECKOPT2_BOOL("divingcoefficient", "DivingCoefficient")
   CHECKOPT2_BOOL("divingfractional", "DivingFractional")
   CHECKOPT2_BOOL("divingguided", "DivingGuided")
   CHECKOPT2_BOOL("divinglinesearch", "DivingLineSearch")
   CHECKOPT2_BOOL("divingpseudocost", "DivingPseudoCost")
   CHECKOPT2_BOOL("divingvectorlength", "DivingVectorLength")
   CHECKOPT2_BOOL("feaspump", "feasibilityPump")
   CHECKOPT2_INT("feaspump_passes", "passFeasibilityPump")
   CHECKOPT2_BOOL("localtreesearch", "localTreeSearch")
   CHECKOPT2_BOOL("naiveheuristics", "naiveHeuristics")
   CHECKOPT2_BOOL("pivotandfix", "pivotAndFix")
   CHECKOPT2_BOOL("randomizedrounding", "randomizedRounding")
   CHECKOPT2_BOOL("rens", "Rens")
   CHECKOPT2_BOOL("rins", "Rins")
   CHECKOPT2_BOOL("roundingheuristic", "roundingHeuristic")
   CHECKOPT_BOOL("vubheuristic");

   stringenummap.clear();
   stringenummap["on"];
   stringenummap["off"];
   stringenummap["root"];
   CHECKOPT2_STRINGENUM("greedyheuristic", "greedyHeuristic")

   stringenummap.clear();
   stringenummap["off"];
   stringenummap["priorities"];
   stringenummap["length"];
   stringenummap["columnorder"];
   stringenummap["binaryfirst"] = "01first";
   stringenummap["binarylast"] = "01last";
   CHECKOPT2_STRINGENUM("coststrategy", "costStrategy")

   stringenummap.clear();
   stringenummap["hybrid"];
   stringenummap["fewest"];
   stringenummap["depth"];
   stringenummap["upfewest"];
   stringenummap["downfewest"];
   stringenummap["updepth"];
   stringenummap["downdepth"];
   CHECKOPT2_STRINGENUM("nodestrategy", "nodeStrategy")

   stringenummap.clear();
   stringenummap["off"];
   stringenummap["on"];
   stringenummap["equal"];
   stringenummap["equalall"];
   stringenummap["sos"];
   stringenummap["trysos"];
   CHECKOPT_STRINGENUM("preprocess")
// should happen automatically now:
//   if (gmoGetVarTypeCnt(gmo, gmovar_SC) || gmoGetVarTypeCnt(gmo, gmovar_SI)) {
//      gevLogStat(gev, "CBC integer preprocessing does not handle semicontinuous variables correct, thus we switch it off.");
//      par_list.push_back("-preprocess");
//      par_list.push_back("off");
//   }

   CHECKOPT_DOUBLE("increment")

   nthreads = options.getInteger("threads");
   if( nthreads > 1 && CbcModel::haveMultiThreadSupport() )
   {
      par_list.push_back("-threads");
      sprintf(buffer, "%d", nthreads);
      par_list.push_back(buffer);

      // no Blas multithreading if Cbc is doing multithreading
      setNumThreadsBlas(gev, 1);
   }
   else
   {
      if( nthreads > 1 )
         gevLogStat(gev, "Warning: Multithreading support not available in CBC.");

      // allow Blas multithreading
      setNumThreadsBlas(gev, nthreads);
      nthreads = 1;
   }

   // special options set by user and passed unseen to CBC
   if( options.isDefined("special") )
   {
      char* value = options.getString("special", buffer);
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
   if( options.isDefined("startalg") )
   {
      char* value = options.getString("startalg", buffer);
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

#undef CHECKOPT_INT
#undef CHECKOPT2_INT
#undef CHECKOPT_DOUBLE
#undef CHECKOPT2_DOUBLE
#undef CHECKOPT_BOOL
#undef CHECKOPT2_BOOL
#undef CHECKOPT_STRINGENUM
#undef CHECKOPT2_STRINGENUM

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
      cbc_args[i] = strdup(it->c_str());
   cbc_args[i++] = strdup("-quit");

   mipstart = options.getBool("mipstart");
   solvefinal = options.getBool("solvefinal");

   // whether to write MPS file
   free(writemps);
   writemps = NULL;
   if( options.isDefined("writemps") )
   {
      options.getString("writemps", buffer);
      writemps = strdup(buffer);
   }

   // whether to write MIP trace file
   free(miptrace);
   miptrace = NULL;
   if( options.isDefined("miptrace") )
   {
      options.getString("miptrace", buffer);
      miptrace = strdup(buffer);
   }
   miptracenodefreq = options.getInteger("miptracenodefreq");
   miptracetimefreq = options.getDouble("miptracetimefreq");

   return true;
}

bool GamsCbc::writeSolution(
   double             cputime,            /**< CPU time spend by solver */
   double             walltime            /**< wallclock time spend by solver */
)
{
   bool write_solution = false;

   gevLogStat(gev, "");
   if( isLP() )
   {
      // if Cbc solved an LP, the solution status is not correctly stored in the CbcModel, we have to look into the solver
      if( model->solver()->isProvenDualInfeasible() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_UnboundedNoSolution);
         gevLogStat(gev, "Model unbounded.");
      }
      else if( model->solver()->isProvenPrimalInfeasible() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
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
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gevLogStat(gev, "Time limit reached.");
      }
      else if( model->solver()->isIterationLimitReached() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gevLogStat(gev, "Iteration limit reached.");
      }
      else if( model->solver()->isPrimalObjectiveLimitReached() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gevLogStat(gev, "Primal objective limit reached.");
      }
      else if( model->solver()->isDualObjectiveLimitReached() )
      {
         gmoSolveStatSet(gmo, gmoSolveStat_Solver);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
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
         if( optca > 0.0 || optcr > 0.0 )
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

   gmoSetHeadnTail(gmo, gmoHiterused, model->getIterationCount());
   gmoSetHeadnTail(gmo, gmoHresused,  nthreads > 1 ? walltime : cputime);
   gmoSetHeadnTail(gmo, gmoTmipbest,  model->getBestPossibleObjValue());
   gmoSetHeadnTail(gmo, gmoTmipnod,   model->getNodeCount());

   if( write_solution && !gamsOsiStoreSolution(gmo, *model->solver()) )
      return false;

   if( !isLP() )
   {
      char buffer[255];
      if( model->bestSolution() )
      {
         if( nthreads > 1 )
            snprintf(buffer, 255, "MIP solution: %15.6e   (%d nodes, %.2f CPU seconds, %.2f wall clock seconds)", model->getObjValue(), model->getNodeCount(), cputime, walltime);
         else
            snprintf(buffer, 255, "MIP solution: %15.6e   (%d nodes, %g seconds)", model->getObjValue(), model->getNodeCount(), cputime);
         gevLogStat(gev, buffer);
      }
      snprintf(buffer, 255, "Best possible: %14.6e", model->getBestPossibleObjValue());
      gevLogStat(gev, buffer);
      if( model->bestSolution() )
      {
         snprintf(buffer, 255, "Absolute gap: %15.6e   (absolute tolerance optca: %g)", CoinAbs(model->getObjValue() - model->getBestPossibleObjValue()), optca);
         gevLogStat(gev, buffer);
         snprintf(buffer, 255, "Relative gap: %14.6f%%   (relative tolerance optcr: %g%%)", 100* CoinAbs(model->getObjValue() - model->getBestPossibleObjValue()) / CoinMax(CoinAbs(model->getBestPossibleObjValue()), 1.), 100*optcr);
         gevLogStat(gev, buffer);
      }
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

#define GAMSSOLVERC_ID         cbc
#define GAMSSOLVERC_CLASS      GamsCbc
#include "GamsSolverC_tpl.cpp"
