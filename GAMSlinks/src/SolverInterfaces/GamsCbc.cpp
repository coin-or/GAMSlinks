// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsCbc.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

// some STD templates to simplify Johns parameter handling for us
#include <list>
#include <string>

// GAMS
extern "C" {
#include "gmocc.h"
}
#include "GamsMessageHandler.hpp"
#include "GamsOsiHelper.hpp"

// For Branch and bound
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp" //for CbcSOS
#include "CbcBranchLotsize.hpp" //for CbcLotsize

#include "OsiClpSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"


GamsCbc::GamsCbc()
: gmo(NULL), msghandler(NULL), model(NULL), cbc_argc(0), cbc_args(NULL)
{
#ifdef GAMS_BUILD
	strcpy(cbc_message, "GAMS/CoinCbcD (CBC Library 2.2)\nwritten by J. Forrest\n");
#else
	strcpy(cbc_message, "GAMS/CbcD (CBC Library 2.2)\nwritten by J. Forrest\n");
#endif
}

GamsCbc::~GamsCbc() {
	delete model;
	delete msghandler;
	for (int i = 0; i < cbc_argc; ++i)
		free(cbc_args[i]);
	delete[] cbc_args;
}

int GamsCbc::readyAPI(struct gmoRec* gmo_, struct optRec* opt, struct gcdRec* gcd) {
	gmo = gmo_;
	assert(gmo);
	assert(!model);
	
	options.setGMO(gmo);
	if (opt) {
		options.setOpt(opt);
	} else {
		char buffer[1024];
		gmoNameOptFile(gmo, buffer);
#ifdef GAMS_BUILD	
		options.readOptionsFile("coincbcd", gmoOptFile(gmo) ? buffer : NULL);
#else
		options.readOptionsFile("cbcd",     gmoOptFile(gmo) ? buffer : NULL);
#endif
	}
	if (!options.isDefined("reslim"))  options.setDouble ("reslim",  gmoResLim(gmo));
	if (!options.isDefined("iterlim")) options.setInteger("iterlim", gmoIterLim(gmo));
	if (!options.isDefined("nodlim") && gmoNodeLim(gmo))  options.setInteger("nodlim", gmoNodeLim(gmo));
	if (!options.isDefined("nodelim") && options.isDefined("nodlim")) options.setInteger("nodelim", options.getInteger("nodlim"));
	if (!options.isDefined("optca"))   options.setDouble ("optca",   gmoOptCA(gmo));
	if (!options.isDefined("optcr"))   options.setDouble ("optcr",   gmoOptCR(gmo));
	if (!options.isDefined("cutoff")    && gmoUseCutOff(gmo)) options.setDouble("cutoff",    gmoCutOff(gmo));
	if (!options.isDefined("increment") && gmoUseCheat (gmo)) options.setDouble("increment", gmoCheat (gmo));

	dict.setGMO(gmo);
	if (options.getBool("names")) {
		if (gcd)
			dict.setGCD(gcd);
		else
			dict.readDictionary();
	}

	OsiClpSolverInterface solver;

	gmoPinfSet(gmo,  solver.getInfinity());
	gmoMinfSet(gmo, -solver.getInfinity());
	gmoObjReformSet(gmo, 1);
	gmoObjStyleSet(gmo, ObjType_Fun);
  gmoIndexBaseSet(gmo, 0);
	
	msghandler = new GamsMessageHandler(gmo);
	solver.passInMessageHandler(msghandler);
	solver.setHintParam(OsiDoReducePrint, true, OsiHintTry);

	if (!setupProblem(solver)) {
		gmoLogStat(gmo, "Error setting up problem...");
		return -1;
	}

	// write MPS file
	if (options.isDefined("writemps")) {
		char buffer[1024];
		options.getString("writemps", buffer);
		gmoLogStatPChar(gmo, "\nWriting MPS file ");
		gmoLogStat(gmo, buffer);
  	solver.writeMps(buffer, "", (gmoSense(gmo) == Obj_Min) ? 1.0 : -1.0);
	}
	
	model = new CbcModel(solver);
	model->passInMessageHandler(msghandler);

	CbcMain0(*model);
  // Switch off most output
	model->solver()->setHintParam(OsiDoReducePrint, true, OsiHintTry);

	if (gmoN(gmo)) {
		if (!setupPrioritiesSOSSemiCon()) {
			gmoLogStat(gmo, "Error setting up priorities, SOS, or semicontinuous variables.");
			return -1;
		}
		if (!setupStartingPoint()) {
			gmoLogStat(gmo, "Error setting up starting point.");
			return -1;
		}
	}
	if (!setupParameters()) {
		gmoLogStat(gmo, "Error setting up CBC parameters.");
		return -1;
	}

	return 1;
}
	
