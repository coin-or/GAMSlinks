// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsCoinGlpk.cpp 157 2007-08-04 19:22:29Z stefan $
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

#include "OsiSolverInterface.hpp"
#include "CoinPackedVector.hpp"

#if COIN_HAS_CLP
#include "OsiClpSolverInterface.hpp"
#endif
#if COIN_HAS_GLPK
#include "OsiGlpkSolverInterface.hpp"
#endif
#if COIN_HAS_VOL
#include "OsiVolSolverInterface.hpp"
#endif
#if COIN_HAS_DYLP
#include "OsiDylpSolverInterface.hpp"
#endif
#if COIN_HAS_SYMPHONY
#include "OsiSymSolverInterface.hpp"
#endif

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsFinalize.hpp"

//int printme(void* info, const char* msg) {
//	GamsMessageHandler* myout=(GamsMessageHandler*)info;
//	assert(myout);
//
//	*myout << msg;
//	if (*msg && msg[strlen(msg)-1]=='\n')
//		*myout << CoinMessageEol; // this will make the message handler actually print the message
//
//	return 1;
//}

void setupParameters(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver);
void setupParametersMIP(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver);
void setupStartPoint(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver);

// TODO: catch CoinError, might be thrown if illegal parameter is set

int main (int argc, const char *argv[]) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif

	if (argc==1) {
		fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n",  argv[0]);
		exit(EXIT_FAILURE);
	}
	int j;
	char buffer[255];
	
	double infty=COIN_DBL_MAX;
//	double infty=OsiVolInfinity;
//	double infty=sym_get_infinity();
	
	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
//	GamsModel gm(argv[1],-solver->getInfinity(),solver->getInfinity());
	GamsModel gm(argv[1], -infty, infty);

	// Pass in the GAMS status/log file print routines
	GamsMessageHandler myout(&gm), slvout(&gm);
	slvout.setPrefix(0);

#ifdef GAMS_BUILD
	myout << "\nGAMS/CoinOsi LP/MIP Solver" << CoinMessageEol;
#else
	myout << "\nGAMS/Osi LP/MIP Solver" << CoinMessageEol;
#endif

	if (gm.nSOS1() || gm.nSOS2()) {
		myout << "OSI cannot handle special ordered sets (SOS)" << CoinMessageEol;
		myout << "Exiting ..." << CoinMessageEol;
		exit(EXIT_FAILURE);
	}

#ifdef GAMS_BUILD
	if (!gm.ReadOptionsDefinitions("coinosi"))
#else
	if (!gm.ReadOptionsDefinitions("osi"))
#endif
		myout << "Error intializing option file handling or reading option file definitions!" << CoinMessageEol
			<< "Processing of options is likely to fail!" << CoinMessageEol;
	gm.ReadOptionsFile();

	OsiSolverInterface* solver=NULL;
	if (!gm.optDefined("solver")) {
		gm.optSetString("solver", "clp");
		//TODO: be more relaxed
//		myout << "Error: You need to specify a solver." << CoinMessageEol;
//		exit(EXIT_FAILURE);
	}
	gm.optGetString("solver", buffer);
	
