// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author:  Stefan Vigerske
//
// This CbcStrategy is copied in large portions from John Forrest's CbcStrategyDefault.

#include "CbcStrategyGams.hpp"

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

// for CoinAbs 
#include "CoinHelperFunctions.hpp"

#include "OsiSolverInterface.hpp"
#ifdef COIN_HAS_CLP
#include "OsiClpSolverInterface.hpp"
#endif
#include "CbcModel.hpp"
#include "CbcMessage.hpp"
#include "CbcStrategy.hpp"
#include "CbcCutGenerator.hpp"
#include "CbcBranchActual.hpp"
#include "CbcNode.hpp"
#include "CoinWarmStart.hpp"
#include "CglPreProcess.hpp"

// Cuts
#include "CglGomory.hpp"
#include "CglProbing.hpp"
#include "CglKnapsackCover.hpp"
#include "CglOddHole.hpp"
#include "CglClique.hpp"
#include "CglFlowCover.hpp"
#include "CglMixedIntegerRounding2.hpp"
#include "CglRedSplit.hpp"

// Heuristics
#include "CbcHeuristic.hpp"
#include "CbcHeuristicLocal.hpp"

// Comparison methods
#include "CbcCompareActual.hpp"

CbcStrategyGams::CbcStrategyGams(GamsModel& gm_)
: CbcStrategy(),
	gm(gm_)
{ }

// Copy constructor 
CbcStrategyGams::CbcStrategyGams(const CbcStrategyGams & rhs)
: CbcStrategy(rhs),
	gm(rhs.gm)
{ setNested(rhs.getNested());
}

// Clone
CbcStrategy* CbcStrategyGams::clone() const {
	return new CbcStrategyGams(*this);
}

// Setup cut generators
void CbcStrategyGams::setupCutGenerators(CbcModel & model) {
	int cutparam=gm.optGetInteger("cuts");
	if (cutparam==-1) return; // user want no cuts

	// Set up some cut generators and defaults
	int howoften=gm.optGetBool("cutsonlyatroot") ? -99 : -1;

	// Probing first as gets tight bounds on continuous
	if (cutparam==1 || gm.optGetBool("probing")) {
		CglProbing generator;
		generator.setUsingObjective(true);
		generator.setMaxPass(1);
		// Number of unsatisfied variables to look at
		generator.setMaxProbe(10);
		// How far to follow the consequences
		generator.setMaxLook(10);
		// Only look at rows with fewer than this number of elements
		generator.setMaxElements(200);
		//generator1.setRowCuts(3);
		model.addCutGenerator(&generator,howoften,"Probing");
	}
	
	if (cutparam==1 || gm.optGetBool("gomorycuts")) {
		CglGomory generator;
		// try larger limit
		generator.setLimit(300);
		model.addCutGenerator(&generator,howoften,"Gomory");
	}

	if (cutparam==1 || gm.optGetBool("knapsackcuts")) {
		CglKnapsackCover generator;
		model.addCutGenerator(&generator,howoften,"Knapsack");
	}
	
	if (cutparam==1 || gm.optGetBool("oddholecuts")) {
		CglOddHole generator;
		generator.setMinimumViolation(0.005);
		generator.setMinimumViolationPer(0.00002);
		// try larger limit
		generator.setMaximumEntries(200);
		model.addCutGenerator(&generator,howoften,"OddHole");
	}

	if (cutparam==1 || gm.optGetBool("cliquecuts")) {
		CglClique generator;
		generator.setStarCliqueReport(false);
		generator.setRowCliqueReport(false);
		model.addCutGenerator(&generator,howoften,"Clique");
	}

	if (cutparam==1 || gm.optGetBool("flowcovercuts")) {
		CglFlowCover generator;
		model.addCutGenerator(&generator,howoften,"FlowCover");
	}

	if (cutparam==1 || gm.optGetBool("mircuts")) {
		CglMixedIntegerRounding2 generator;
		model.addCutGenerator(&generator,howoften,"MixedIntegerRounding2");
	}

	if (cutparam==1 || gm.optGetBool("redsplitcuts")) {
		CglRedSplit generator;
		model.addCutGenerator(&generator,howoften,"RedSplit");
	}

	// Say we want timings
	int NumberGenerators = model.numberCutGenerators();
	for (int iGenerator=0; iGenerator<NumberGenerators; ++iGenerator)
		model.cutGenerator(iGenerator)->setTiming(true);
		
	if (model.getNumCols()<500)
	  model.setMaximumCutPassesAtRoot(-100); // always do 100 if possible
	else if (model.getNumCols()<5000)
	  model.setMaximumCutPassesAtRoot(100); // use minimum drop
	else
	  model.setMaximumCutPassesAtRoot(20);
	model.setMaximumCutPasses(10);

	// minimum drop to continue cuts
	model.setMinimumDrop(min(1.0, CoinAbs(model.getObjValue())*1.0e-3+1.0e-4));
}