//int GamsCbc::haveModifyProblem() {
//	return 0;
//}
//	
//int GamsCbc::modifyProblem() {
//	assert(false);
//	return -1;
//}
	
int GamsCbc::callSolver() {
	gmoLogStat(gmo, "\nCalling CBC main solution routine...");
	
	//TODO? reset iteration and time counter in Cbc?
	//TODO? was user allowed to change options between readyAPI and callSolver?

	double start_time = CoinCpuTime();
//	CoinWallclockTime(); // start wallclock timer
	
//	myout.setCurrentDetail(2);
	CbcMain1(cbc_argc, const_cast<const char**>(cbc_args), *model);
	
	double end_time = CoinCpuTime();

	double* varlow = NULL;
	double* varup  = NULL;
	if (!isLP() && model->bestSolution()) { // solve again with fixed noncontinuous variables and original bounds on continuous variables
		//TODO parameter to turn this off or do only if cbc reduced bounds
		gmoLog(gmo, "Resolve with fixed noncontinuous variables.");
		varlow = new double[gmoN(gmo)];
		varup  = new double[gmoN(gmo)];
		gmoGetVarLower(gmo, varlow);
		gmoGetVarUpper(gmo, varup);
		for (int i = 0; i < gmoN(gmo); ++i)
			if ((enum gmoVarType)gmoGetVarTypeOne(gmo, i) != var_X)
				varlow[i] = varup[i] = model->bestSolution()[i];
		model->solver()->setColLower(varlow);
		model->solver()->setColUpper(varup);
		
		model->solver()->messageHandler()->setLogLevel(1);
		model->solver()->resolve();
		if (!model->solver()->isProvenOptimal()) {
			gmoLog(gmo, "Resolve failed, values for dual variables will be unavailable.");
			model->solver()->setColSolution(model->bestSolution());
		}
	}

	writeSolution(end_time - start_time);

	if (!isLP() && model->bestSolution()) { // reset original bounds
		gmoGetVarLower(gmo, varlow);
		gmoGetVarUpper(gmo, varup);
		model->solver()->setColLower(varlow);
		model->solver()->setColUpper(varup);
	}
	delete[] varlow;
	delete[] varup;

	return 1;
}

bool GamsCbc::setupProblem(OsiSolverInterface& solver) {
	if (!gamsOsiLoadProblem(gmo, solver))
		return false;

	// Cbc thinks in terms of lotsize objects, and for those the lower bound has to be 0
	//TODO do this only if there are semicontinuous or semiinteger variables
	for (int i = 0; i < gmoN(gmo); ++i)
		if (gmoGetVarTypeOne(gmo, i) == var_SC || gmoGetVarTypeOne(gmo, i) == var_SI)
			solver.setColLower(i, 0.);

	if (!gmoN(gmo)) {
		gmoLog(gmo, "Problem has no columns. Adding fake column...");
		CoinPackedVector vec(0);
		solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.);
	}
	
	if (dict.haveNames()) { // set variable and constraint names
		solver.setIntParam(OsiNameDiscipline, 2);
		char buffer[255];
		std::string stbuffer;
		for (int j = 0; j < gmoN(gmo); ++j)
			if (dict.getColName(j, buffer, 255)) {
				stbuffer = buffer;
				solver.setColName(j, stbuffer);
			}
		for (int j = 0; j < gmoM(gmo); ++j)
			if (dict.getRowName(j, buffer, 255)) {
				stbuffer = buffer;
				solver.setRowName(j, stbuffer);
			}
	}

	return true;
}

