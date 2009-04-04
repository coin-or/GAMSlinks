// Copyright (C) 2006-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

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

// some STD templates to simplify Johns parameter handling for us
#include <list>
#include <string>

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsFinalize.hpp"
#include "GamsOptions.hpp"
#include "GamsDictionary.hpp"
#include "GamsHandlerIOLib.hpp"
#include "GamsBCH.hpp"
#include "GamsCutGenerator.hpp"
#include "GamsHeuristic.hpp"
extern "C" {
#include "gdxcc.h"
}

// For Branch and bound
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp" //for CbcSOS
#include "CbcBranchLotsize.hpp" //for CbcLotsize

#include "OsiClpSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"

void setupProblem(GamsModel& gm, GamsDictionary& gamsdict, OsiClpSolverInterface& solver);
void setupPrioritiesSOSSemiCon(GamsModel& gm, CbcModel& model);
void setupStartingPoint(GamsModel& gm, GamsOptions& opt, CbcModel& model);
void setupParameters(GamsModel& gm, GamsOptions& opt, CbcModel& model);
void setupParameterList(GamsModel& gm, GamsOptions& opt, CoinMessageHandler& myout, std::list<std::string>& par_list);

/* Meaning of whereFrom:
   1 after initial solve by dualsimplex etc
   2 after preprocessing
   3 just before branchAndBound (so user can override)
   4 just after branchAndBound (before postprocessing)
   5 after postprocessing
   6 after a user called heuristic phase
*/
CbcModel* preprocessedmodel=NULL;
int gamsCallBack(CbcModel* currentSolver, int whereFrom) {
//	printf("Got callback from %d with current solver %p\n", whereFrom, (void*)currentSolver);
	if (whereFrom==3) preprocessedmodel=currentSolver;
	
	return 0;
}

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

	OsiClpSolverInterface solver;

	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
	GamsModel gm(argv[1]);
	gm.setInfinity(-solver.getInfinity(), solver.getInfinity());
	
	GamsHandlerIOLib gamshandler(gm.isReformulated());
	
	// Pass in the GAMS status/log file print routines 
	GamsMessageHandler myout(gamshandler), slvout(gamshandler);
	slvout.setPrefix(0);
	solver.passInMessageHandler(&slvout);
//	solver.getModelPtr()->passInMessageHandler(&slvout);
	solver.setHintParam(OsiDoReducePrint,true,OsiHintTry);

	myout.setCurrentDetail(1);
	gm.PrintOut(GamsModel::StatusMask, "=1"); // turn on copying into .lst file
#ifdef GAMS_BUILD
	myout << "\nGAMS/CoinCbc 2.3pre LP/MIP Solver\nwritten by J. Forrest\n " << CoinMessageEol;
	GamsOptions opt(gamshandler, "coincbc");
#else
	myout << "\nGAMS/Cbc 2.3pre LP/MIP Solver\nwritten by J. Forrest\n " << CoinMessageEol;
	GamsOptions opt(gamshandler, "cbc");