try {
//TODO: we should ignore the case of the given strings
#if COIN_HAS_CLP
	if (strcasecmp(buffer, "clp")==0) {
		solver=new OsiClpSolverInterface();
		myout << "Using Clp solver." << CoinMessageEol;
	}
#endif
#if COIN_HAS_GLPK
	if (strcasecmp(buffer, "glpk")==0) {
		solver=new OsiGlpkSolverInterface();
		myout << "Using Glpk solver." << CoinMessageEol;
	}
#endif
#if COIN_HAS_VOL
	if (strcasecmp(buffer, "volume")==0) {
		solver=new OsiVolSolverInterface();
		myout << "Using Volume solver." << CoinMessageEol;
	}
#endif
#if COIN_HAS_DYLP
	if (strcasecmp(buffer, "dylp")==0) {
		solver=new OsiDylpSolverInterface();
		myout << "Using DyLP solver." << CoinMessageEol;
	}
#endif
#if COIN_HAS_SYMPHONY
	if (strcasecmp(buffer, "symphony")==0) {
		solver=new OsiSymSolverInterface();
		myout << "Using SYMPHONY solver." << CoinMessageEol;
	}
#endif
	if (!solver) {
		myout << "Solver " << buffer << " not recognized or not available." << CoinMessageEol;
		exit(EXIT_FAILURE);
	}
	
	assert(solver->getInfinity()==infty);

//	glp_term_hook(printme, &slvout);
	solver->passInMessageHandler(&slvout);

	/* Overwrite GAMS Options */
	if (!gm.optDefined("reslim")) gm.optSetDouble("reslim", gm.getResLim());
	if (!gm.optDefined("iterlim")) gm.optSetInteger("iterlim", gm.getIterLim());
//	if (!gm.optDefined("optcr")) gm.optSetDouble("optcr", gm.getOptCR());
//	if (!gm.optDefined("cutoff") && gm.getCutOff()!=gm.ObjSense()*solver.getInfinity()) gm.optSetDouble("cutoff", gm.getCutOff());

	gm.TimerStart();

	// OsiSolver needs rowrng for the loadProblem call
	double *rowrng = new double[gm.nRows()];
	for (j=0; j<gm.nRows(); ++j) rowrng[j] = 0.0;

	// some solvers do not like zeros in the problem matrix
	gm.matSqueezeZeros();

	solver->setDblParam(OsiObjOffset, gm.ObjConstant()); // obj constant

	solver->loadProblem(gm.nCols(), gm.nRows(), gm.matStart(),
															gm.matRowIdx(), gm.matValue(),
															gm.ColLb(), gm.ColUb(), gm.ObjCoef(),
															gm.RowSense(), gm.RowRhs(), rowrng);
	solver->setObjSense(gm.ObjSense());

	// We don't need these guys anymore
	delete[] rowrng;

	// Tell solver which variables are discrete
	int *discVar=gm.ColDisc();
	for (j=0; j<gm.nCols(); j++)
		if (discVar[j]) solver->setInteger(j);

	// why some LP solver cannot minimize a linear function over a box?
	if (!gm.nRows()) {
		myout << "Problem has no rows. Adding fake row..." << CoinMessageEol;
		if (!gm.nCols()) {
			CoinPackedVector vec(0);
			solver->addCol(vec, -solver->getInfinity(), solver->getInfinity(), 0.);
		}
		int index=0; double coeff=1; //TODO: there need to be one variable !!
		CoinPackedVector vec(1, &index, &coeff);
		solver->addRow(vec, -solver->getInfinity(), solver->getInfinity());
	}

	if (gm.haveNames()) { // set variable and constraint names
		solver->setIntParam(OsiNameDiscipline, 2);
		std::string stbuffer;
		for (j=0; j<gm.nCols(); ++j)
			if (gm.ColName(j, buffer, 255)) {
				stbuffer=buffer;
				solver->setColName(j, stbuffer);
			}
		for (j=0; j<gm.nRows(); ++j)
			if (gm.RowName(j, buffer, 255)) {
				stbuffer=buffer;
				solver->setRowName(j, stbuffer);
			}
		if (!gm.nRows()) {
			stbuffer="fakerow";
			solver->setRowName(j, stbuffer);
		}
	}

//	LPX* glpk_model=solver->getModelPtr();

	// Write MPS file
	if (gm.optDefined("writemps")) {
		gm.optGetString("writemps", buffer);
  	myout << "\nWriting MPS file " << buffer << "... " << CoinMessageEol;
  	solver->writeMps(buffer);
  	//TODO
//		lpx_write_mps(glpk_model, buffer);
	}
	
	setupParameters(gm, myout, *solver);
	setupStartPoint(gm, myout, *solver);

	myout << CoinMessageNewline << CoinMessageEol;
	if (gm.nDCols()==0) { // LP

		myout << "Starting LP solver..." << CoinMessageEol;
		solver->initialSolve();

	} else { // MIP
		setupParametersMIP(gm, myout, *solver);

		myout << "Starting MIP solver... " << CoinMessageEol;
		solver->branchAndBound();

//		int mipstat = lpx_mip_status(glpk_model);
		if (solver->isProvenOptimal() || solver->isIterationLimitReached()) {
			const double *colLevel=solver->getColSolution();
			// We might loose colLevel after a call to setColBounds. So we save the levels.
			double *colLevelsav = new double[gm.nCols()];
			for (j=0; j<gm.nCols(); j++) colLevelsav[j] = colLevel[j];

			// No iteration limit for fixed run and special time limit
			// TODO
//			lpx_set_real_parm(glpk_model, LPX_K_TMLIM, gm.optGetDouble("reslim_fixedrun"));
//			lpx_set_int_parm(glpk_model, LPX_K_ITLIM, -1);

			for (j=0; j<gm.nCols(); j++)
				if (discVar[j])
					solver->setColBounds(j,colLevelsav[j],colLevelsav[j]);
			delete[] colLevelsav;

			solver->setHintParam(OsiDoReducePrint, true, OsiHintTry); // loglevel 1
			myout << "\nSolving fixed problem... " << CoinMessageEol;
			solver->resolve();
			if (!solver->isProvenOptimal())
				myout << "Problems solving fixed problem. No solution returned." << CoinMessageEol;
		}
	}

	// Determine status and write solution
	GamsFinalizeOsi(&gm, &myout, solver, 0, false);

} catch (CoinError error) {
	myout << "We got following error:" << error.message() << CoinMessageEol;
	myout << "at:" << error.fileName() << ":" << error.className() << "::" << error.methodName() << CoinMessageEol;
}

	return 0;
}

