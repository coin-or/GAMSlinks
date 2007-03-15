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

void write_mps(GamsModel& gm, OsiSolverInterface& solver, GamsMessageHandler& myout, char* filename);

int main (int argc, const char *argv[]) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif

	if (argc==1) {
		fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n",  argv[0]);
		exit(EXIT_FAILURE);
	}	
	int i,j;
	char buffer[255];

	OsiXXXSolverInterface solver;

	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
	GamsModel gm(argv[1],-solver.getInfinity(),solver.getInfinity());

	// Pass in the GAMS status/log file print routines 
	GamsMessageHandler myout(&gm), cbcout(&gm), slvout(&gm);
	slvout.setPrefix(0);
	cbcout.setPrefix(0);
	solver.passInMessageHandler(&slvout);
	solver.setHintParam(OsiDoReducePrint,true,OsiHintTry);
	
#ifdef GAMS_BUILD	
	myout << "\nGAMS/CoinCbc 1.2pre LP/MIP Solver\nwritten by J.Forrest\n " << CoinMessageEol;
	if (!gm.ReadOptionsDefinitions("coincbc"))
#else
	myout << "\nGAMS/Cbc 1.2pre LP/MIP Solver\nwritten by J.Forrest\n " << CoinMessageEol;
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

	// Write MPS file
	if (gm.optDefined("writemps"))
		write_mps(gm, solver, myout, gm.optGetString("writemps", buffer));

	// Some tolerances and limits
#if (OsiXXXSolverInterface == OsiClpSolverInterface)
	solver.getModelPtr()->setDualBound(1.0e10);
	solver.getModelPtr()->setDblParam(ClpMaxSeconds, gm.optGetDouble("reslim"));
#endif
	solver.setDblParam(OsiPrimalTolerance, gm.optGetDouble("tol_primal"));
	solver.setDblParam(OsiDualTolerance, gm.optGetDouble("tol_dual"));
	solver.setHintParam(OsiDoScale, gm.optGetBool("scaling"));
	solver.setHintParam(OsiDoPresolveInInitial, gm.optGetBool("presolve"));

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
			// branch on long sets first
			objects[i-1]->setPriority(gm.nCols()-n);
		}
		for (i=1; i<=gm.nSOS2(); ++i) {
			int n=0;
			for (j=0; j<gm.nCols(); ++j)
				if (gm.SOSIndicator()[j]==-i) which[n++]=j;
			objects[gm.nSOS1()+i-1]=new CbcSOS(&model, n, which, NULL, gm.nSOS1()+i-1, 2);
			// branch on long sets first
			objects[gm.nSOS1()+i-1]->setPriority(gm.nCols()-n);
		}
		delete[] which;
		
	  model.addObjects(gm.nSOS1()+gm.nSOS2(), objects);
	  for (i=0; i<gm.nSOS1()+gm.nSOS2(); ++i)
			delete objects[i];
		delete[] objects;
  }

	
	// Do initial solve to continuous
	if (gm.nDCols() || gm.nSOS1() || gm.nSOS2()) {
	  myout << CoinMessageEol << "Solving the root node..." << CoinMessageEol;
	} else { // iteration limit only for LPs
		model.solver()->setIntParam(OsiMaxNumIteration, gm.optGetInteger("iterlim"));
	}	
	buffer[0]=0; gm.optGetString("startalg", buffer);
	model.solver()->setHintParam(OsiDoDualInInitial, strcmp(buffer, "primal")==0 ? false : true);
	model.solver()->messageHandler()->setLogLevel(4);
	model.initialSolve();

	// If this was an LP or the LP relaxation couldn't be solved we are done
	if (!model.solver()->isProvenOptimal() || (0==gm.nDCols() && 0==gm.nSOS1() && 0==gm.nSOS2())) {
	  // Get some statistics 
	  gm.setIterUsed(model.solver()->getIterationCount());
	  gm.setResUsed(gm.SecondsSinceStart());
	  gm.setObjVal(gm.ObjSense()*model.solver()->getObjValue());
 
	  GamsFinalizeOsi(&gm, &myout, model.solver(),0);
	  return EXIT_SUCCESS;
	}
	
	
	model.setDblParam(CbcModel::CbcMaximumSeconds, gm.optGetDouble("reslim"));
	model.setIntParam(CbcModel::CbcMaxNumNode, gm.optGetInteger("nodelim"));
	model.setDblParam(CbcModel::CbcAllowableGap, gm.optGetDouble("optca"));
	model.setDblParam(CbcModel::CbcAllowableFractionGap, gm.optGetDouble("optcr"));
	if (gm.optDefined("cutoff")) model.setCutoff(gm.ObjSense()*gm.optGetDouble("cutoff")); // Cbc assumes a minimizatio problem here
	model.setDblParam(CbcModel::CbcIntegerTolerance, gm.optGetDouble("tol_integer"));
	model.solver()->setIntParam(OsiMaxNumIterationHotStart,100);

	CbcStrategyGams strategy(gm);
	model.setStrategy(strategy);

	myout << CoinMessageEol << "Starting branch-and-bound..." << CoinMessageEol;
	model.branchAndBound();

	// if the lp solver says feasible, but cbc says infeasible, then the lp solver was probably not called and the model found infeasible in the preprocessing 
	GamsFinalizeOsi(&gm, &myout, model.solver(), model.solver()->isProvenOptimal() && model.isProvenInfeasible());

	return EXIT_SUCCESS;
}

void write_mps(GamsModel& gm, OsiSolverInterface& solver, GamsMessageHandler& myout, char* filename) {
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

  myout << "\nWriting MPS file " << filename << "... " << CoinMessageEol;
  solver.writeMpsNative(filename,rownames,colnames,0,2,gm.ObjSense());

  // We don't need these guys anymore
  for (j=gm.nRows()-1; j>=0; j--)
    free((void*)rownames[j]);
  for (j=gm.nCols()-1; j>=0; j--)
    free((void*)colnames[j]);
  delete[] rownames;
  delete[] colnames;
}