#endif
	opt.readOptionsFile(gm.getOptionfile());

	/* Overwrite GAMS Options */
	if (!opt.isDefined("reslim")) opt.setDouble("reslim", gm.getResLim());
	if (!opt.isDefined("iterlim")) opt.setInteger("iterlim", gm.getIterLim());
	if (!opt.isDefined("nodlim")) opt.setInteger("nodlim", gm.getNodeLim());
	if (!opt.isDefined("nodelim")) opt.setInteger("nodelim", opt.getInteger("nodlim"));
	if (!opt.isDefined("optca")) opt.setDouble("optca", gm.getOptCA());
	if (!opt.isDefined("optcr")) opt.setDouble("optcr", gm.getOptCR());
	if (!opt.isDefined("cutoff") && gm.getCutOff()!=gm.ObjSense()*solver.getInfinity()) opt.setDouble("cutoff", gm.getCutOff());
	if (!opt.isDefined("increment") && gm.getCheat()) opt.setDouble("increment", gm.getCheat());
	
	gm.readMatrix();
	myout << "Problem statistics:" << gm.nCols() << "columns and" << gm.nRows() << "rows." << CoinMessageEol;
	if (gm.nDCols()) myout << "                   " << gm.nDCols() << "variables have integrality restrictions." << CoinMessageEol;
	if (gm.nSemiContinuous()) myout << "                   " << gm.nSemiContinuous() << "variables are semicontinuous or semiinteger." << CoinMessageEol;
	if (gm.nSOS1() || gm.nSOS2()) {
		myout << "                   ";
		if (gm.nSOS1()) myout << gm.nSOS1() << "SOS of type 1. ";
		if (gm.nSOS2()) myout << gm.nSOS2() << "SOS of type 2.";
		myout << CoinMessageEol;
	}

	gm.TimerStart();
	CoinWallclockTime(); // start wallclock timer
	
	GamsDictionary gamsdict(gamshandler);
	if (opt.getBool("names"))
		gamsdict.readDictionary();

	setupProblem(gm, gamsdict, solver);
	
	// Write MPS file
	if (opt.isDefined("writemps")) {
		opt.getString("writemps", buffer);
  	myout << "\nWriting MPS file " << buffer << "... " << CoinMessageEol;
  	solver.writeMps(buffer,"",gm.ObjSense());
	}

	CbcModel model(solver);
	model.passInMessageHandler(&slvout);

	CbcMain0(model);
  // Switch off most output
	model.solver()->setHintParam(OsiDoReducePrint, true, OsiHintTry);

	if (gm.nCols()) {
		setupPrioritiesSOSSemiCon(gm, model);
		setupStartingPoint(gm, opt, model);
	}
	if (opt.isDefined("usercutcall") || opt.isDefined("userheurcall")) { //TODO: avoid this
		opt.setString("preprocess", "off");
	}
	setupParameters(gm, opt, model);
	
	std::list<std::string> par_list;
	setupParameterList(gm, opt, myout, par_list);
	int par_list_length=par_list.size();
	const char** cbc_args=new const char*[par_list_length+2];
	cbc_args[0]="GAMS/CBC";
	int i=1;
	for (std::list<std::string>::iterator it(par_list.begin()); it!=par_list.end(); ++it, ++i)
		cbc_args[i]=it->c_str();
	cbc_args[i++]="-quit";

	// setup BCH if required
	GamsBCH* bch=NULL;
	gdxHandle_t gdxhandle=NULL;
	if (opt.isDefined("usercutcall") || opt.isDefined("userheurcall")) {
		if (!gdxCreate(&gdxhandle, buffer, sizeof(buffer))) {
			gm.PrintOut(GamsModel::AllMask, buffer);
			exit(EXIT_FAILURE);
		}
		if (!gamsdict.haveNames() && !gamsdict.readDictionary()) {
			gm.PrintOut(GamsModel::AllMask, "Error: Need dictionary to enable BCH.");
			exit(EXIT_FAILURE);
		}
		
		bch=new GamsBCH(gamshandler, gamsdict, opt);
		bch->setGlobalBounds(gm.ColLb(), gm.ColUb());

		if (opt.isDefined("usercutcall")) {
			GamsCutGenerator gamscutgen(*bch, preprocessedmodel);
			model.addCutGenerator(&gamscutgen, 1, "GamsBCH", true, opt.getBool("usercutnewint"));
		}
		if (opt.isDefined("userheurcall")) {
			GamsHeuristic gamsheu(*bch);
			model.addHeuristic(&gamsheu, "GamsBCH");
		}
	}
	
	
	gm.PrintOut(GamsModel::StatusMask, "=2"); // turn off copying into .lst file
	myout << "\nCalling CBC main solution routine..." << CoinMessageEol;	
	myout.setCurrentDetail(2);
	CbcMain1(par_list_length+2,cbc_args,model,gamsCallBack);
	delete[] cbc_args;

	myout.setCurrentDetail(1);
	gm.PrintOut(GamsModel::StatusMask, "=1"); // turn on copying into .lst file
	if (gm.isLP()) { // we solved an LP
		// Clp interchange lower and upper activity of rows since it thinks in term of artifical variables
		//TODO: report whether feasible if time/iterlimit reached
	  GamsFinalizeOsi(&gm, &myout, model.solver(), model.isSecondsLimitReached(), false, true);
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
		if (opt.getDouble("optca")>0 || opt.getDouble("optcr")>0) {
			gm.setStatus(GamsModel::NormalCompletion, GamsModel::IntegerSolution);
			myout << "Solved to optimality (within gap tolerances)." << CoinMessageEol;
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
		if (model.bestSolution()) {
			write_solution=true;
			gm.setStatus(GamsModel::TerminatedBySolver, GamsModel::IntegerSolution);
			myout << "Model status unknown, but have feasible solution.";
		} else {
			gm.setStatus(GamsModel::TerminatedBySolver, GamsModel::ErrorNoSolution);
			myout << "Model status unknown, No feasible solution found.";
		}
		 myout << "CBC primary status:" << model.status() << "secondary status:" << model.secondaryStatus() << CoinMessageEol;
//		 myout << "CBC number of nodes:" << model.getNodeCount() << " max: " << model.getIntParam(CbcModel::CbcMaxNumNode) << CoinMessageEol;
	}

	gm.setIterUsed(model.getIterationCount());
	if (opt.getInteger("threads")>1)
		gm.setResUsed(CoinWallclockTime());
	else
		gm.setResUsed(gm.SecondsSinceStart());
	gm.setObjBound(model.getBestPossibleObjValue());
	gm.setNodesUsed(model.getNodeCount());
	if (write_solution) {
		GamsWriteSolutionOsi(&gm, &myout, model.solver(), false);
	} else { // trigger the write of GAMS solution file
		gm.setSolution();
	}

	if (model.bestSolution()) {
		if (opt.getInteger("threads")>1)
			snprintf(buffer, 255, "MIP solution: %21.10g   (%d nodes, %g CPU seconds, %.2f wall clock seconds)", model.getObjValue(), model.getNodeCount(), gm.SecondsSinceStart(), CoinWallclockTime());
		else
			snprintf(buffer, 255, "MIP solution: %21.10g   (%d nodes, %g seconds)", model.getObjValue(), model.getNodeCount(), gm.SecondsSinceStart());
		gm.PrintOut(GamsModel::AllMask, buffer);
	}
	snprintf(buffer, 255, "Best possible: %20.10g", model.getBestPossibleObjValue());
	gm.PrintOut(GamsModel::AllMask, buffer);
	if (model.bestSolution()) {
		snprintf(buffer, 255, "Absolute gap: %21.5g   (absolute tolerance optca: %g)", CoinAbs(model.getObjValue()-model.getBestPossibleObjValue()), opt.getDouble("optca"));
		gm.PrintOut(GamsModel::AllMask, buffer);
		snprintf(buffer, 255, "Relative gap: %21.5g   (relative tolerance optcr: %g)", CoinAbs(model.getObjValue()-model.getBestPossibleObjValue())/CoinMax(CoinAbs(model.getBestPossibleObjValue()), 1.), opt.getDouble("optcr"));
		gm.PrintOut(GamsModel::AllMask, buffer);
	}

	delete bch;
	if (gdxhandle) {
		gdxClose(gdxhandle);
		gdxFree(&gdxhandle);
		gdxLibraryUnload();
	}

	return EXIT_SUCCESS;
}

