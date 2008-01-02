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

// some STD templates to simplify Johns parameter handling for us
#include <list>
#include <string>

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsFinalize.hpp"
#include "GamsBCH.hpp"
#include "GamsCutGenerator.hpp"

// For Branch and bound
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp" //for CbcSOS
#include "CbcBranchLotsize.hpp" //for CbcLotsize

#include "OsiClpSolverInterface.hpp"
#include "CoinHelperFunctions.hpp"

void setupProblem(GamsModel& gm, OsiClpSolverInterface& solver);
void setupPrioritiesSOSSemiCon(GamsModel& gm, CbcModel& model);
void setupStartingPoint(GamsModel& gm, CbcModel& model);
void setupParameters(GamsModel& gm, CbcModel& model);
void setupParameterList(GamsModel& gm, CoinMessageHandler& myout, std::list<std::string>& par_list);

//GamsBCH* bch=NULL;
///* Meaning of whereFrom:
//   1 after initial solve by dualsimplex etc
//   2 after preprocessing
//   3 just before branchAndBound (so user can override)
//   4 just after branchAndBound (before postprocessing)
//   5 after postprocessing
//   6 after a user called heuristic phase
//*/
//int gamsCallBack(CbcModel* currentSolver, int whereFrom) {
//	printf("Got callback from %d. Incumbent sol.: %g\n", whereFrom, currentSolver->getObjValue());
//	return 0;
//}

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
	gm.setInfinity(-solver.getInfinity(),solver.getInfinity());
	
	// Pass in the GAMS status/log file print routines 
	GamsMessageHandler myout(&gm), slvout(&gm);
	slvout.setPrefix(0);
	solver.passInMessageHandler(&slvout);
	solver.getModelPtr()->passInMessageHandler(&slvout);
	solver.setHintParam(OsiDoReducePrint,true,OsiHintTry);

	myout.setCurrentDetail(1);
	gm.PrintOut(GamsModel::StatusMask, "=1"); // turn on copying into .lst file
#ifdef GAMS_BUILD	
	myout << "\nGAMS/CoinCbc 2.0 LP/MIP Solver\nwritten by J. Forrest\n " << CoinMessageEol;
	if (!gm.ReadOptionsDefinitions("coincbc"))
#else
	myout << "\nGAMS/Cbc 2.0 LP/MIP Solver\nwritten by J. Forrest\n " << CoinMessageEol;
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
	if (!gm.optDefined("increment") && gm.getCheat()) gm.optSetDouble("increment", gm.getCheat());
	
	gm.readMatrix();
	myout << "Problem statistics:" << gm.nCols() << "columns and" << gm.nRows() << "rows." << CoinMessageEol;
	if (gm.nDCols()) myout << "                   " << gm.nDCols() << "variables have integrality restrictions." << CoinMessageEol;
	if (gm.nSemiContinuous()) myout << "                   " << gm.nSemiContinuous() << "variables are semicontinuous or semiinteger." << CoinMessageEol;
	if (gm.nSOS1() || gm.nSOS2()) {
		myout << "                   ";
		if (gm.nSOS1()) myout << gm.nSOS1() << "SOS of type 1.";
		if (gm.nSOS2()) myout << gm.nSOS2() << "SOS of type 2.";
		myout << CoinMessageEol;
	}
