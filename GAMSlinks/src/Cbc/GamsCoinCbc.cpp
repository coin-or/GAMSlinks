// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#include "GAMSlinksConfig.h"

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsFinalize.hpp"

// For Branch and bound
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp" //for CbcSOS
#include "CbcStrategyGams.hpp"

//TODO: CbcMain allows only Clp as subsolver, so we can forget about all the XXX stuff
#include "OsiSolverInterface.hpp"
#if defined(COIN_HAS_CLP)
#include "OsiClpSolverInterface.hpp"
#define OsiXXXSolverInterface OsiClpSolverInterface
#elif defined(COIN_HAS_GLPK)
#include "OsiGlpkSolverInterface.hpp"
#define OsiXXXSolverInterface OsiGlpkSolverInterface
#else
#error "Clp or Glpk need to be available."
#endif

void setupProblem(GamsModel& gm, OsiXXXSolverInterface& solver);
void setupPrioritiesAndSOS(GamsModel& gm, CbcModel& model);
void setupStartingPoint(GamsModel& gm, CbcModel& model);
void setupParameters(GamsModel& gm, CbcModel& model);

int main (int argc, const char *argv[]) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif

	if (argc==1) {
		fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n",  argv[0]);
		exit(EXIT_FAILURE);
	}	
	char buffer[255];

	OsiXXXSolverInterface solver;

	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
	GamsModel gm(argv[1],-solver.getInfinity(),solver.getInfinity());

	// Pass in the GAMS status/log file print routines 
	GamsMessageHandler myout(&gm), slvout(&gm);
	slvout.setPrefix(0);
	solver.passInMessageHandler(&slvout);
//TODO: do I need this for Clp?
	solver.getModelPtr()->passInMessageHandler(&slvout);
	solver.setHintParam(OsiDoReducePrint,true,OsiHintTry);
	
#ifdef GAMS_BUILD	
	myout << "\nGAMS/CoinCbc 1.3pre LP/MIP Solver\nwritten by J. Forrest\n " << CoinMessageEol;
	if (!gm.ReadOptionsDefinitions("coincbc"))
#else
	myout << "\nGAMS/Cbc 1.3pre LP/MIP Solver\nwritten by J. Forrest\n " << CoinMessageEol;
	if (!gm.ReadOptionsDefinitions("cbc"))
#endif
		myout << "Error intializing option file handling or reading option file definitions!" << CoinMessageEol
			<< "Processing of options is likely to fail!" << CoinMessageEol;  
	gm.ReadOptionsFile();

	/* Overwrite GAMS Options */
	if (!gm.optDefined("reslim")) gm.optSetDouble("reslim", gm.getResLim());
	if (!gm.optDefined("iterlim")) gm.optSetInteger("iterlim", gm.getIterLim());
	if (!gm.optDefined("nodlim")) gm.optSetInteger("nodlim", gm.getNodeLim());
	if (!gm.optDefined("nodelim")) gm.optSetInteger("nodelim", gm.optGetInteger("nodlim"));
	if (!gm.optDefined("optca")) gm.optSetDouble("optca", gm.getOptCA());
	if (!gm.optDefined("optcr")) gm.optSetDouble("optcr", gm.getOptCR());
	if (!gm.optDefined("cutoff") && gm.getCutOff()!=gm.ObjSense()*solver.getInfinity()) gm.optSetDouble("cutoff", gm.getCutOff());
	
	gm.TimerStart();
	
	setupProblem(gm, solver);

	// Write MPS file
	if (gm.optDefined("writemps")) {
		gm.optGetString("writemps", buffer);
  	myout << "\nWriting MPS file " << buffer << "... " << CoinMessageEol;
  	solver.writeMps(buffer,"",gm.ObjSense());
	}

	CbcModel model(solver);
	model.passInMessageHandler(&slvout);

	CbcMain0(model);
  // Switch off most output
	model.solver()->setHintParam(OsiDoReducePrint,true,OsiHintTry);

	setupPrioritiesAndSOS(gm, model);
	setupStartingPoint(gm, model);
	setupParameters(gm, model);
