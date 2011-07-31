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

#include "GamsOptions.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsOsiHelper.hpp"

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
      model->solver()->writeMps(writemps, "", model->solver()->getObjSense());
   }

	gevLogStat(gev, "\nCalling CBC main solution routine...");

	double start_cputime  = CoinCpuTime();
	double start_walltime = CoinWallclockTime();

	CbcMain1(cbc_argc, const_cast<const char**>(cbc_args), *model);

	double end_cputime  = CoinCpuTime();
	double end_walltime = CoinWallclockTime();

   writeSolution(end_cputime - start_cputime, end_walltime - start_walltime);

   // solve again with fixed noncontinuous variables and original bounds on continuous variables
   // TODO parameter to turn this off or do only if cbc reduced bounds
	if( !isLP() && model->bestSolution() )
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
   OsiClpSolverInterface solver;

   int* cbcprior = NULL;

   CbcObject** sosobjects = NULL;
   int numSos = 0;

   CbcObject** semiobjects = NULL;
   int numSemi = 0;

   gmoPinfSet(gmo,  solver.getInfinity());
   gmoMinfSet(gmo, -solver.getInfinity());
   gmoObjReformSet(gmo, 1);
   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoIndexBaseSet(gmo, 0);

	if( !gamsOsiLoadProblem(gmo, solver, true) )
		return false;

   // assemble integer variable branching priorities
	// range of priority values
	double minprior =  solver.getInfinity();
	double maxprior = -solver.getInfinity();
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
	      cbcprior = new int[gmoNDisc(gmo)];
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
	   }
	}

	// assemble SOS of type 1 or 2
	int numSos1, numSos2, nzSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	numSos = numSos1 + numSos2;
	if( nzSos > 0 )
	{
	   sosobjects = new CbcObject*[numSos];

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

	   delete[] which;
	   delete[] weights;
	   delete[] sostype;
	   delete[] sosbeg;
	   delete[] sosind;
	   delete[] soswt;
	}

	// assemble semicontinuous and semiinteger variables
	numSemi = gmoGetVarTypeCnt(gmo, gmovar_SC) + gmoGetVarTypeCnt(gmo, gmovar_SI);
	if( numSemi > 0)
	{
	   semiobjects = new CbcObject*[numSemi];
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
         solver.setColLower(i, 0.0);

	      ++object_nr;
	   }
	   assert(object_nr == numSemi);
	}

   if( gmoN(gmo) == 0 )
   {
      gevLog(gev, "Problem has no columns. Adding fake column...");
      CoinPackedVector vec(0);
      solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.0);
   }

   /* setup CbcModel */
	model = new CbcModel(solver);

	msghandler->setLogLevel(1, isLP()); /* we want LP output if we solve an LP */
	model->passInMessageHandler(msghandler);

	/* pass in branching priorities */
	model->passInPriorities(cbcprior, false);
	delete[] cbcprior;

	/* pass in SOS objects */
	model->addObjects(numSos, sosobjects);
	for( int i = 0; i < numSos; ++i )
	   delete sosobjects[i];
   delete[] sosobjects;

   /* pass in lotsize objects */
   model->addObjects(numSemi, semiobjects);
   for( int i = 0; i < numSemi; ++i )
      delete semiobjects[i];
   delete[] semiobjects;

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

	// LP parameters

	if (options.isDefined("idiotcrash")) {
		par_list.push_back("-idiotCrash");
		sprintf(buffer, "%d", options.getInteger("idiotcrash"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("sprintcrash")) {
		par_list.push_back("-sprintCrash");
		sprintf(buffer, "%d", options.getInteger("sprintcrash"));
		par_list.push_back(buffer);
	} else if (options.isDefined("sifting")) { // synonym for sprintCrash
		par_list.push_back("-sprintCrash");
		sprintf(buffer, "%d", options.getInteger("sifting"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("crash")) {
		char* value = options.getString("crash", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'crash'. Ignoring this option");
		}	else {
			if (strcmp(value, "off") == 0 || strcmp(value, "on") == 0) {
				par_list.push_back("-crash");
				par_list.push_back(value);
			} else if(strcmp(value, "solow_halim") == 0) {
				par_list.push_back("-crash");
				par_list.push_back("so");
			} else if(strcmp(value, "halim_solow") == 0) {
				par_list.push_back("-crash");
				par_list.push_back("ha");
			} else {
				gevLogStat(gev, "Unsupported value for option 'crash'. Ignoring this option");
			}
		}
	}

	if (options.isDefined("maxfactor")) {
		par_list.push_back("-maxFactor");
		sprintf(buffer, "%d", options.getInteger("maxfactor"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("crossover")) { // should be revised if we can do quadratic
		par_list.push_back("-crossover");
		par_list.push_back(options.getBool("crossover") ? "on" : "off");
	}

	if (options.isDefined("dualpivot")) {
		char* value = options.getString("dualpivot", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'dualpivot'. Ignoring this option");
		} else {
			if (strcmp(value, "auto")     == 0 ||
			    strcmp(value, "dantzig")  == 0 ||
			    strcmp(value, "partial")  == 0 ||
					strcmp(value, "steepest") == 0) {
				par_list.push_back("-dualPivot");
				par_list.push_back(value);
			} else {
				gevLogStat(gev, "Unsupported value for option 'dualpivot'. Ignoring this option");
			}
		}
	}

	if (options.isDefined("primalpivot")) {
		char* value = options.getString("primalpivot", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'primalpivot'. Ignoring this option");
		} else {
			if (strcmp(value, "auto")     == 0 ||
			    strcmp(value, "exact")    == 0 ||
			    strcmp(value, "dantzig")  == 0 ||
			    strcmp(value, "partial")  == 0 ||
					strcmp(value, "steepest") == 0 ||
					strcmp(value, "change")   == 0 ||
					strcmp(value, "sprint")   == 0) {
				par_list.push_back("-primalPivot");
				par_list.push_back(value);
			} else {
				gevLogStat(gev, "Unsupported value for option 'primalpivot'. Ignoring this option");
			}
		}
	}

	if (options.isDefined("perturbation")) {
		par_list.push_back("-perturb");
		par_list.push_back(options.getBool("perturbation") ? "on" : "off");
	}

	if (options.isDefined("scaling")) {
		char* value = options.getString("scaling", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'scaling'. Ignoring this option");
		} else if
			(strcmp(value, "auto")        == 0 ||
			 strcmp(value, "off")         == 0 ||
			 strcmp(value, "equilibrium") == 0 ||
			 strcmp(value, "geometric")   == 0) {
			par_list.push_back("-scaling");
			par_list.push_back(value);
		} else {
			gevLogStat(gev, "Unsupported value for option 'scaling'. Ignoring this option");
		}
	}

	if (options.isDefined("presolve")) {
		par_list.push_back("-presolve");
		par_list.push_back(options.getBool("presolve") ? "on" : "off");
	}

	if (options.isDefined("tol_presolve")) {
		par_list.push_back("-preTolerance");
		sprintf(buffer, "%g", options.getDouble("tol_presolve"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("passpresolve")) {
		par_list.push_back("-passpresolve");
		sprintf(buffer, "%d", options.getInteger("passpresolve"));
		par_list.push_back(buffer);
	}

	// MIP parameters

	if (options.isDefined("strategy")) {
		par_list.push_back("-strategy");
		sprintf(buffer, "%d", options.getInteger("strategy"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("sollim")) {
		par_list.push_back("-maxSolutions");
		sprintf(buffer, "%d", options.getInteger("sollim"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("strongbranching")) {
		par_list.push_back("-strongBranching");
		sprintf(buffer, "%d", options.getInteger("strongbranching"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("trustpseudocosts")) {
		par_list.push_back("-trustPseudoCosts");
		sprintf(buffer, "%d", options.getInteger("trustpseudocosts"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("cutdepth")) {
		par_list.push_back("-cutDepth");
		sprintf(buffer, "%d", options.getInteger("cutdepth"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("cut_passes_root")) {
		par_list.push_back("-passCuts");
		sprintf(buffer, "%d", options.getInteger("cut_passes_root"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("cut_passes_tree")) {
		par_list.push_back("-passTree");
		sprintf(buffer, "%d", options.getInteger("cut_passes_tree"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("cuts")) {
		char* value = options.getString("cuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'cuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-cutsOnOff");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-cutsOnOff");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'cuts'. Ignoring this option");
		}
	}

	if (options.isDefined("cliquecuts")) {
		char* value = options.getString("cliquecuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'cliquecuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0) {
			par_list.push_back("-cliqueCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-cliqueCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'cliquecuts'. Ignoring this option");
		}
	}

	if (options.isDefined("flowcovercuts")) {
		char* value = options.getString("flowcovercuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'flowcovercuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0) {
			par_list.push_back("-flowCoverCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-flowCoverCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'flowcovercuts'. Ignoring this option");
		}
	}

	if (options.isDefined("gomorycuts")) {
		char* value = options.getString("gomorycuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'gomorycuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-gomoryCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-gomoryCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'gomorycuts'. Ignoring this option");
		}
	}

	if (options.isDefined("knapsackcuts")) {
		char* value = options.getString("knapsackcuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'knapsackcuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-knapsackCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-knapsackCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'knapsackcuts'. Ignoring this option");
		}
	}

	if (options.isDefined("liftandprojectcuts")) {
		char* value = options.getString("liftandprojectcuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'liftandprojectcuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-liftAndProjectCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-liftAndProjectCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'liftandprojectcuts'. Ignoring this option");
		}
	}

	if (options.isDefined("mircuts")) {
		char* value = options.getString("mircuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'mircuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-mixedIntegerRoundingCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-mixedIntegerRoundingCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'mircuts'. Ignoring this option");
		}
	}

	if (options.isDefined("probingcuts")) {
		char* value = options.getString("probingcuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'probingcuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-probingCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOn");
		} else if (strcmp(value, "forceonbut") == 0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOnBut");
		} else if (strcmp(value, "forceonstrong") == 0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOnStrong");
		} else if (strcmp(value, "forceonbutstrong") == 0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOnButStrong");
		} else {
			gevLogStat(gev, "Unsupported value for option 'probingcuts'. Ignoring this option");
		}
	}

	if (options.isDefined("reduceandsplitcuts")) {
		char* value = options.getString("reduceandsplitcuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'reduceandsplitcuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-reduceAndSplitCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-reduceAndSplitCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'reduceandsplitcuts'. Ignoring this option");
		}
	}

	if (options.isDefined("residualcapacitycuts")) {
		char* value = options.getString("residualcapacitycuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'residualcapacitycuts'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")    == 0 ||
		     strcmp(value, "on")     == 0 ||
		     strcmp(value, "root")   == 0 ||
		     strcmp(value, "ifmove") == 0 ) {
			par_list.push_back("-residualCapacityCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon") == 0) {
			par_list.push_back("-residualCapacityCuts");
			par_list.push_back("forceOn");
		} else {
			gevLogStat(gev, "Unsupported value for option 'residualcapacitycuts'. Ignoring this option");
		}
	}

	if (options.isDefined("twomircuts")) {
		char* value = options.getString("twomircuts", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'twomircuts'. Ignoring this option");
		} else if
		  ( strcmp(value, "off")     == 0 ||
		    strcmp(value, "on")      == 0 ||
		    strcmp(value, "root")    == 0 ||
		    strcmp(value, "ifmove")  == 0 ||
		    strcmp(value, "forceon") == 0 ) {
			par_list.push_back("-twoMirCuts");
			par_list.push_back(value);
		} else {
			gevLogStat(gev, "Unsupported value for option 'twomircuts'. Ignoring this option");
		}
	}

	if (options.isDefined("heuristics")) {
		par_list.push_back("-heuristicsOnOff");
		par_list.push_back(options.getBool("heuristics") ? "on" : "off");
	}

	if (options.isDefined("combinesolutions")) {
		par_list.push_back("-combineSolution");
		par_list.push_back(options.getBool("combinesolutions") ? "on" : "off");
	}

	if (options.isDefined("dins")) {
		par_list.push_back("-Dins");
		par_list.push_back(options.getBool("dins") ? "on" : "off");
	}

	if (options.isDefined("divingrandom")) {
		par_list.push_back("-DivingSome");
		par_list.push_back(options.getBool("divingrandom") ? "on" : "off");
	}
	if (options.isDefined("divingcoefficient")) {
		par_list.push_back("-DivingCoefficient");
		par_list.push_back(options.getBool("divingcoefficient") ? "on" : "off");
	}
	if (options.isDefined("divingfractional")) {
		par_list.push_back("-DivingFractional");
		par_list.push_back(options.getBool("divingfractional") ? "on" : "off");
	}
	if (options.isDefined("divingguided")) {
		par_list.push_back("-DivingGuided");
		par_list.push_back(options.getBool("divingguided") ? "on" : "off");
	}
	if (options.isDefined("divinglinesearch")) {
		par_list.push_back("-DivingLineSearch");
		par_list.push_back(options.getBool("divinglinesearch") ? "on" : "off");
	}
	if (options.isDefined("divingpseudocost")) {
		par_list.push_back("-DivingPseudoCost");
		par_list.push_back(options.getBool("divingpseudocost") ? "on" : "off");
	}
	if (options.isDefined("divingvectorlength")) {
		par_list.push_back("-DivingVectorLength");
		par_list.push_back(options.getBool("divingvectorlength") ? "on" : "off");
	}

	if (options.isDefined("feaspump")) {
		par_list.push_back("-feasibilityPump");
		par_list.push_back(options.getBool("feaspump") ? "on" : "off");
	}

	if (options.isDefined("feaspump_passes")) {
		par_list.push_back("-passFeasibilityPump");
		sprintf(buffer, "%d", options.getInteger("feaspump_passes"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("greedyheuristic")) {
		char* value = options.getString("greedyheuristic", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'greedyheuristic'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")  == 0 ||
		     strcmp(value, "on")   == 0 ||
		     strcmp(value, "root") == 0 ) {
			par_list.push_back("-greedyHeuristic");
			par_list.push_back(value);
		} else {
			gevLogStat(gev, "Unsupported value for option 'greedyheuristic'. Ignoring this option");
		}
	}

	if (options.isDefined("localtreesearch")) {
		par_list.push_back("-localTreeSearch");
		par_list.push_back(options.getBool("localtreesearch") ? "on" : "off");
	}

	if (options.isDefined("naiveheuristics")) {
		par_list.push_back("-naiveHeuristics");
		par_list.push_back(options.getBool("naiveheuristics") ? "on" : "off");
	}

	if (options.isDefined("pivotandfix")) {
		par_list.push_back("-pivotAndFix");
		par_list.push_back(options.getBool("pivotandfix") ? "on" : "off");
	}

	if (options.isDefined("randomizedrounding")) {
		par_list.push_back("-randomizedRounding");
		par_list.push_back(options.getBool("randomizedrounding") ? "on" : "off");
	}

	if (options.isDefined("rens")) {
		par_list.push_back("-Rens");
		par_list.push_back(options.getBool("rens") ? "on" : "off");
	}

	if (options.isDefined("rins")) {
		par_list.push_back("-Rins");
		par_list.push_back(options.getBool("rins") ? "on" : "off");
	}

	if (options.isDefined("roundingheuristic")) {
		par_list.push_back("-roundingHeuristic");
		par_list.push_back(options.getBool("roundingheuristic") ? "on" : "off");
	}

	if (options.isDefined("vubheuristic")) {
		par_list.push_back("-vubheuristic");
		sprintf(buffer, "%d", options.getInteger("vubheuristic"));
		par_list.push_back(buffer);
	}

	if (options.isDefined("coststrategy")) {
		char* value = options.getString("coststrategy", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'coststrategy'. Ignoring this option");
		} else if
		   ( strcmp(value, "off")         == 0 ||
		     strcmp(value, "priorities")  == 0 ||
		     strcmp(value, "length")      == 0 ||
		     strcmp(value, "columnorder") == 0 ) {
			par_list.push_back("-costStrategy");
			par_list.push_back(value);
		} else if (strcmp(value, "binaryfirst") == 0) {
			par_list.push_back("-costStrategy");
			par_list.push_back("01first");
		} else if (strcmp(value, "binarylast") == 0) {
			par_list.push_back("-costStrategy");
			par_list.push_back("01last");
		} else {
			gevLogStat(gev, "Unsupported value for option 'coststrategy'. Ignoring this option");
		}
	}

	if (options.isDefined("nodestrategy")) {
		char* value=options.getString("nodestrategy", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'nodestrategy'. Ignoring this option");
		} else if
		   ( strcmp(value, "hybrid")     == 0 ||
		     strcmp(value, "fewest")     == 0 ||
		     strcmp(value, "depth")      == 0 ||
		     strcmp(value, "upfewest")   == 0 ||
		     strcmp(value, "downfewest") == 0 ||
		     strcmp(value, "updepth")    == 0 ||
		     strcmp(value, "downdepth")  == 0 ) {
			par_list.push_back("-nodeStrategy");
			par_list.push_back(value);
		} else {
			gevLogStat(gev, "Unsupported value for option 'nodestrategy'. Ignoring this option");
		}
	}

	if (options.isDefined("preprocess")) {
		char* value = options.getString("preprocess", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'preprocess'. Ignoring this option");
		} else if
		  ( strcmp(value, "off")      == 0 ||
		    strcmp(value, "on")       == 0 ||
		    strcmp(value, "equal")    == 0 ||
		    strcmp(value, "equalall") == 0 ||
		    strcmp(value, "sos")      == 0 ||
		    strcmp(value, "trysos")   == 0 ) {
			par_list.push_back("-preprocess");
			par_list.push_back(value);
		} else {
			gevLogStat(gev, "Unsupported value for option 'preprocess'. Ignoring this option");
		}
	} else if (gmoGetVarTypeCnt(gmo, gmovar_SC) || gmoGetVarTypeCnt(gmo, gmovar_SI)) {
		gevLogStat(gev, "CBC integer preprocessing does not handle semicontinuous variables correct, thus we switch it off.");
		par_list.push_back("-preprocess");
		par_list.push_back("off");
	}

	if (options.isDefined("increment")) {
		par_list.push_back("-increment");
		sprintf(buffer, "%g", options.getDouble("increment"));
		par_list.push_back(buffer);
	}

   nthreads = options.getInteger("threads");
	if( nthreads > 1 ) {
	   par_list.push_back("-threads");
	   sprintf(buffer, "%d", nthreads);
	   par_list.push_back(buffer);

	   // TODO print warning if Cbc compiled without multithreading support (how to know?)
	   //  gevLogStat(gev, "Warning: CBC has not been compiled with multithreading support. Option threads ignored.");

	   // no Blas multithreading if Cbc is doing multithreading
	   setNumThreadsBlas(gev, 1);
	}
	else
	{
	   // allow Blas multithreading
	   setNumThreadsBlas(gev, gevThreads(gev));
	}

	// special options set by user and passed unseen to CBC
	if (options.isDefined("special")) {
		char longbuffer[10000];
		char* value = options.getString("special", longbuffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'special'. Ignoring this option");
		} else {
			char* tok = strtok(value, " ");
			while (tok) {
				par_list.push_back(tok);
				tok = strtok(NULL, " ");
			}
		}
	}

	// algorithm for root node and solve command

	if (options.isDefined("startalg")) {
		char* value = options.getString("startalg", buffer);
		if (!value) {
			gevLogStat(gev, "Cannot read value for option 'startalg'. Ignoring this option");
		} else if (strcmp(value, "primal")  == 0) {
			par_list.push_back("-primalSimplex");
		} else if (strcmp(value, "dual")    == 0) {
			par_list.push_back("-dualSimplex");
		} else if (strcmp(value, "barrier") == 0) {
			par_list.push_back("-barrier");
		} else {
			gevLogStat(gev, "Unsupported value for option 'startalg'. Ignoring this option");
		}
		if (!isLP())
			par_list.push_back("-solve");
	} else
		par_list.push_back("-solve");

	size_t par_list_length = par_list.size();
	if( cbc_args != NULL ) {
      for (int i = 0; i < cbc_argc; ++i)
         free(cbc_args[i]);
      delete[] cbc_args;
	}
	cbc_argc = (int)par_list_length+2;
	cbc_args = new char*[cbc_argc];
	cbc_args[0] = strdup("GAMS/CBC");
	int i = 1;
	for (std::list<std::string>::iterator it(par_list.begin()); it != par_list.end(); ++it, ++i)
		cbc_args[i] = strdup(it->c_str());
	cbc_args[i++] = strdup("-quit");

   mipstart = options.getBool("mipstart");

   // whether to write MPS file
   free(writemps);
   writemps = NULL;
   if( options.isDefined("writemps") )
   {
      char buffer[GMS_SSSIZE];
      options.getString("writemps", buffer);
      writemps = strdup(buffer);
   }

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