void setupProblem(GamsModel& gm, GamsDictionary& gamsdict, OsiClpSolverInterface& solver) {
	// Osi needs rowrng for the loadProblem call
	double *rowrng = CoinCopyOfArrayOrZero((double*)NULL, gm.nRows()); 

	// current Cbc does not like zeros when doing postsolve
	gm.matSqueezeZeros();

	solver.setObjSense(gm.ObjSense());
	solver.setDblParam(OsiObjOffset, gm.ObjConstant()); // obj constant

	solver.loadProblem(gm.nCols(), gm.nRows(), gm.matStart(), 
	                    gm.matRowIdx(), gm.matValue(), 
	                    gm.ColLb(), gm.ColUb(), gm.ObjCoef(), 
	                    gm.RowSense(), gm.RowRhs(), rowrng);
//	for (int i=0; i<gm.nCols(); ++i)
//		printf("col. %d: [%.20g, %.20g]\t %.20g\t %d\n", i, gm.ColLb()[i], gm.ColUb()[i], gm.ObjCoef()[i], gm.matStart()[i]);  
//	for (int i=0; i<gm.nRows(); ++i)
//		printf("row. %d: %c\t %.20g\n", i, gm.RowSense()[i], gm.RowRhs()[i]);  
//	for (int i=0; i<gm.nNnz(); ++i)
//		printf("%d %d %.20g\n", i, gm.matRowIdx()[i], gm.matValue()[i]);  

	// We don't need these guys anymore
	delete[] rowrng;

	// Tell solver which variables are discrete
	if (gm.nDCols()) {
		bool* discVar=gm.ColDisc();
		for (int j=0; j<gm.nCols(); ++j) 
	  	if (discVar[j]) solver.setInteger(j);
	}
	
	// Cbc thinks in terms of lotsize objects, and for those the lower bound has to be 0
	if (gm.nSemiContinuous())
		for (int j=0; j<gm.nCols(); ++j)
			if (gm.ColSemiContinuous()[j]) solver.setColLower(j, 0.);

	if (!gm.nCols()) {
		gm.PrintOut(GamsModel::LogMask, "Problem has no columns. Adding fake column...\n");
		CoinPackedVector vec(0);
		solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.);
	}

	if (gamsdict.haveNames()) { // set variable and constraint names
		solver.setIntParam(OsiNameDiscipline, 2);
		char buffer[255]; 
		std::string stbuffer;
		for (int j=0; j<gm.nCols(); ++j)
			if (gamsdict.getColName(j, buffer, 255)) {
				stbuffer=buffer;
				solver.setColName(j, stbuffer);
			}
		for (int j=0; j<gm.nRows(); ++j)
			if (gamsdict.getRowName(j, buffer, 255)) {
				stbuffer=buffer;
				solver.setRowName(j, stbuffer);
			}
	}
}