//	model.solver()->messageHandler()->setLogLevel(2);

	myout << "\nCalling CBC main solution routine..." << CoinMessageEol;	
	const char * argv2[]={"GAMS/CBC", "-solve","-quit"};
	CbcMain1(3,argv2,model);

	if (0==gm.nDCols() && 0==gm.nSOS1() && 0==gm.nSOS2()) { // we solved an LP
	  // Get some statistics 
	  gm.setIterUsed(model.solver()->getIterationCount());
	  gm.setResUsed(gm.SecondsSinceStart());
	  gm.setObjVal(gm.ObjSense()*model.solver()->getObjValue());
 
 		bool timelimit_reached=model.isSecondsLimitReached();
	  GamsFinalizeOsi(&gm, &myout, model.solver(), false, timelimit_reached);
	  return EXIT_SUCCESS;
	}
	
	myout << "\n" << CoinMessageEol;
	bool write_solution=false;
	if (model.solver()->isProvenDualInfeasible()) {
		gm.setStatus(GamsModel::NormalCompletion, GamsModel::UnboundedNoSolution);
		myout << "Model unbounded." << CoinMessageEol;
	} else if (model.isAbandoned()) {
		gm.setStatus(GamsModel::ErrorSolverFailure, GamsModel::ErrorNoSolution);
		myout << "Model abandoned." << CoinMessageEol;
	} else if (model.isProvenOptimal()) {
		write_solution=true;
		if (gm.optGetDouble("optca")>0 || gm.optGetDouble("optcr")>0) {
			gm.setStatus(GamsModel::NormalCompletion, GamsModel::IntegerSolution);
			myout << "Solved optimal (within gap tolerances: absolute =" << gm.optGetDouble("optca") << "relative =" << gm.optGetDouble("optcr") << ")." << CoinMessageEol;
		} else {
			gm.setStatus(GamsModel::NormalCompletion, GamsModel::Optimal);
			myout << "Solved to optimality." << CoinMessageEol;
		}
	} else if (model.isNodeLimitReached()) {
		if (model.bestSolution()) {
			write_solution=true;
			gm.setStatus(GamsModel::IterationInterrupt, GamsModel::IntegerSolution);
			myout << "Node limit reached. Have feasible solution." << CoinMessageEol;
		} else {
			gm.setStatus(GamsModel::IterationInterrupt, GamsModel::NoSolutionReturned);
			myout << "Node limit reached. No feasible solution found." << CoinMessageEol;
		}
	} else if (model.isSecondsLimitReached()) {
		if (model.bestSolution()) {
			write_solution=true;
			gm.setStatus(GamsModel::ResourceInterrupt, GamsModel::IntegerSolution);
			myout << "Time limit reached. Have feasible solution." << CoinMessageEol;
		} else {
			gm.setStatus(GamsModel::ResourceInterrupt, GamsModel::NoSolutionReturned);
			myout << "Time limit reached. No feasible solution found." << CoinMessageEol;
		}
	} else if (model.isProvenInfeasible()) {
		gm.setStatus(GamsModel::NormalCompletion, GamsModel::InfeasibleNoSolution);
		myout << "Model infeasible." << CoinMessageEol;
	} else {
		myout << "Model status unkown." << CoinMessageEol;
	}

	gm.setIterUsed(model.getIterationCount());
	gm.setResUsed(gm.SecondsSinceStart());
	if (write_solution) {
		GamsWriteSolutionOsi(&gm, &myout, model.solver());
	} else {
		gm.setObjVal(0.0);
		gm.setSolution(); // trigger the write of GAMS solution file
	}

	return EXIT_SUCCESS;
}

void setupProblem(GamsModel& gm, OsiXXXSolverInterface& solver) {
	int i,j;
	// CLP needs rowrng for the loadProblem call
	double *rowrng = new double[gm.nRows()];
	for (i=0; i<gm.nRows(); i++)
	  rowrng[i] = 0.0;

	// current Cbc does not like zeros when doing postsolve
	gm.matSqueezeZeros();

	solver.setObjSense(gm.ObjSense());
	solver.setDblParam(OsiObjOffset, gm.ObjConstant()); // obj constant

	solver.loadProblem(gm.nCols(), gm.nRows(), gm.matStart(), 
	                    gm.matRowIdx(), gm.matValue(), 
	                    gm.ColLb(), gm.ColUb(), gm.ObjCoef(), 
	                    gm.RowSense(), gm.RowRhs(), rowrng);

	// We don't need these guys anymore
	delete[] rowrng;

	// Tell solver which variables are discrete
	int *discVar=gm.ColDisc();
	for (j=0; j<gm.nCols(); j++) 
	  if (discVar[j]) solver.setInteger(j);

	char buffer[255];
	if (gm.haveNames()) { // set variable and constraint names
		solver.setIntParam(OsiNameDiscipline, 2);
		std::string stbuffer;
		for (j=0; j<gm.nCols(); ++j)
			if (gm.ColName(j, buffer, 255)) {
				stbuffer=buffer;
				solver.setColName(j, stbuffer);
			}
		for (j=0; j<gm.nRows(); ++j)
			if (gm.RowName(j, buffer, 255)) {
				stbuffer=buffer;
				solver.setRowName(j, stbuffer);
			}
	}
}