bool GamsCbc::setupPrioritiesSOSSemiCon() {
	assert(model);

#if 0 //TODO gmo does not seem to have variable branching priorities yet
	// range of priority values
	double minprior =  model->solver()->getInfinity();
	double maxprior = -model->solver()->getInfinity();
	// take care of integer variable branching priorities
	if (false) { //TODO get priority flag
		// first check which range of priorities is given
		for (int i = 0; i < gmoN(gmo); ++i) {
			if (gmoVarTypeOne(gmo, i) == var_X) continue; // gams forbids branching priorities for continuous variables
			if (gm.ColPriority()[i] < minprior) minprior = gm.ColPriority()[i];
			if (gm.ColPriority()[i] > maxprior) maxprior = gm.ColPriority()[i];
		}
		if (minprior!=maxprior) {
			int* cbcprior=new int[gm.nDCols()];
			for (int i=0; i<gm.nDCols(); ++i) {
				// we map gams priorities into the range {1,..,1000}
				// (1000 is standard priority in Cbc, and 1 is highest priority)
				cbcprior[i]=1+(int)(999*(gm.ColPriority()[i]-minprior)/(maxprior-minprior));
			}
			model.passInPriorities(cbcprior, false);
			delete[] cbcprior;
		}
	}
#endif

	// Tell solver which variables belong to SOS of type 1 or 2
	int numSos1, numSos2, nzSos, numSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	numSos = numSos1 + numSos2;
	if (nzSos) {
		CbcObject** objects = new CbcObject*[numSos];
		
		int* sostype = new int[numSos];
		int* sosbeg  = new int[numSos+1];
		int* sosind  = new int[nzSos];
		double* soswt = new double[nzSos];
		
		gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);
		
		int*    which   = new int[CoinMin(gmoN(gmo), nzSos)];
		double* weights = new double[CoinMin(gmoN(gmo), nzSos)];
		
		for (int i = 0; i < numSos; ++i) {
			int k = 0;
			for (int j = sosbeg[i]; j < sosbeg[i+1]; ++j, ++k) {
				which[k]   = sosind[j];
				weights[k] = soswt[j];
				assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? var_S1 : var_S2));
			}
			objects[i] = new CbcSOS(model, k, which, weights, i, sostype[i]);
			objects[i]->setPriority(gmoN(gmo)-k); // branch on long sets first
		}
		
		delete[] which;
		delete[] weights;
		
	  model->addObjects(numSos, objects);
	  for (int i = 0; i < numSos; ++i)
			delete objects[i];
		delete[] objects;
  }
	
	int numSemi = 0; // number of semicontinuous and semiinteger variables
	for (int i = 0; i < gmoN(gmo); ++i) // TODO there should be a gmo function to get this count
		if (gmoGetVarTypeOne(gmo, i) == var_SC || gmoGetVarTypeOne(gmo, i) == var_SI)
			++numSemi;
  
  if (numSemi) {
		CbcObject** objects = new CbcObject*[numSemi];
		int object_nr = 0;
		double points[4];
		points[0] = 0.;
		points[1] = 0.;
		for (int i = 0; i < gmoN(gmo); ++i) {
			enum gmoVarType vartype = (enum gmoVarType)gmoGetVarTypeOne(gmo, i);
			if (vartype != var_SC && vartype != var_SI)
				continue;
			
			double varlb = gmoGetVarLowerOne(gmo, i);
			double varub = gmoGetVarUpperOne(gmo, i);
			
			if (vartype == var_SI && varub - varlb <= 1000) { // model lotsize for discrete variable as a set of integer values
				int len = (int)(varub - varlb + 2);
				double* points2 = new double[len];
				points2[0] = 0.;
				int j = 1;
				for (int p = (int)varlb; p <= varub; ++p, ++j)
					points2[j] = (double)p;
				objects[object_nr] = new CbcLotsize(model, i, len, points2, false);
				delete[] points2;
				
			} else { // lotsize for continuous variable or integer with large upper bounds
				if (vartype == var_SI)
					gmoLogStat(gmo, "Warning: Support of semiinteger variables with a range larger than 1000 is experimental.\n");
				points[2] = varlb;
				points[3] = varub;
				if (varlb == varub) // var. can be only 0 or varlb
					objects[object_nr] = new CbcLotsize(model, i, 2, points+1, false);
				else // var. can be 0 or in the range between low and upper 
					objects[object_nr] = new CbcLotsize(model, i, 2, points,   true );
			}

			++object_nr;
		}
		assert(object_nr == numSemi);
	  model->addObjects(object_nr, objects);
	  for (int i = 0; i < object_nr; ++i)
			delete objects[i];
		delete[] objects;
  }
  
	return true;
}