void setupParameters(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver) {
	// Some tolerances and limits
	solver.setIntParam(OsiMaxNumIteration, gm.optGetInteger("iterlim"));
//	lpx_set_real_parm(glpk_model, LPX_K_TMLIM, gm.optGetDouble("reslim"));

	if (!solver.setDblParam(OsiDualTolerance, gm.optGetDouble("tol_dual")))
		myout << "Failed to set dual tolerance to " << gm.optGetDouble("tol_dual") << CoinMessageEol;

	if (!solver.setDblParam(OsiPrimalTolerance, gm.optGetDouble("tol_primal")))
		myout << "Failed to set primal tolerance to " << gm.optGetDouble("tol_dual") << CoinMessageEol;

//	// more parameters
//	gm.optGetString("scaling", buffer);
//	if (strcmp(buffer, "off")==0)
//		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 0);
//	else if (strcmp(buffer, "equilibrium")==0)
//		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 1);
//	else if (strcmp(buffer, "mean")==0)
//		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 2);
//	else if (strcmp(buffer, "meanequilibrium")==0)
//		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 3); // default (set in OsiGlpk)

	char buffer[255];
	gm.optGetString("startalg", buffer);
	solver.setHintParam(OsiDoDualInInitial, (strcmp(buffer, "dual")==0), OsiHintDo);

	solver.setHintParam(OsiDoPresolveInInitial, gm.optGetBool("presolve"), OsiHintDo);

//	solver.setHintParam(OsiDoReducePrint, true, OsiHintDo); // GLPK loglevel 3
}

void setupParametersMIP(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver) {
//	char buffer[255];

	// cutindicator overwritten by OsiGlpkSolverInterface
//		int cutindicator=0;
//		switch (gm.optGetInteger("cuts")) {
//			case -1 : break; // no cuts
//			case  1 : cutindicator=LPX_C_ALL; break; // all cuts
//			case  0 : // user defined cut selection
//				if (gm.optGetBool("covercuts")) cutindicator|=LPX_C_COVER;
//				if (gm.optGetBool("cliquecuts")) cutindicator|=LPX_C_CLIQUE;
//				if (gm.optGetBool("gomorycuts")) cutindicator|=LPX_C_GOMORY;
//				break;
//			default: ;
//		};
//		lpx_set_int_parm(glpk_model, LPX_K_USECUTS, cutindicator);

	// cutoff do not seem to work in Branch&Bound
//		if (gm.optDefined("cutoff")) {
//			solver.setDblParam(OsiPrimalObjectiveLimit, gm.optGetDouble("cutoff"));
//			myout << "OBJLL: " << lpx_get_real_parm(glpk_model, LPX_K_OBJLL)
//				<< "OBJUL: " << lpx_get_real_parm(glpk_model, LPX_K_OBJUL) << CoinMessageEol;
//		}

	// not sure that optcr (=relative gap tolerance) is the same as TOLOBJ in GLPK
//		double optcr=max(1e-7,gm.optGetDouble("optcr"));
//		if (optcr>0.001) {
//			myout << "Cannot use optcr of larger then 0.001. Setting objective tolerance to 0.001." << CoinMessageEol;
//			optcr=0.001;
//		}
//		lpx_set_real_parm(glpk_model, LPX_K_TOLOBJ, optcr);

}

void setupStartPoint(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver) {
//	solver.setColSolution(gm.ColLevel()); // TODO: what if we added a fake col because we added a fake row?
//	solver.setRowPrice(gm.RowMargin()); // TODO: what if we added a fake row?
  CoinWarmStartBasis warmstart;
  warmstart.setSize(solver.getNumCols(), solver.getNumRows());
	for (int j=0; j<gm.nCols(); ++j) {
		switch (gm.ColBasis()[j]) {
			case GamsModel::NonBasicLower : warmstart.setStructStatus(j, CoinWarmStartBasis::atLowerBound); break;
			case GamsModel::NonBasicUpper : warmstart.setStructStatus(j, CoinWarmStartBasis::atUpperBound); break;
			case GamsModel::Basic : warmstart.setStructStatus(j, CoinWarmStartBasis::basic); break;
			case GamsModel::SuperBasic : warmstart.setStructStatus(j, CoinWarmStartBasis::isFree); break;
			default: warmstart.setStructStatus(j, CoinWarmStartBasis::isFree);
				myout << "Column basis status " << gm.ColBasis()[j] << " unknown!" << CoinMessageEol;
		}
	}
	for (int j=0; j<gm.nRows(); ++j) {
		switch (gm.RowBasis()[j]) {
			case GamsModel::NonBasicLower : warmstart.setArtifStatus(j, CoinWarmStartBasis::atLowerBound); break;
			case GamsModel::NonBasicUpper : warmstart.setArtifStatus(j, CoinWarmStartBasis::atUpperBound); break;
			case GamsModel::Basic : warmstart.setArtifStatus(j, CoinWarmStartBasis::basic); break;
			case GamsModel::SuperBasic : warmstart.setArtifStatus(j, CoinWarmStartBasis::isFree); break;
			default: warmstart.setArtifStatus(j, CoinWarmStartBasis::isFree); break;
				myout << "Row basis status " << gm.RowBasis()[j] << " unknown!" << CoinMessageEol;
		}
	}
	solver.setWarmStart(&warmstart);
}