// Setup heuristics
void CbcStrategyGams::setupHeuristics(CbcModel& model) {
	if (gm.optGetBool("roundingheuristic")) {
		CbcRounding heuristic(model);
		model.addHeuristic(&heuristic);
	}

	if (gm.optGetBool("localsearch")) {
		CbcHeuristicLocal heuristic(model);
		model.addHeuristic(&heuristic);
	}
}

// Do printing stuff
void CbcStrategyGams::setupPrinting(CbcModel& model, int modelLogLevel) {
	if (!modelLogLevel) {
		model.solver()->setHintParam(OsiDoReducePrint,true,OsiHintTry);
		model.messageHandler()->setLogLevel(0);
		model.solver()->messageHandler()->setLogLevel(0);
	} else if (modelLogLevel==1) {
		model.solver()->setHintParam(OsiDoReducePrint,true,OsiHintTry);
		model.messageHandler()->setLogLevel(1);
		model.solver()->messageHandler()->setLogLevel(0);
	} else {
		if (gm.getSysOut()) {
		  model.messageHandler()->setLogLevel(4);
		  model.solver()->messageHandler()->setLogLevel(4);
		}	else {
		  model.messageHandler()->setLogLevel(2);
		  model.solver()->messageHandler()->setLogLevel(1);
		}
	}
	
	model.setPrintFrequency(gm.optGetInteger("printfrequency"));
}