bool GamsCbc::setupStartingPoint() {
	double* varlevel = NULL;
	if (gmoHaveBasis(gmo)) {
		varlevel         = new double[gmoN(gmo)];
		double* rowprice = new double[gmoM(gmo)];
		int* cstat       = new int[gmoN(gmo)];
		int* rstat       = new int[gmoM(gmo)];

		gmoGetVarL(gmo, varlevel);
		gmoGetEquM(gmo, rowprice);
		
	  int nbas = 0;
		for (int j = 0; j < gmoN(gmo); ++j) {
			switch (gmoGetVarBasOne(gmo, j)) {
				case 0: // this seem to mean that variable should be basic
					if (++nbas <= gmoM(gmo))
						cstat[j] = 1; // basic
					else if (gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo))
						cstat[j] = 0; // superbasic = free
					else if (fabs(gmoGetVarLowerOne(gmo, j)) < fabs(gmoGetVarUpperOne(gmo, j)))
						cstat[j] = 3; // nonbasic at lower bound
					else
						cstat[j] = 2; // nonbasic at upper bound
					break;
				case 1:
					if (fabs(gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j)) < fabs(gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j)))
						cstat[j] = 3; // nonbasic at lower bound
					else
						cstat[j] = 2; // nonbasic at upper bound
					break;
				default:
					gmoLogStat(gmo, "Error: invalid basis indicator for column.");
					delete[] cstat;
					delete[] rstat;
					delete[] rowprice;
					delete[] varlevel;
					return false;
			}
		}
		
		nbas = 0;
		for (int j = 0; j< gmoM(gmo); ++j) {
			switch (gmoGetEquBasOne(gmo, j)) {
				case 0:
					if (++nbas < gmoM(gmo))
						rstat[j] = 1; // basic
					else
						rstat[j] = 2; // nonbasic at upper bound (flipped for cbc)
					break;
				case 1:
					rstat[j] = 2; // nonbasic at upper bound (flipped for cbc)
					break;
				default:
					gmoLogStat(gmo, "Error: invalid basis indicator for row.");
					delete[] cstat;
					delete[] rstat;
					delete[] rowprice;
					delete[] varlevel;
					return false;
			}
		}

		model->solver()->setColSolution(varlevel);
		model->solver()->setRowPrice(rowprice);
		// this call initializes ClpSimplex data structures, which can produce an error if CLP does not like the model
		if (model->solver()->setBasisStatus(cstat, rstat)) {
			gmoLogStat(gmo, "Failed to set initial basis. Probably CLP abandoned the model. Exiting ...");
			delete[] varlevel;
			delete[] rowprice;
			delete[] cstat;
			delete[] rstat;
			return false;
		}

		delete[] rowprice;
		delete[] cstat;
		delete[] rstat;
	}
	
	if (gmoModelType(gmo) != Proc_lp && gmoModelType(gmo) != Proc_rmip && options.getBool("mipstart")) {
		double objval = 0.;
		if (!varlevel) {
			varlevel = new double[gmoN(gmo)];
			gmoGetVarL(gmo, varlevel);
		}
		const double* objcoeff = model->solver()->getObjCoefficients();
		for (int i = 0; i < gmoN(gmo); ++i)
			objval += objcoeff[i] * varlevel[i];
		gmoLogPChar(gmo, "Letting CBC try user-given column levels as initial MIP solution:");
		model->setBestSolution(varlevel, gmoN(gmo), objval, true);
	}
	
	delete[] varlevel;
	
	return true;
}