//	if (gm.nSemiContinuous()) {
//		myout << "CBC does not handle semicontinuous variables correct (yet). Exiting..." << CoinMessageEol;
//		gm.setStatus(GamsModel::CapabilityProblems, GamsModel::ErrorNoSolution);
//		gm.setSolution();
//		exit(EXIT_FAILURE);
//	}
	
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
	model.solver()->setHintParam(OsiDoReducePrint, true, OsiHintTry);

	if (gm.nCols()) {
		setupPrioritiesSOSSemiCon(gm, model);
		setupStartingPoint(gm, model);
	}
	if (gm.optDefined("usercutcall")) { //TODO: avoid this
		gm.optSetString("preprocess", "off");
	}
	setupParameters(gm, model);
	
	std::list<std::string> par_list;
	setupParameterList(gm, myout, par_list);
	int par_list_length=par_list.size();
	const char** cbc_args=new const char*[par_list_length+2];
	cbc_args[0]="GAMS/CBC";
	int i=1;
	for (std::list<std::string>::iterator it(par_list.begin()); it!=par_list.end(); ++it, ++i)
		cbc_args[i]=it->c_str();
	cbc_args[i++]="-quit";

	// setup BCH if required
	GamsBCH* bch=NULL;
	if (gm.optDefined("usercutcall")) {
		bch=new GamsBCH(gm);
		GamsCutGenerator gamscutgen(*bch, model);
		model.addCutGenerator(&gamscutgen, 1, "GamsBCH"); // TODO: check the remaining arguments
	}
	
	
	gm.PrintOut(GamsModel::StatusMask, "=2"); // turn off copying into .lst file
	myout << "\nCalling CBC main solution routine..." << CoinMessageEol;	
	myout.setCurrentDetail(2);
	CbcMain1(par_list_length+2,cbc_args,model);
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
		gm.setStatus(GamsModel::ErrorSystemFailure,GamsModel::ErrorNoSolution);	
		myout << "Model status unkown." << CoinMessageEol;
	}

	gm.setIterUsed(model.getIterationCount());
	gm.setResUsed(gm.SecondsSinceStart());
	gm.setObjBound(model.getBestPossibleObjValue());
	gm.setNodesUsed(model.getNodeCount());
	if (write_solution) {
		GamsWriteSolutionOsi(&gm, &myout, model.solver(), true);
	} else { // trigger the write of GAMS solution file
		gm.setSolution();
	}
	
	delete bch;

	return EXIT_SUCCESS;
}

