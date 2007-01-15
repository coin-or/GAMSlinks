// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "GAMSlinksConfig.h"
#include <iostream>

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsFinalize.hpp"

// For Branch and bound
#include "CbcModel.hpp"
#include "CbcBranchActual.hpp"
#include "CbcStrategy.hpp"
#include "CbcBranchUser.hpp"
#include "CbcCompareUser.hpp"
// Heuristics
#include "CbcHeuristic.hpp"
#include "CbcHeuristicLocal.hpp"

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

// Cuts
#include "CglPreProcess.hpp"
#include "CglGomory.hpp"
#include "CglProbing.hpp"
#include "CglKnapsackCover.hpp"
#include "CglOddHole.hpp"
#include "CglRedSplit.hpp"
#include "CglClique.hpp"
#include "CglFlowCover.hpp"
#include "CglMixedIntegerRounding2.hpp"

void write_mps(GamsModel& gm, OsiSolverInterface& solver, GamsMessageHandler& myout);

/**
This main program copied in large portions from John Forrest's original
sample2.cpp.

It sets up some Cgl cut generators and calls branch and cut.

Branching is simple binary branching on integer variables.

Node selection is depth first until first solution is found and then based on
objective and number of unsatisfied integer variables.  In this example the
functionality is the same as default but it is a user comparison function.

Variable branching selection is on maximum minimum-of-up-down change after
strong branching on 5 variables closest to 0.5.

A simple rounding heuristic is used.
*/
int main (int argc, const char *argv[]) {
	if (argc==1) {
		std::cerr << "usage: " << argv[0] << " <gams-control-file>" << std::endl;
		exit(EXIT_FAILURE);
	}	
	int i,j;

	OsiXXXSolverInterface solver;

	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
	GamsModel gm(argv[1],solver.getInfinity());

	// Pass in the GAMS status/log file print routines 
	GamsMessageHandler myout, cbcout, slvout;
	myout.setGamsModel(&gm);
	slvout.setGamsModel(&gm); slvout.setPrefix(0);
	cbcout.setGamsModel(&gm); cbcout.setPrefix(0);
	solver.passInMessageHandler(&slvout);
	solver.setHintParam(OsiDoReducePrint,true,OsiHintTry);
	
	myout << "\nGAMS/CoinCbc Lp/Mip Solver\nwritten by J.Forrest\n " << CoinMessageEol;
	
//	char options[][32]={"cut generation", "rounding heuristic", "local search", "strong branching", "integer presolve"};
//	char cutnames[][32]={"Probing", "Gomory", "Knapsack", "OddHole", "Clique", "FlowCover", "MIR", "RedSplit"};
//
//	myout << "\nOptions:" << CoinMessageEol;
//	for (i=0; i<4; i++) { // We don't want to advertise integer presolve
//	  int onoff=gm.getGamsSwitch(1,i)? 0:1;
//    myout << "  " << options[i] << "\t";
//	  if (0==i && onoff) {
//	    for (j=0; j<8; j++) 
//	    	switch (j) {
//	    		case 3: case 7: 
//				    if (1==gm.getGamsSwitch(2,j)) myout << cutnames[j];
//				    break;
//				  default:
//	    			if (0==gm.getGamsSwitch(2,j)) myout << cutnames[j];
//	  		}
//	    myout << CoinMessageEol;
//	  } else
//	    myout << (onoff? "on":"off") << CoinMessageEol;      
//	}
	gm.TimerStart();

	// CLP needs rowrng for the loadProblem call
	double *rowrng = new double[gm.nRows()];
	for (i=0; i<gm.nRows(); i++)
	  rowrng[i] = 0.0;

	solver.setObjSense(gm.ObjSense());
	solver.setDblParam(OsiObjOffset, gm.ObjRhs()); // obj constant

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

	// Write MPS file
	if (gm.getGamsInteger(0))
		write_mps(gm, solver, myout);

	// Some tolerances and limits
	solver.setIntParam(OsiMaxNumIteration, gm.getIterLim());
#if (OsiXXXSolverInterface == OsiClpSolverInterface)
	solver.getModelPtr()->setDualBound(1.0e10);
#endif

	CbcModel model(solver);
  // Switch off most output
	model.solver()->setHintParam(OsiDoReducePrint,true,OsiHintTry);
	model.passInMessageHandler(&cbcout);

	// Tell solver which variables belong to SOS of type 1 or 2
	if (gm.nSOS1() || gm.nSOS2()) {
		CbcObject** objects = new CbcObject*[gm.nSOS1()+gm.nSOS2()];
		
		int* which = new int[gm.nCols()];
		for (i=1; i<=gm.nSOS1(); ++i) {
			int n=0;
			for (j=0; j<gm.nCols(); ++j)
				if (gm.SOSIndicator()[j]==i) which[n++]=j;
			objects[i-1]=new CbcSOS(&model, n, which, NULL, i-1, 1);
		}
		for (i=1; i<=gm.nSOS2(); ++i) {
			int n=0;
			for (j=0; j<gm.nCols(); ++j)
				if (gm.SOSIndicator()[j]==-i) which[n++]=j;
			objects[gm.nSOS1()+i-1]=new CbcSOS(&model, n, which, NULL, gm.nSOS1()+i-1, 2);
		}
		delete[] which;
		
	  model.addObjects(gm.nSOS1()+gm.nSOS2(), objects);
	  for (i=0; i<gm.nSOS1()+gm.nSOS2(); ++i)
			delete objects[i];
		delete[] objects;
  }

	model.setIntParam(CbcModel::CbcMaxNumNode, gm.getNodeLim());
	model.setDblParam(CbcModel::CbcMaximumSeconds, gm.getResLim());
	model.setDblParam(CbcModel::CbcAllowableGap, gm.getOptCA());
	model.setCutoff(gm.getCutOff());

	// Set up some cut generators and defaults
	// Probing first as gets tight bounds on continuous
	CglProbing generator1;
	generator1.setUsingObjective(true);
	generator1.setMaxPass(1);
	generator1.setMaxPassRoot(5);
//	generator1.setMaxProbe(10);
	generator1.setMaxProbe(1000);
	generator1.setMaxLook(50);
	generator1.setMaxLookRoot(500);
	generator1.setMaxElements(200);
	generator1.setRowCuts(3);

	CglGomory generator2;
	generator2.setLimit(300);   // try larger limit

	CglKnapsackCover generator3;

	CglOddHole generator4;
	generator4.setMinimumViolation(0.005);
	generator4.setMinimumViolationPer(0.00002);
	generator4.setMaximumEntries(200);   // try larger limit

	CglClique generator5;
	generator5.setStarCliqueReport(false);
	generator5.setRowCliqueReport(false);

	CglRedSplit generator6;
	// try larger limit
	generator6.setLimit(200);

	CglMixedIntegerRounding2 mixedGen;
	CglFlowCover flowGen;
	
	if (0==gm.getGamsSwitch(1,0)) {  // m.integer1 = 1 turns cut generation off
	  // Add in generators
	  if (0==gm.getGamsSwitch(2,0)) model.addCutGenerator(&generator1,-1,"Probing");
	  if (0==gm.getGamsSwitch(2,1)) model.addCutGenerator(&generator2,-1,"Gomory");
	  if (0==gm.getGamsSwitch(2,2)) model.addCutGenerator(&generator3,-1,"Knapsack");
	  if (1==gm.getGamsSwitch(2,3)) model.addCutGenerator(&generator4,-1,"OddHole");
	  if (0==gm.getGamsSwitch(2,4)) model.addCutGenerator(&generator5,-1,"Clique");
	  if (0==gm.getGamsSwitch(2,5)) model.addCutGenerator(&flowGen,-1,"FlowCover");
	  if (0==gm.getGamsSwitch(2,6)) model.addCutGenerator(&mixedGen,-1,"MixedIntegerRounding2");
	  if (1==gm.getGamsSwitch(2,7)) model.addCutGenerator(&generator6,-1,"RedSplit");
	}

	// Rounding heuristic
	CbcRounding heuristic1(model);
	// m.integer2 = 1 turns rounding heuristic off
	if (0==gm.getGamsSwitch(1,1)) 
	  model.addHeuristic(&heuristic1); 

	// And local search when new solution found
	CbcHeuristicLocal heuristic2(model);
	// m.integer3 = 1 turns local search off
	if (0==gm.getGamsSwitch(1,2)) 
	  model.addHeuristic(&heuristic2); 

	// Redundant definition of default branching (as Default == User)
	CbcBranchUserDecision branch;
	model.setBranchingMethod(&branch);

	// Definition of node choice
	CbcCompareUser compare;
	model.setNodeComparison(compare);

	// Do initial solve to continuous
	model.solver()->messageHandler()->setLogLevel(2);
	if (gm.nDCols() || gm.nSOS1() || gm.nSOS2())
	  myout << "\nSolving the root node..." << CoinMessageEol;
	model.initialSolve();

	if (0==gm.nDCols() && 0==gm.nSOS1() && 0==gm.nSOS2()) {  // If this was an LP we are done
	  // Get some statistics 
	  gm.setIterUsed(model.solver()->getIterationCount());
	  gm.setResUsed(gm.SecondsSinceStart());
	  gm.setObjVal(gm.ObjSense()*model.solver()->getObjValue());
 
	  GamsFinalizeOsi(&gm, &myout, model.solver(),0);
	  return 0;
	}
	
	// minimum drop to continue cuts
	model.setMinimumDrop(min(1.0, fabs(model.getObjValue())*1.0e-3+1.0e-4));

	if (model.getNumCols()<500)
	  model.setMaximumCutPassesAtRoot(-100); // always do 100 if possible
	else if (model.getNumCols()<5000)
	  model.setMaximumCutPassesAtRoot(100); // use minimum drop
	else
	  model.setMaximumCutPassesAtRoot(20);
	model.setMaximumCutPasses(10);

	// Do more strong branching if small
	if (model.getNumCols()<5000)
	  model.setNumberStrong(10);
	model.setNumberBeforeTrust(5);

	// m.integer4 = 1 turns strong branching off
	if (1==gm.getGamsSwitch(1,3)) model.setNumberStrong(0);

	model.solver()->setIntParam(OsiMaxNumIterationHotStart,100);

	if (gm.getSysOut()) {
	  model.messageHandler()->setLogLevel(4);
	  model.solver()->messageHandler()->setLogLevel(4);
	}	else {
	  model.messageHandler()->setLogLevel(2);
	  model.solver()->messageHandler()->setLogLevel(0);
	}
	model.setPrintFrequency(10);

	// Default strategy will leave cut generators as they exist already
	// so cutsOnlyAtRoot (1) ignored
	// numberStrong (2) is 5 (default)
	// numberBeforeTrust (3) is 5 (default is 0)
	// printLevel (4) defaults (0)
	CbcStrategyDefault strategy(true,5,5);
	// Set up pre-processing to find sos if wanted
	if (0==gm.getGamsSwitch(1,4))
	  strategy.setupPreProcessing(2);
	model.setStrategy(strategy);
	
	myout << "Starting branch-and-bound..." << CoinMessageEol;
	model.branchAndBound();

	myout << "Cbc Status: " << model.status() << CoinMessageEol;
  
	GamsFinalizeOsi(&gm, &myout, model.solver(), 0);

	return 0;
}    

void write_mps(GamsModel& gm, OsiSolverInterface& solver, GamsMessageHandler& myout) {
	char namebuf[10];
	const char **colnames=new const char *[gm.nCols()];
	const char **rownames=new const char *[gm.nRows()];
	int j;

  for (j=0; j<gm.nCols(); j++) {
    sprintf(namebuf,"X%d",j);
    colnames[j]=strdup(namebuf);
  }

  for (j=0; j<gm.nRows(); j++) {
    sprintf(namebuf,"E%d",j);
    rownames[j] = strdup(namebuf); 
  }

  myout << "\nWriting MPS file coinprob.mps... " << CoinMessageEol;
  solver.writeMpsNative("coinprob.mps",rownames,colnames,0,2,gm.ObjSense());

  // We don't need these guys anymore
  for (j=gm.nRows()-1; j>=0; j--)
    free((void*)rownames[j]);
  for (j=gm.nCols()-1; j>=0; j--)
    free((void*)colnames[j]);
  delete[] rownames;
  delete[] colnames;
}