// Other stuff e.g. strong branching
void CbcStrategyGams::setupOther(CbcModel & model) {
	// See if preprocessing wanted
	if (gm.optGetBool("integerpresolve")) {
		delete process_;
		// solver_ should have been cloned outside
		CglPreProcess* process = new CglPreProcess();
		// Pass in models message handler
		process->passInMessageHandler(model.messageHandler());
		OsiSolverInterface* solver = model.solver();
		int logLevel = model.messageHandler()->logLevel();
#ifdef COIN_HAS_CLP
		OsiClpSolverInterface* clpSolver = dynamic_cast<OsiClpSolverInterface*>(solver);
		ClpSimplex* lpSolver=NULL;
		if (clpSolver) {
			if (clpSolver->messageHandler()->logLevel())
				clpSolver->messageHandler()->setLogLevel(1);
			if (logLevel>-1)
				clpSolver->messageHandler()->setLogLevel(CoinMin(logLevel,clpSolver->messageHandler()->logLevel()));
			lpSolver = clpSolver->getModelPtr();
			// If user left factorization frequency then compute
			lpSolver->defaultFactorizationFrequency();
		}
#endif
		// Tell solver we are in Branch and Cut
		solver->setHintParam(OsiDoInBranchAndCut,true,OsiHintDo);
		// Default set of cut generators
		CglProbing generator1;
		generator1.setUsingObjective(true);
		generator1.setMaxPass(3);
		generator1.setMaxProbeRoot(solver->getNumCols());
		generator1.setMaxElements(100);
		generator1.setMaxLookRoot(50);
		generator1.setRowCuts(3);
		//generator1.messageHandler()->setLogLevel(logLevel);
		// Not needed with pass in process->messageHandler()->setLogLevel(logLevel);
		// Add in generators
		process->addCutGenerator(&generator1);
		int findsos=gm.optGetBool("findsos") ? 2 : 0;
		OsiSolverInterface* solver2 = process->preProcessNonDefault(*solver,findsos,10);
  	// Tell solver we are not in Branch and Cut
		solver->setHintParam(OsiDoInBranchAndCut,false,OsiHintDo);
		if (solver2) solver2->setHintParam(OsiDoInBranchAndCut,false,OsiHintDo);
		bool feasible=true;
		if (!solver2) {
			feasible = false;
			delete process;
			preProcessState_=-1;
			process_=NULL;
		} else {
			// now tighten bounds
#ifdef COIN_HAS_CLP
			if (clpSolver) {
				// model has changed
				solver = model.solver();
				OsiClpSolverInterface * clpSolver = dynamic_cast<OsiClpSolverInterface*>(solver);
				ClpSimplex* lpSolver = clpSolver->getModelPtr();
				if (lpSolver->tightenPrimalBounds()==0) {
					lpSolver->passInMessageHandler(solver->messageHandler());
					lpSolver->dual();
				} else {
					feasible = false;
				}
			}
#endif
			if (feasible) {
				preProcessState_=1;
				process_=process;
				/* Note that original solver will be kept (with false)
					 and that final solver will also be kept.
					 This is for post-processing
				*/
				OsiSolverInterface* solver3 = solver2->clone();
				model.assignSolver(solver3,false);
				if (process_->numberSOS()) {
					int numberSOS = process_->numberSOS();
					int numberIntegers = model.numberIntegers();
					/* model may not have created objects
						 If none then create
						 NOTE - put back to original column numbers as 
						 CbcModel will pack down ALL as it doesn't know where from
					*/
					bool someObjects = model.numberObjects()>0;
					if (!numberIntegers||!model.numberObjects()) {
						model.findIntegers(true);
						numberIntegers = model.numberIntegers();
					}
					CbcObject** oldObjects = model.objects();
					// Do sets and priorities
					CbcObject** objects = new CbcObject*[numberSOS];
					// set old objects to have low priority
					int numberOldObjects = model.numberObjects();
					int numberColumns = model.getNumCols();
					for (int iObj = 0;iObj<numberOldObjects;iObj++) {
						int oldPriority = oldObjects[iObj]->priority();
						oldObjects[iObj]->setPriority(numberColumns+oldPriority);
					}
					const int* starts = process_->startSOS();
					const int* which = process_->whichSOS();
					const int* type = process_->typeSOS();
					const double* weight = process_->weightSOS();
					int iSOS;
					for (iSOS =0;iSOS<numberSOS;iSOS++) {
						int iStart = starts[iSOS];
						int n=starts[iSOS+1]-iStart;
						objects[iSOS] = new CbcSOS(&model,n,which+iStart,weight+iStart,iSOS,type[iSOS]);
						// branch on long sets first
						objects[iSOS]->setPriority(numberColumns-n);
					}
					model.addObjects(numberSOS,objects);
					for (iSOS=0;iSOS<numberSOS;iSOS++)
						delete objects[iSOS];
					delete [] objects;
					if (!someObjects) {
						// put back old column numbers
						const int* originalColumns = process_->originalColumns();
						// use reverse lookup to fake it
						int n=originalColumns[numberColumns-1]+1;
						int* fake = new int[n];
						for (int i=0;i<n;i++)
							fake[i]=-1;
						for (int i=0;i<numberColumns;i++)
							fake[originalColumns[i]]=i;
						for (int iObject=0;iObject<model.numberObjects();iObject++) {
							// redo ids etc
							model.modifiableObject(iObject)->redoSequenceEtc(&model,n,fake);
						}
						delete [] fake;
					}
				}
			} else {
				delete process;
				preProcessState_=-1;
				process_=NULL;
			}
		}
	}
	
	if (gm.optGetBool("strongbranching"))
		if (model.getNumCols()<5000)
			model.setNumberStrong(10);
		else
			model.setNumberStrong(5);
	else
		model.setNumberStrong(0);
	model.setNumberBeforeTrust(5);
	
	char compareopt[255];
	gm.optGetString("nodecompare", compareopt);
	if (strcmp(compareopt, "depth")==0) {
		CbcCompareDepth comparedepth;
		model.setNodeComparison(&comparedepth);
	} else if (strcmp(compareopt, "objective")==0) {
		CbcCompareObjective compareobjective;
		model.setNodeComparison(&compareobjective);
	} else {
		// default comparision method: nothing to do
	}
	//TODO: add CbcCompareEstimate in case that pseudo costs are available

}

// Create C++ lines to get to current state
void CbcStrategyGams::generateCpp( FILE * fp) {
	fprintf(fp,"0#include \"CbcStrategy.hpp\"\n");
	fprintf(fp,"3 CbcStrategyGams strategy(gm);\n");
}