void setupProblem(GamsModel& gm, OsiClpSolverInterface& solver) {
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

	char buffer[255];
	if (gm.haveNames()) { // set variable and constraint names
		solver.setIntParam(OsiNameDiscipline, 2);
		std::string stbuffer;
		for (int j=0; j<gm.nCols(); ++j)
			if (gm.ColName(j, buffer, 255)) {
				stbuffer=buffer;
				solver.setColName(j, stbuffer);
			}
		for (int j=0; j<gm.nRows(); ++j)
			if (gm.RowName(j, buffer, 255)) {
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
			
			points[2]=gm.ColLb()[i];
			points[3]=gm.ColUb()[i];			
			if (gm.ColLb()[i]==gm.ColUb()[i]) // var. can be only 0 or ColLb()[i]
				objects[object_nr]=new CbcLotsize(&model, i, 2, points+1, false);
			else // var. can be 0 or in the range between low and upper 
				objects[object_nr]=new CbcLotsize(&model, i, 2, points, true);
				
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
			default: cstat[j]=0;
		}
	}
	for (int j=0; j<gm.nRows(); ++j) {
		switch (gm.RowBasis()[j]) {
			case GamsModel::NonBasicLower : rstat[j]=2; break;
			case GamsModel::NonBasicUpper : rstat[j]=3; break;
			case GamsModel::Basic : rstat[j]=1; break;
			case GamsModel::SuperBasic : rstat[j]=0; break;
			default: rstat[j]=0;
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

void setupParameters(GamsModel& gm, CbcModel& model) {
	//note: does not seem to work via Osi: OsiDoPresolveInInitial, OsiDoDualInInitial  

	// Some tolerances and limits
	model.setDblParam(CbcModel::CbcMaximumSeconds, gm.optGetDouble("reslim"));
	model.solver()->setDblParam(OsiPrimalTolerance, gm.optGetDouble("tol_primal"));
	model.solver()->setDblParam(OsiDualTolerance, gm.optGetDouble("tol_dual"));
	
	// iteration limit only for LPs
	if (gm.isLP())
		model.solver()->setIntParam(OsiMaxNumIteration, gm.optGetInteger("iterlim"));

	// MIP parameters
	model.setIntParam(CbcModel::CbcMaxNumNode, gm.optGetInteger("nodelim"));
	model.setDblParam(CbcModel::CbcAllowableGap, gm.optGetDouble("optca"));
	model.setDblParam(CbcModel::CbcAllowableFractionGap, gm.optGetDouble("optcr"));
	if (gm.optDefined("cutoff")) model.setCutoff(gm.ObjSense()*gm.optGetDouble("cutoff")); // Cbc assumes a minimization problem here
	model.setDblParam(CbcModel::CbcIntegerTolerance, gm.optGetDouble("tol_integer"));
	model.setPrintFrequency(gm.optGetInteger("printfrequency"));
//	model.solver()->setIntParam(OsiMaxNumIterationHotStart,100);
}

void setupParameterList(GamsModel& gm, CoinMessageHandler& myout, std::list<std::string>& par_list) {
	char buffer[255];

	// LP parameters
	
	if (gm.optDefined("idiotcrash")) {
		par_list.push_back("-idiotCrash");
		sprintf(buffer, "%d", gm.optGetInteger("idiotcrash"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("sprintcrash")) {
		par_list.push_back("-sprintCrash");
		sprintf(buffer, "%d", gm.optGetInteger("sprintcrash"));
		par_list.push_back(buffer);
	} else if (gm.optDefined("sifting")) { // synonym for sprintCrash
		par_list.push_back("-sprintCrash");
		sprintf(buffer, "%d", gm.optGetInteger("sifting"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("crash")) {
		char* value=gm.optGetString("crash", buffer);
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

	if (gm.optDefined("maxfactor")) {
		par_list.push_back("-maxFactor");
		sprintf(buffer, "%d", gm.optGetInteger("maxfactor"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("crossover")) { // should be revised if we can do quadratic
		par_list.push_back("-crossover");
		par_list.push_back(gm.optGetBool("crossover") ? "on" : "off");
	}

	if (gm.optDefined("dualpivot")) {
		char* value=gm.optGetString("dualpivot", buffer);
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

	if (gm.optDefined("primalpivot")) {
		char* value=gm.optGetString("primalpivot", buffer);
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
	
	if (gm.optDefined("perturbation")) {
		par_list.push_back("-perturb");
		par_list.push_back(gm.optGetBool("perturbation") ? "on" : "off");
	}
	
	if (gm.optDefined("scaling")) {
		char* value=gm.optGetString("scaling", buffer);
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

	if (gm.optDefined("presolve")) {
		par_list.push_back("-presolve");
		par_list.push_back(gm.optGetBool("presolve") ? "on" : "off");
	}

	if (gm.optDefined("tol_presolve")) {
		par_list.push_back("-preTolerance");
		sprintf(buffer, "%g", gm.optGetDouble("tol_presolve"));
		par_list.push_back(buffer);
	}

	// MIP parameters
	
	if (gm.optDefined("sollim")) {
		par_list.push_back("-maxSolutions");
		sprintf(buffer, "%d", gm.optGetInteger("sollim"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("strongbranching")) {
		par_list.push_back("-strongBranching");
		sprintf(buffer, "%d", gm.optGetInteger("strongbranching"));
		par_list.push_back(buffer);
	}
			
	if (gm.optDefined("trustpseudocosts")) {
		par_list.push_back("-trustPseudoCosts");
		sprintf(buffer, "%d", gm.optGetInteger("trustpseudocosts"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("cutdepth")) {
		par_list.push_back("-cutDepth");
		sprintf(buffer, "%d", gm.optGetInteger("cutdepth"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("cut_passes_root")) {
		par_list.push_back("-passCuts");
		sprintf(buffer, "%d", gm.optGetInteger("cut_passes_root"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("cut_passes_tree")) {
		par_list.push_back("-passTree");
		sprintf(buffer, "%d", gm.optGetInteger("cut_passes_tree"));
		par_list.push_back(buffer);
	}
	
	if (gm.optDefined("cuts")) {
		char* value=gm.optGetString("cuts", buffer);
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
	
	if (gm.optDefined("cliquecuts")) {
		char* value=gm.optGetString("cliquecuts", buffer);
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

	if (gm.optDefined("flowcovercuts")) {
		char* value=gm.optGetString("flowcovercuts", buffer);
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

	if (gm.optDefined("gomorycuts")) {
		char* value=gm.optGetString("gomorycuts", buffer);
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

	if (gm.optDefined("knapsackcuts")) {
		char* value=gm.optGetString("knapsackcuts", buffer);
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

	if (gm.optDefined("liftandprojectcuts")) {
		char* value=gm.optGetString("liftandprojectcuts", buffer);
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

	if (gm.optDefined("mircuts")) {
		char* value=gm.optGetString("mircuts", buffer);
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

	if (gm.optDefined("probingcuts")) {
		char* value=gm.optGetString("probingcuts", buffer);
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

	if (gm.optDefined("reduceandsplitcuts")) {
		char* value=gm.optGetString("reduceandsplitcuts", buffer);
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

	if (gm.optDefined("residualcapacitycuts")) {
		char* value=gm.optGetString("residualcapacitycuts", buffer);
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

	if (gm.optDefined("twomircuts")) {
		char* value=gm.optGetString("twomircuts", buffer);
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

	if (gm.optDefined("heuristics")) {
		par_list.push_back("-heuristicsOnOff");
		par_list.push_back(gm.optGetBool("heuristics") ? "on" : "off");
	}
	
	if (gm.optDefined("combinesolutions")) {
		par_list.push_back("-combineSolution");
		par_list.push_back(gm.optGetBool("combinesolutions") ? "on" : "off");
	}

	if (gm.optDefined("feaspump")) {
		par_list.push_back("-feasibilityPump");
		par_list.push_back(gm.optGetBool("feaspump") ? "on" : "off");
	}

	if (gm.optDefined("feaspump_passes")) {
		par_list.push_back("-passFeasibilityPump");
		sprintf(buffer, "%d", gm.optGetInteger("feaspump_passes"));
		par_list.push_back(buffer);
	}

	if (gm.optDefined("greedyheuristic")) {
		char* value=gm.optGetString("greedyheuristic", buffer);
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

	if (gm.optDefined("localtreesearch")) {
		par_list.push_back("-localTreeSearch");
		par_list.push_back(gm.optGetBool("localtreesearch") ? "on" : "off");
	}

	if (gm.optDefined("rins")) {
		par_list.push_back("-Rins");
		par_list.push_back(gm.optGetBool("rins") ? "on" : "off");
	}

	if (gm.optDefined("roundingheuristic")) {
		par_list.push_back("-roundingHeuristic");
		par_list.push_back(gm.optGetBool("roundingheuristic") ? "on" : "off");
	}
	
	if (gm.optDefined("coststrategy")) {
		char* value=gm.optGetString("coststrategy", buffer);
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
	
	if (gm.optDefined("nodestrategy")) {
		char* value=gm.optGetString("nodestrategy", buffer);
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

	if (gm.optDefined("preprocess")) {
		char* value=gm.optGetString("preprocess", buffer);
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
	} else if (gm.nSemiContinuous()) {
		myout << "CBC integer preprocessing does not handle semicontinuous variables correct (yet), thus we switch it off." << CoinMessageEol;
		par_list.push_back("-preprocess");
		par_list.push_back("off");
	}
	
	if (gm.optDefined("increment")) {
		par_list.push_back("-increment");
		sprintf(buffer, "%g", gm.optGetDouble("increment"));
		par_list.push_back(buffer);
	}
	
	// special options set by user and passed unseen to CBC
	if (gm.optDefined("special")) {
		char longbuffer[10000];
		char* value=gm.optGetString("special", longbuffer);
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

	if (gm.optDefined("startalg")) {
		char* value=gm.optGetString("startalg", buffer);
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