void setupPrioritiesSOSSemiCon(GamsModel& gm, CbcModel& model) {
	int i,j;
	// range of priority values
	double minprior=model.solver()->getInfinity();
	double maxprior=-model.solver()->getInfinity();
	// take care of integer variable branching priorities
	if (gm.getPriorityOption()) {
		// first check which range of priorities is given
		for (int i=0; i<gm.nCols(); ++i) {
			if (!gm.ColDisc()[i] && !gm.SOSIndicator()[i] && !gm.ColSemiContinuous()) continue;
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
  
  if (gm.nSemiContinuous()) {
		CbcObject** objects = new CbcObject*[gm.nSemiContinuous()];
		int object_nr=0;
		double points[4];
		points[0]=0.;
		points[1]=0.;
		for (i=0; i<gm.nCols(); ++i) {
			if (!gm.ColSemiContinuous()[i]) continue;
			
			if (gm.ColDisc()[i] && gm.ColUb()[i]<=1000) { // model lotsize for discrete variable as a set of integer values
				int len = (int)(gm.ColUb()[i]-gm.ColLb()[i]+2);
				double* points2 = new double[len];
				points2[0]=0.;
				int j=1;
				for (int p=(int)gm.ColLb()[i]; p<=gm.ColUb()[i]; ++p, ++j)
					points2[j] = (double)p;
				objects[object_nr]=new CbcLotsize(&model, i, len, points2, false);
				delete[] points2;
				
			} else { // lotsize for continuous variable or integer with large upper bounds
				if (gm.ColUb()[i]>1000)
					gm.PrintOut(GamsModel::AllMask, "Warning: Support of semiinteger variables with an upper bound larger than 1000 is experimental.\n");
				points[2]=gm.ColLb()[i];
				points[3]=gm.ColUb()[i];			
				if (gm.ColLb()[i]==gm.ColUb()[i]) // var. can be only 0 or ColLb()[i]
					objects[object_nr]=new CbcLotsize(&model, i, 2, points+1, false);
				else // var. can be 0 or in the range between low and upper 
					objects[object_nr]=new CbcLotsize(&model, i, 2, points, true);
			}

			if (gm.getPriorityOption() && minprior!=maxprior)
				objects[object_nr]->setPriority(1+(int)(999*(gm.ColPriority()[i]-minprior)/(maxprior-minprior)));

			++object_nr;
		}
		assert(object_nr==gm.nSemiContinuous());
	  model.addObjects(object_nr, objects);
	  for (i=0; i<object_nr; ++i)
			delete objects[i];
		delete[] objects;
  }
}


void setupStartingPoint(GamsModel& gm, GamsOptions& opt, CbcModel& model) {
	if (!gm.getIgnoreBasis()) { // set initial basis
		//gm.PrintOut(GamsModel::LogMask, "Setting initial basis as provided by GAMS.");
		model.solver()->setColSolution(gm.ColLevel());
		model.solver()->setRowPrice(gm.RowMargin());
		int* cstat = new int[gm.nCols()];
		int* rstat = new int[gm.nRows()];
		int nbas = 0;
		for (int j=0; j<gm.nCols(); ++j) {
			switch (gm.ColBasis()[j]) {
				case 0: // this seem to mean that variable should be basic
					if (++nbas <= gm.nRows())
						cstat[j] = 1; // basic
					else if (gm.ColLb()[j] <= gm.getMInfinity() && gm.ColUb()[j] >= gm.getPInfinity())
						cstat[j] = 0; // superbasic = free
					else if (fabs(gm.ColLb()[j]) < fabs(gm.ColUb()[j]))
						cstat[j] = 3; // nonbasic at lower bound
					else
						cstat[j] = 2; // nonbasic at upper bound
					break;
				case 1:
					if (fabs(gm.ColLevel()[j] - gm.ColLb()[j]) < fabs(gm.ColUb()[j] - gm.ColLevel()[j]))
						cstat[j] = 3; // nonbasic at lower bound
					else
						cstat[j] = 2; // nonbasic at upper bound
					break;
				default:
					gm.PrintOut(GamsModel::AllMask, "Error: invalid basis indicator for column");
					cstat[j] = 0; // free
			}
		}

		nbas = 0;
		for (int j=0; j<gm.nRows(); ++j) {
			switch (gm.RowBasis()[j]) {
				case 0:
					if (++nbas < gm.nRows())
						rstat[j] = 1; // basic
					else
						rstat[j] = 2; // nonbasic at upper bound (flipped for cbc)
					break;
				case 1:
					rstat[j] = 2; // nonbasic at upper bound (flipped for cbc)
					break;
				default:
					gm.PrintOut(GamsModel::AllMask, "Error: invalid basis indicator for row");
					rstat[j] = 2; // nonbasic at upper bound (flipped for cbc)
			}
		}
		// this call initializes ClpSimplex data structures, which can produce an error if CLP does not like the model
		if (model.solver()->setBasisStatus(cstat, rstat)) {
			gm.PrintOut(GamsModel::AllMask, "Failed to set initial basis. Probably CLP abandoned the model.\nExiting ...\n"); 
			gm.setStatus(GamsModel::TerminatedBySolver, GamsModel::ErrorNoSolution);
			gm.setSolution();
			exit(EXIT_FAILURE);
		}

		delete[] cstat;
		delete[] rstat;
	}
	
	if (!gm.isLP() && opt.getBool("mipstart")) {
		double objval=0.;
		for (int i=0; i<gm.nCols(); ++i)
			objval += gm.ObjCoef()[i]*gm.ColLevel()[i];
		gm.PrintOut(GamsModel::LogMask, "Letting CBC try user-given column levels as initial MIP solution:");
		model.setBestSolution(gm.ColLevel(), gm.nCols(), objval, true);
	}
}

void setupParameters(GamsModel& gm, GamsOptions& opt, CbcModel& model) {
	//note: does not seem to work via Osi: OsiDoPresolveInInitial, OsiDoDualInInitial  

	// Some tolerances and limits
	model.setDblParam(CbcModel::CbcMaximumSeconds, opt.getDouble("reslim"));
	model.solver()->setDblParam(OsiPrimalTolerance, opt.getDouble("tol_primal"));
	model.solver()->setDblParam(OsiDualTolerance, opt.getDouble("tol_dual"));
	
	// iteration limit only for LPs
	if (gm.isLP())
		model.solver()->setIntParam(OsiMaxNumIteration, opt.getInteger("iterlim"));

	// MIP parameters
	model.setIntParam(CbcModel::CbcMaxNumNode, opt.getInteger("nodelim"));
	model.setDblParam(CbcModel::CbcAllowableGap, opt.getDouble("optca"));
	model.setDblParam(CbcModel::CbcAllowableFractionGap, opt.getDouble("optcr"));
	if (opt.isDefined("cutoff")) model.setCutoff(gm.ObjSense()*opt.getDouble("cutoff")); // Cbc assumes a minimization problem here
	model.setDblParam(CbcModel::CbcIntegerTolerance, opt.getDouble("tol_integer"));
	model.setPrintFrequency(opt.getInteger("printfrequency"));
//	model.solver()->setIntParam(OsiMaxNumIterationHotStart,100);
}

void setupParameterList(GamsModel& gm, GamsOptions& opt, CoinMessageHandler& myout, std::list<std::string>& par_list) {
	char buffer[255];

	// LP parameters
	
	if (opt.isDefined("idiotcrash")) {
		par_list.push_back("-idiotCrash");
		sprintf(buffer, "%d", opt.getInteger("idiotcrash"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("sprintcrash")) {
		par_list.push_back("-sprintCrash");
		sprintf(buffer, "%d", opt.getInteger("sprintcrash"));
		par_list.push_back(buffer);
	} else if (opt.isDefined("sifting")) { // synonym for sprintCrash
		par_list.push_back("-sprintCrash");
		sprintf(buffer, "%d", opt.getInteger("sifting"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("crash")) {
		char* value=opt.getString("crash", buffer);
		if (!value) {
			myout << "Cannot read value for option 'crash'. Ignoring this option" << CoinMessageEol;
		}	else {
			if (strcmp(value, "off")==0 || strcmp(value, "on")==0) {
				par_list.push_back("-crash");
				par_list.push_back(value);
			} else if(strcmp(value, "solow_halim")==0) {
				par_list.push_back("-crash");
				par_list.push_back("so");
			} else if(strcmp(value, "halim_solow")==0) {
				par_list.push_back("-crash");
				par_list.push_back("ha");
			} else {
				myout << "Value " << value << " not supported for option 'crash'. Ignoring this option" << CoinMessageEol;
			} 
		}				
	}

	if (opt.isDefined("maxfactor")) {
		par_list.push_back("-maxFactor");
		sprintf(buffer, "%d", opt.getInteger("maxfactor"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("crossover")) { // should be revised if we can do quadratic
		par_list.push_back("-crossover");
		par_list.push_back(opt.getBool("crossover") ? "on" : "off");
	}

	if (opt.isDefined("dualpivot")) {
		char* value=opt.getString("dualpivot", buffer);
		if (!value) {
			myout << "Cannot read value for option 'dualpivot'. Ignoring this option" << CoinMessageEol;
		} else {
			if (strcmp(value, "auto")==0 || 
			    strcmp(value, "dantzig")==0 || 
			    strcmp(value, "partial")==0 ||
					strcmp(value, "steepest")==0) {
				par_list.push_back("-dualPivot");
				par_list.push_back(value);
			} else {
				myout << "Value " << value << " not supported for option 'dualpivot'. Ignoring this option" << CoinMessageEol;
			} 
		}				
	}

	if (opt.isDefined("primalpivot")) {
		char* value=opt.getString("primalpivot", buffer);
		if (!value) {
			myout << "Cannot read value for option 'primalpivot'. Ignoring this option" << CoinMessageEol;
		} else {
			if (strcmp(value, "auto")==0 || 
			    strcmp(value, "exact")==0 || 
			    strcmp(value, "dantzig")==0 || 
			    strcmp(value, "partial")==0 ||
					strcmp(value, "steepest")==0 ||
					strcmp(value, "change")==0 ||
					strcmp(value, "sprint")==0) {
				par_list.push_back("-primalPivot");
				par_list.push_back(value);
			} else {
				myout << "Value " << value << " not supported for option 'primalpivot'. Ignoring this option" << CoinMessageEol;
			} 
		}				
	}
	
	if (opt.isDefined("perturbation")) {
		par_list.push_back("-perturb");
		par_list.push_back(opt.getBool("perturbation") ? "on" : "off");
	}
	
	if (opt.isDefined("scaling")) {
		char* value=opt.getString("scaling", buffer);
		if (!value) {
			myout << "Cannot read value for option 'scaling'. Ignoring this option" << CoinMessageEol;
		} else if
		     (strcmp(value, "auto")==0 || 
			    strcmp(value, "off")==0 || 
			    strcmp(value, "equilibrium")==0 || 
					strcmp(value, "geometric")==0) {
			par_list.push_back("-scaling");
			par_list.push_back(value);
		} else {
			myout << "Value " << value << " not supported for option 'scaling'. Ignoring this option" << CoinMessageEol;
		} 
	}				

	if (opt.isDefined("presolve")) {
		par_list.push_back("-presolve");
		par_list.push_back(opt.getBool("presolve") ? "on" : "off");
	}

	if (opt.isDefined("tol_presolve")) {
		par_list.push_back("-preTolerance");
		sprintf(buffer, "%g", opt.getDouble("tol_presolve"));
		par_list.push_back(buffer);
	}

	// MIP parameters
	
	if (opt.isDefined("sollim")) {
		par_list.push_back("-maxSolutions");
		sprintf(buffer, "%d", opt.getInteger("sollim"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("strongbranching")) {
		par_list.push_back("-strongBranching");
		sprintf(buffer, "%d", opt.getInteger("strongbranching"));
		par_list.push_back(buffer);
	}
			
	if (opt.isDefined("trustpseudocosts")) {
		par_list.push_back("-trustPseudoCosts");
		sprintf(buffer, "%d", opt.getInteger("trustpseudocosts"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("cutdepth")) {
		par_list.push_back("-cutDepth");
		sprintf(buffer, "%d", opt.getInteger("cutdepth"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("cut_passes_root")) {
		par_list.push_back("-passCuts");
		sprintf(buffer, "%d", opt.getInteger("cut_passes_root"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("cut_passes_tree")) {
		par_list.push_back("-passTree");
		sprintf(buffer, "%d", opt.getInteger("cut_passes_tree"));
		par_list.push_back(buffer);
	}
	
	if (opt.isDefined("cuts")) {
		char* value=opt.getString("cuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'cuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-cutsOnOff");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-cutsOnOff");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'cuts'. Ignoring this option" << CoinMessageEol;
		} 
	}
	
	if (opt.isDefined("cliquecuts")) {
		char* value=opt.getString("cliquecuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'cliquecuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-cliqueCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-cliqueCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'cliquecuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("flowcovercuts")) {
		char* value=opt.getString("flowcovercuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'flowcovercuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-flowCoverCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-flowCoverCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'flowcovercuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("gomorycuts")) {
		char* value=opt.getString("gomorycuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'gomorycuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-gomoryCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-gomoryCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'gomorycuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("knapsackcuts")) {
		char* value=opt.getString("knapsackcuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'knapsackcuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-knapsackCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-knapsackCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'knapsackcuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("liftandprojectcuts")) {
		char* value=opt.getString("liftandprojectcuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'liftandprojectcuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-liftAndProjectCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-liftAndProjectCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'liftandprojectcuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("mircuts")) {
		char* value=opt.getString("mircuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'mircuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-mixedIntegerRoundingCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-mixedIntegerRoundingCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'mircuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("probingcuts")) {
		char* value=opt.getString("probingcuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'probingcuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-probingCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOn");
		} else if (strcmp(value, "forceonbut")==0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOnBut");
		} else if (strcmp(value, "forceonstrong")==0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOnStrong");
		} else if (strcmp(value, "forceonbutstrong")==0) {
			par_list.push_back("-probingCuts");
			par_list.push_back("forceOnButStrong");
		} else {
			myout << "Value " << value << " not supported for option 'probingcuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("reduceandsplitcuts")) {
		char* value=opt.getString("reduceandsplitcuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'reduceandsplitcuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-reduceAndSplitCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-reduceAndSplitCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'reduceandsplitcuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("residualcapacitycuts")) {
		char* value=opt.getString("residualcapacitycuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'residualcapacitycuts'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ||
		     strcmp(value, "ifmove")==0 ) {
			par_list.push_back("-residualCapacityCuts");
			par_list.push_back(value);
		} else if (strcmp(value, "forceon")==0) {
			par_list.push_back("-residualCapacityCuts");
			par_list.push_back("forceOn");
		} else {
			myout << "Value " << value << " not supported for option 'residualcapacitycuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("twomircuts")) {
		char* value=opt.getString("twomircuts", buffer);
		if (!value) {
			myout << "Cannot read value for option 'twomircuts'. Ignoring this option" << CoinMessageEol;
		} else if
		  ( strcmp(value, "off")==0 ||
		    strcmp(value, "on")==0 ||
		    strcmp(value, "root")==0 ||
		    strcmp(value, "ifmove")==0 ||
		    strcmp(value, "forceon")==0 ) {		   
			par_list.push_back("-twoMirCuts");
			par_list.push_back(value);
		} else {
			myout << "Value " << value << " not supported for option 'residualcapacitycuts'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("heuristics")) {
		par_list.push_back("-heuristicsOnOff");
		par_list.push_back(opt.getBool("heuristics") ? "on" : "off");
	}
	
	if (opt.isDefined("combinesolutions")) {
		par_list.push_back("-combineSolution");
		par_list.push_back(opt.getBool("combinesolutions") ? "on" : "off");
	}

	if (opt.isDefined("feaspump")) {
		par_list.push_back("-feasibilityPump");
		par_list.push_back(opt.getBool("feaspump") ? "on" : "off");
	}

	if (opt.isDefined("feaspump_passes")) {
		par_list.push_back("-passFeasibilityPump");
		sprintf(buffer, "%d", opt.getInteger("feaspump_passes"));
		par_list.push_back(buffer);
	}

	if (opt.isDefined("greedyheuristic")) {
		char* value=opt.getString("greedyheuristic", buffer);
		if (!value) {
			myout << "Cannot read value for option 'greedyheuristic'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "on")==0 ||
		     strcmp(value, "root")==0 ) {
			par_list.push_back("-greedyHeuristic");
			par_list.push_back(value);
		} else {
			myout << "Value " << value << " not supported for option 'greedyheuristic'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("localtreesearch")) {
		par_list.push_back("-localTreeSearch");
		par_list.push_back(opt.getBool("localtreesearch") ? "on" : "off");
	}

	if (opt.isDefined("rins")) {
		par_list.push_back("-Rins");
		par_list.push_back(opt.getBool("rins") ? "on" : "off");
	}

	if (opt.isDefined("roundingheuristic")) {
		par_list.push_back("-roundingHeuristic");
		par_list.push_back(opt.getBool("roundingheuristic") ? "on" : "off");
	}
	
	if (opt.isDefined("coststrategy")) {
		char* value=opt.getString("coststrategy", buffer);
		if (!value) {
			myout << "Cannot read value for option 'coststrategy'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "off")==0 ||
		     strcmp(value, "priorities")==0 ||
		     strcmp(value, "length")==0 ||
		     strcmp(value, "columnorder")==0 ) {
			par_list.push_back("-costStrategy");
			par_list.push_back(value);
		} else if (strcmp(value, "binaryfirst")==0) {
			par_list.push_back("-costStrategy");
			par_list.push_back("01first");
		} else if (strcmp(value, "binarylast")==0) {
			par_list.push_back("-costStrategy");
			par_list.push_back("01last");
		} else {
			myout << "Value " << value << " not supported for option 'coststrategy'. Ignoring this option" << CoinMessageEol;
		} 
	}
	
	if (opt.isDefined("nodestrategy")) {
		char* value=opt.getString("nodestrategy", buffer);
		if (!value) {
			myout << "Cannot read value for option 'nodestrategy'. Ignoring this option" << CoinMessageEol;
		} else if 
		   ( strcmp(value, "hybrid")==0 ||
		     strcmp(value, "fewest")==0 ||
		     strcmp(value, "depth")==0 ||
		     strcmp(value, "upfewest")==0 ||
		     strcmp(value, "downfewest")==0 ||
		     strcmp(value, "updepth")==0 ||
		     strcmp(value, "downdepth")==0 ) {
			par_list.push_back("-nodeStrategy");
			par_list.push_back(value);
		} else {
			myout << "Value " << value << " not supported for option 'nodestrategy'. Ignoring this option" << CoinMessageEol;
		} 
	}

	if (opt.isDefined("preprocess")) {
		char* value=opt.getString("preprocess", buffer);
		if (!value) {
			myout << "Cannot read value for option 'preprocess'. Ignoring this option" << CoinMessageEol;
		} else if
		  ( strcmp(value, "off")==0 ||
		    strcmp(value, "on")==0 ||
		    strcmp(value, "equal")==0 ||
		    strcmp(value, "equalall")==0 ||
		    strcmp(value, "sos")==0 ||
		    strcmp(value, "trysos")==0 ) {
			par_list.push_back("-preprocess");
			par_list.push_back(value);
		} else {
			myout << "Value " << value << " not supported for option 'coststrategy'. Ignoring this option" << CoinMessageEol;
		} 
	} /* else if (gm.nSemiContinuous()) {
		myout << "CBC integer preprocessing does not handle semicontinuous variables correct (yet), thus we switch it off." << CoinMessageEol;
		par_list.push_back("-preprocess");
		par_list.push_back("off");
	} */
	
	if (opt.isDefined("increment")) {
		par_list.push_back("-increment");
		sprintf(buffer, "%g", opt.getDouble("increment"));
		par_list.push_back(buffer);
	}
	
	if (opt.isDefined("threads")) {
#ifdef CBC_THREAD
		if (opt.getInteger("threads")>1 && (opt.isDefined("usercutcall") || opt.isDefined("userheurcall"))) {
			myout << "Cannot use BCH in multithread mode. Option threads ignored." << CoinMessageEol;
		} else {
			par_list.push_back("-threads");
			sprintf(buffer, "%d", opt.getInteger("threads"));
			par_list.push_back(buffer);
		}
#else
		myout << "Warning: CBC has not been compiled with multithreading support. Option threads ignored." << CoinMessageEol;
#endif
	}
	
	// special options set by user and passed unseen to CBC
	if (opt.isDefined("special")) {
		char longbuffer[10000];
		char* value=opt.getString("special", longbuffer);
		if (!value) {
			myout << "Cannot read value for option 'special'. Ignoring this option" << CoinMessageEol;
		} else {
			char* tok=strtok(value, " ");
			while (tok) {
				par_list.push_back(tok);
				tok=strtok(NULL, " ");
			} 
		}		
	}

	// algorithm for root node and solve command 

	if (opt.isDefined("startalg")) {
		char* value=opt.getString("startalg", buffer);
		if (!value) {
			myout << "Cannot read value for option 'startalg'. Ignoring this option" << CoinMessageEol;
		} else if (strcmp(value, "primal")==0) {
			par_list.push_back("-primalSimplex");
		} else if (strcmp(value, "dual")==0) {
			par_list.push_back("-dualSimplex");
		} else if (strcmp(value, "barrier")==0) {
			par_list.push_back("-barrier");
		} else {
			myout << "Value " << value << " not supported for option 'startalg'. Ignoring this option" << CoinMessageEol;
		} 
		if (gm.nDCols() || gm.nSOS1() || gm.nSOS2())
			par_list.push_back("-solve");
	} else
		par_list.push_back("-solve"); 
}