bool GamsCbc::setupParameters() {
	//note: does not seem to work via Osi: OsiDoPresolveInInitial, OsiDoDualInInitial  

	// Some tolerances and limits
	model->setDblParam(CbcModel::CbcMaximumSeconds,  options.getDouble("reslim"));
	model->solver()->setDblParam(OsiPrimalTolerance, options.getDouble("tol_primal"));
	model->solver()->setDblParam(OsiDualTolerance,   options.getDouble("tol_dual"));
	
	// iteration limit only for LPs
	if (isLP())
		model->solver()->setIntParam(OsiMaxNumIteration, options.getInteger("iterlim"));

	// MIP parameters
	model->setIntParam(CbcModel::CbcMaxNumNode,           options.getInteger("nodelim"));
	model->setDblParam(CbcModel::CbcAllowableGap,         options.getDouble ("optca"));
	model->setDblParam(CbcModel::CbcAllowableFractionGap, options.getDouble ("optcr"));
	model->setDblParam(CbcModel::CbcIntegerTolerance,     options.getDouble ("tol_integer"));
	if (options.isDefined("cutoff"))
		model->setCutoff(model->solver()->getObjSense() * options.getDouble("cutoff")); // Cbc assumes a minimization problem here
	model->setPrintFrequency(options.getInteger("printfrequency"));
//	model.solver()->setIntParam(OsiMaxNumIterationHotStart,100);
	
	std::list<std::string> par_list;

	char buffer[255];

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
			gmoLogStat(gmo, "Cannot read value for option 'crash'. Ignoring this option");
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
				gmoLogStat(gmo, "Unsupported value for option 'crash'. Ignoring this option");
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
			gmoLogStat(gmo, "Cannot read value for option 'dualpivot'. Ignoring this option");
		} else {
			if (strcmp(value, "auto")     == 0 || 
			    strcmp(value, "dantzig")  == 0 || 
			    strcmp(value, "partial")  == 0 ||
					strcmp(value, "steepest") == 0) {
				par_list.push_back("-dualPivot");
				par_list.push_back(value);
			} else {
				gmoLogStat(gmo, "Unsupported value for option 'dualpivot'. Ignoring this option");
			}
		}
	}

	if (options.isDefined("primalpivot")) {
		char* value = options.getString("primalpivot", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'primalpivot'. Ignoring this option");
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
				gmoLogStat(gmo, "Unsupported value for option 'primalpivot'. Ignoring this option");
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
			gmoLogStat(gmo, "Cannot read value for option 'scaling'. Ignoring this option");
		} else if
			(strcmp(value, "auto")        == 0 || 
			 strcmp(value, "off")         == 0 || 
			 strcmp(value, "equilibrium") == 0 || 
			 strcmp(value, "geometric")   == 0) {
			par_list.push_back("-scaling");
			par_list.push_back(value);
		} else {
			gmoLogStat(gmo, "Unsupported value for option 'scaling'. Ignoring this option");
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

	// MIP parameters
	
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
			gmoLogStat(gmo, "Cannot read value for option 'cuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'cuts'. Ignoring this option");
		} 
	}
	
	if (options.isDefined("cliquecuts")) {
		char* value = options.getString("cliquecuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'cliquecuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'cliquecuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("flowcovercuts")) {
		char* value = options.getString("flowcovercuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'flowcovercuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'flowcovercuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("gomorycuts")) {
		char* value = options.getString("gomorycuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'gomorycuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'gomorycuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("knapsackcuts")) {
		char* value = options.getString("knapsackcuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'knapsackcuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'knapsackcuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("liftandprojectcuts")) {
		char* value = options.getString("liftandprojectcuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'liftandprojectcuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'liftandprojectcuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("mircuts")) {
		char* value = options.getString("mircuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'mircuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'mircuts'. Ignoring this option");
		}
	}

	if (options.isDefined("probingcuts")) {
		char* value = options.getString("probingcuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'probingcuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'probingcuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("reduceandsplitcuts")) {
		char* value = options.getString("reduceandsplitcuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'reduceandsplitcuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'reduceandsplitcuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("residualcapacitycuts")) {
		char* value = options.getString("residualcapacitycuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'residualcapacitycuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'residualcapacitycuts'. Ignoring this option");
		} 
	}

	if (options.isDefined("twomircuts")) {
		char* value = options.getString("twomircuts", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'twomircuts'. Ignoring this option");
		} else if
		  ( strcmp(value, "off")     == 0 ||
		    strcmp(value, "on")      == 0 ||
		    strcmp(value, "root")    == 0 ||
		    strcmp(value, "ifmove")  == 0 ||
		    strcmp(value, "forceon") == 0 ) {		   
			par_list.push_back("-twoMirCuts");
			par_list.push_back(value);
		} else {
			gmoLogStat(gmo, "Unsupported value for option 'twomircuts'. Ignoring this option");
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
			gmoLogStat(gmo, "Cannot read value for option 'greedyheuristic'. Ignoring this option");
		} else if 
		   ( strcmp(value, "off")  == 0 ||
		     strcmp(value, "on")   == 0 ||
		     strcmp(value, "root") == 0 ) {
			par_list.push_back("-greedyHeuristic");
			par_list.push_back(value);
		} else {
			gmoLogStat(gmo, "Unsupported value for option 'greedyheuristic'. Ignoring this option");
		} 
	}

	if (options.isDefined("localtreesearch")) {
		par_list.push_back("-localTreeSearch");
		par_list.push_back(options.getBool("localtreesearch") ? "on" : "off");
	}

	if (options.isDefined("rins")) {
		par_list.push_back("-Rins");
		par_list.push_back(options.getBool("rins") ? "on" : "off");
	}

	if (options.isDefined("roundingheuristic")) {
		par_list.push_back("-roundingHeuristic");
		par_list.push_back(options.getBool("roundingheuristic") ? "on" : "off");
	}
	
	if (options.isDefined("coststrategy")) {
		char* value = options.getString("coststrategy", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'coststrategy'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'coststrategy'. Ignoring this option");
		} 
	}
	
	if (options.isDefined("nodestrategy")) {
		char* value=options.getString("nodestrategy", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'nodestrategy'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'nodestrategy'. Ignoring this option");
		} 
	}

	if (options.isDefined("preprocess")) {
		char* value = options.getString("preprocess", buffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'preprocess'. Ignoring this option");
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
			gmoLogStat(gmo, "Unsupported value for option 'coststrategy'. Ignoring this option");
		} 
	} /* else if (gm.nSemiContinuous()) {
		myout << "CBC integer preprocessing does not handle semicontinuous variables correct (yet), thus we switch it off." << CoinMessageEol;
		par_list.push_back("-preprocess");
		par_list.push_back("off");
	} */
	
	if (options.isDefined("increment")) {
		par_list.push_back("-increment");
		sprintf(buffer, "%g", options.getDouble("increment"));
		par_list.push_back(buffer);
	}
	
	if (options.isDefined("threads")) {
#ifdef CBC_THREAD
//		if (options.getInteger("threads") > 1 && (options.isDefined("usercutcall") || options.isDefined("userheurcall"))) {  //TODO
//			myout << "Cannot use BCH in multithread mode. Option threads ignored." << CoinMessageEol;
//		} else {
			par_list.push_back("-threads");
			sprintf(buffer, "%d", options.getInteger("threads"));
			par_list.push_back(buffer);
//		}
#else
		gmoLogStat(gmo, "Warning: CBC has not been compiled with multithreading support. Option threads ignored.");
#endif
	}
	
	// special options set by user and passed unseen to CBC
	if (options.isDefined("special")) {
		char longbuffer[10000];
		char* value = options.getString("special", longbuffer);
		if (!value) {
			gmoLogStat(gmo, "Cannot read value for option 'special'. Ignoring this option");
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
			gmoLogStat(gmo, "Cannot read value for option 'startalg'. Ignoring this option");
		} else if (strcmp(value, "primal")  == 0) {
			par_list.push_back("-primalSimplex");
		} else if (strcmp(value, "dual")    == 0) {
			par_list.push_back("-dualSimplex");
		} else if (strcmp(value, "barrier") == 0) {
			par_list.push_back("-barrier");
		} else {
			gmoLogStat(gmo, "Unsupported value for option 'startalg'. Ignoring this option");
		}
		if (!isLP())
			par_list.push_back("-solve");
	} else
		par_list.push_back("-solve"); 
	
	int par_list_length = par_list.size();
	assert(!cbc_args);
	cbc_argc = par_list_length+2;
	cbc_args = new char*[cbc_argc];
	cbc_args[0] = strdup("GAMS/CBC");
	int i = 1;
	for (std::list<std::string>::iterator it(par_list.begin()); it != par_list.end(); ++it, ++i)
		cbc_args[i] = strdup(it->c_str());
	cbc_args[i++] = strdup("-quit");
	
	return true;
}

bool GamsCbc::writeSolution(double cputime) {
	bool write_solution = false;
	char buffer[255];
	
	gmoLogStat(gmo, "");
	if (isLP()) { // if Cbc solved an LP, the solution status is not correctly stored in the CbcModel, we have to look into the solver
		if (model->solver()->isProvenDualInfeasible()) {
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
			gmoLogStat(gmo, "Model unbounded.");

		} else if (model->solver()->isProvenPrimalInfeasible()) {
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
			gmoLogStat(gmo, "Model infeasible.");

		} else if (model->solver()->isAbandoned()) {
			gmoSolveStatSet(gmo, SolveStat_SolverErr);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoLogStat(gmo, "Model abandoned.");

		} else if (model->solver()->isProvenOptimal()) {
			write_solution = true;
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
			gmoLogStat(gmo, "Solved to optimality.");

		} else if (model->isSecondsLimitReached()) {
			gmoSolveStatSet(gmo, SolveStat_Resource);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoLogStat(gmo, "Time limit reached.");
			
		} else if (model->solver()->isIterationLimitReached()) {
			gmoSolveStatSet(gmo, SolveStat_Iteration);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoLogStat(gmo, "Iteration limit reached.");
			
		} else if (model->solver()->isPrimalObjectiveLimitReached()) {
			gmoSolveStatSet(gmo, SolveStat_Solver);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoLogStat(gmo, "Primal objective limit reached.");

		} else if (model->solver()->isDualObjectiveLimitReached()) {
			gmoSolveStatSet(gmo, SolveStat_Solver);
			gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
			gmoLogStat(gmo, "Dual objective limit reached.");

		} else {
			gmoSolveStatSet(gmo, SolveStat_Solver);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoLogStat(gmo, "Model status unknown, no feasible solution found.");
			gmoLogStat(gmo, buffer);
		}
		
	} else { // solved a MIP
		if (model->solver()->isProvenDualInfeasible()) {
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
			gmoLogStat(gmo, "Model unbounded.");

		} else if (model->isAbandoned()) {
			gmoSolveStatSet(gmo, SolveStat_SolverErr);
			gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			gmoLogStat(gmo, "Model abandoned.");

		} else if (model->isProvenOptimal()) {
			write_solution = true;
			gmoSolveStatSet(gmo, SolveStat_Normal);
			if (options.getDouble("optca") > 0 || options.getDouble("optcr") > 0) {
				gmoModelStatSet(gmo, ModelStat_Integer);
				gmoLogStat(gmo, "Solved to optimality (within gap tolerances optca and optcr).");
			} else {
				gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
				gmoLogStat(gmo, "Solved to optimality.");
			}

		} else if (model->isNodeLimitReached()) {
			gmoSolveStatSet(gmo, SolveStat_Iteration);
			if (model->bestSolution()) {
				write_solution = true;
				gmoModelStatSet(gmo, ModelStat_Integer);
				gmoLogStat(gmo, "Node limit reached. Have feasible solution.");
			} else {
				gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
				gmoLogStat(gmo, "Node limit reached. No feasible solution found.");
			}

		} else if (model->isSecondsLimitReached()) {
			gmoSolveStatSet(gmo, SolveStat_Resource);
			if (model->bestSolution()) {
				write_solution = true;
				gmoModelStatSet(gmo, ModelStat_Integer);
				gmoLogStat(gmo, "Time limit reached. Have feasible solution.");
			} else {
				gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
				gmoLogStat(gmo, "Time limit reached. No feasible solution found.");
			}

		} else if (model->isProvenInfeasible()) {
			gmoSolveStatSet(gmo, SolveStat_Normal);
			gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
			gmoLogStat(gmo, "Model infeasible.");

		} else {
			gmoSolveStatSet(gmo, SolveStat_Solver);
			if (model->bestSolution()) {
				write_solution = true;
				gmoModelStatSet(gmo, ModelStat_Integer);
				gmoLogStat(gmo, "Model status unknown, but have feasible solution.");
			} else {
				gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
				gmoLogStat(gmo, "Model status unknown, no feasible solution found.");
			}
			sprintf(buffer, "CBC primary status: %d secondary status: %d\n", model->status(), model->secondaryStatus());
			gmoLogStat(gmo, buffer);
		}
	}

	gmoSetHeadnTail(gmo, Hiterused, model->getIterationCount());
	gmoSetHeadnTail(gmo, HresUsed, cputime); // TODO give walltime if options.getInteger("threads") > 1
	gmoSetHeadnTail(gmo, Tmipbest, model->getBestPossibleObjValue());
	gmoSetHeadnTail(gmo, Tmipnod, model->getNodeCount());
	
	if (write_solution && !gamsOsiStoreSolution(gmo, *model->solver(), false))
		return false;

	if (!isLP()) {
		if (model->bestSolution()) {
			//		if (opt.getInteger("threads")>1)
			//			snprintf(buffer, 255, "MIP solution: %21.10g   (%d nodes, %g CPU seconds, %.2f wall clock seconds)", model.getObjValue(), model.getNodeCount(), gm.SecondsSinceStart(), CoinWallclockTime());
			//		else
			snprintf(buffer, 255, "MIP solution: %21.10g   (%d nodes, %g seconds)", model->getObjValue(), model->getNodeCount(), cputime);
			gmoLogStat(gmo, buffer);
		}
		snprintf(buffer, 255, "Best possible: %20.10g", model->getBestPossibleObjValue());
		gmoLogStat(gmo, buffer);
		if (model->bestSolution()) {
			snprintf(buffer, 255, "Absolute gap: %21.5g   (absolute tolerance optca: %g)", CoinAbs(model->getObjValue() - model->getBestPossibleObjValue()), options.getDouble("optca"));
			gmoLogStat(gmo, buffer);
			snprintf(buffer, 255, "Relative gap: %21.5g   (relative tolerance optcr: %g)", CoinAbs(model->getObjValue() - model->getBestPossibleObjValue()) / CoinMax(CoinAbs(model->getBestPossibleObjValue()), 1.), options.getDouble("optcr"));
			gmoLogStat(gmo, buffer);
		}
	}
	
	return true;
}

bool GamsCbc::isLP() {
	if (gmoModelType(gmo) == Proc_lp)
		return true;
	if (gmoModelType(gmo) == Proc_rmip)
		return true;
	if (gmoNDisc(gmo)) // TODO does semicontinuous variables count as discrete?
		return false;
	int numSos1, numSos2, nzSos;
	gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
	if (nzSos)
		return false;
	return true;
}