void setupPrioritiesAndSOS(GamsModel& gm, CbcModel& model) {
	int i,j;
	// range of priority values
	double minprior=model.solver()->getInfinity();
	double maxprior=-model.solver()->getInfinity();
	// take care of integer variable branching priorities
	if (gm.getPriorityOption()) {
		// first check which range of priorities is given
		for (int i=0; i<gm.nCols(); ++i) {
			if (gm.ColPriority()[i]<minprior) minprior=gm.ColPriority()[i];
			if (gm.ColPriority()[i]>maxprior) maxprior=gm.ColPriority()[i];
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

	// Tell solver which variables belong to SOS of type 1 or 2
	if (gm.nSOS1() || gm.nSOS2()) {
		CbcObject** objects = new CbcObject*[gm.nSOS1()+gm.nSOS2()];
		
		int* which = new int[gm.nCols()];
		for (i=1; i<=gm.nSOS1(); ++i) {
			int n=0;
			double priorsum=0.;
			for (j=0; j<gm.nCols(); ++j)
				if (gm.SOSIndicator()[j]==i) {
					which[n++]=j;
					priorsum+=gm.ColPriority()[j];
				}
			objects[i-1]=new CbcSOS(&model, n, which, NULL, i-1, 1);
			if (gm.getPriorityOption() && minprior!=maxprior) // scale avg. of gams priorities into {1,..,1000} range
				objects[i-1]->setPriority(1+(int)(999*((priorsum/n)-minprior)/(maxprior-minprior)));
			else // branch on long sets first
				objects[i-1]->setPriority(gm.nCols()-n);
		}
		for (i=1; i<=gm.nSOS2(); ++i) {
			int n=0;
			double priorsum=0.;
			for (j=0; j<gm.nCols(); ++j)
				if (gm.SOSIndicator()[j]==-i) {
					which[n++]=j;
					priorsum+=gm.ColPriority()[j];
				}
			objects[gm.nSOS1()+i-1]=new CbcSOS(&model, n, which, NULL, gm.nSOS1()+i-1, 2);

			if (gm.getPriorityOption() && minprior!=maxprior) // scale avg. of gams priorities into {1,..,1000} range
				objects[gm.nSOS1()+i-1]->setPriority(1+(int)(999*((priorsum/n)-minprior)/(maxprior-minprior)));
			else // branch on long sets first
				objects[gm.nSOS1()+i-1]->setPriority(gm.nCols()-n);
		}
		delete[] which;
		
	  model.addObjects(gm.nSOS1()+gm.nSOS2(), objects);
	  for (i=0; i<gm.nSOS1()+gm.nSOS2(); ++i)
			delete objects[i];
		delete[] objects;
  }
}


void setupStartingPoint(GamsModel& gm, CbcModel& model) {
  // starting point
  model.solver()->setColSolution(gm.ColLevel());
  model.solver()->setRowPrice(gm.RowMargin());
  int* cstat=new int[gm.nCols()];
  int* rstat=new int[gm.nRows()];
	for (int j=0; j<gm.nCols(); ++j) {
		switch (gm.ColBasis()[j]) {
			case GamsModel::NonBasicLower : cstat[j]=3; break;
			case GamsModel::NonBasicUpper : cstat[j]=2; break;
			case GamsModel::Basic : cstat[j]=1; break;
			case GamsModel::SuperBasic : cstat[j]=0; break;
			default: cstat[j]=0; // myout << "Column basis status " << gm.ColBasis()[j] << " unknown!" << CoinMessageEol;
		}
	}
	for (int j=0; j<gm.nRows(); ++j) {
		switch (gm.RowBasis()[j]) {
			case GamsModel::NonBasicLower : rstat[j]=2; break;
			case GamsModel::NonBasicUpper : rstat[j]=3; break;
			case GamsModel::Basic : rstat[j]=1; break;
			case GamsModel::SuperBasic : rstat[j]=0; break;
			default: rstat[j]=0; // myout << "Row basis status " << gm.RowBasis()[j] << " unknown!" << CoinMessageEol;
		}
	}
	model.solver()->setBasisStatus(cstat, rstat);
	delete[] cstat;
	delete[] rstat;
}

void setupParameters(GamsModel& gm, CbcModel& model) {
	// Some tolerances and limits
	model.setDblParam(CbcModel::CbcMaximumSeconds, gm.optGetDouble("reslim"));
	model.solver()->setDblParam(OsiPrimalTolerance, gm.optGetDouble("tol_primal"));
	model.solver()->setDblParam(OsiDualTolerance, gm.optGetDouble("tol_dual"));
	model.solver()->setHintParam(OsiDoScale, gm.optGetBool("scaling"));
	model.solver()->setHintParam(OsiDoPresolveInInitial, gm.optGetBool("presolve"));
  
	// Do initial solve to continuous
	if (gm.nDCols() || gm.nSOS1() || gm.nSOS2()) {
	} else { // iteration limit only for LPs
		model.solver()->setIntParam(OsiMaxNumIteration, gm.optGetInteger("iterlim"));
	}
	char buffer[255];	 buffer[0]=0;
	gm.optGetString("startalg", buffer);
	model.solver()->setHintParam(OsiDoDualInInitial, strcmp(buffer, "primal")==0 ? false : true);

	// MIP parameters
	model.setDblParam(CbcModel::CbcMaximumSeconds, gm.optGetDouble("reslim"));
	model.setIntParam(CbcModel::CbcMaxNumNode, gm.optGetInteger("nodelim"));
	model.setDblParam(CbcModel::CbcAllowableGap, gm.optGetDouble("optca"));
	model.setDblParam(CbcModel::CbcAllowableFractionGap, gm.optGetDouble("optcr"));
	if (gm.optDefined("cutoff")) model.setCutoff(gm.ObjSense()*gm.optGetDouble("cutoff")); // Cbc assumes a minimization problem here
	model.setDblParam(CbcModel::CbcIntegerTolerance, gm.optGetDouble("tol_integer"));
	model.solver()->setIntParam(OsiMaxNumIterationHotStart,100);
}
