// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

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

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

#ifdef HAVE_CCTYPE
#include <cctype>
#define HAVE_TOLOWER
#else
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#define HAVE_TOLOWER
#endif
#endif

#include "OsiSolverInterface.hpp"
#include "CoinPackedVector.hpp"

#if COIN_HAS_CLP
#include "OsiClpSolverInterface.hpp"
#endif
#if COIN_HAS_CBC
#include "OsiCbcSolverInterface.hpp"
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

#if COIN_HAS_GLPK
int printme(void* info, const char* msg) {
	GamsMessageHandler* myout=(GamsMessageHandler*)info;
	assert(myout);

	*myout << msg;
	if (*msg && msg[strlen(msg)-1]=='\n')
		*myout << CoinMessageEol; // this will make the message handler actually print the message

	return 1;
}
#endif

void setupParameters(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver);
void setupParametersMIP(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver);
void setupStartPoint(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver);

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
	
	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
	GamsModel gm(argv[1]);

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
		gm.setStatus(GamsModel::CapabilityProblems, GamsModel::ErrorNoSolution);
		gm.setSolution();
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
#if COIN_HAS_CLP
		gm.optSetString("solver", "clp");
#else
#if COIN_HAS_CBC
		gm.optSetString("solver", "cbc");
#else
#if COIN_HAS_GLPK
		gm.optSetString("solver", "glpk");
#else
#if COIN_HAS_DYLP
		gm.optSetString("solver", "dylp");
#else
#if COIN_HAS_VOL
		gm.optSetString("solver", "volume");
#else
#if COIN_HAS_GLPK
		gm.optSetString("solver", "symphony");
#else //wow, when will a user get to here?
		myout << "Error: solver parameter not set and no solver available. Aborting!" << CoinMessageEol;
		gm.setStatus(ErrorSystemFailure, ErrorNoSolution);
		gm.setSolution();
		exit(EXIT_FAILURE);
#endif
#endif
#endif
#endif
#endif
#endif
		myout << "Parameter 'solver' not set. Using default solver." << CoinMessageEol;
	}
	if (!gm.optGetString("solver", buffer)) {
		myout << "Error reading value of parameter 'solver'. Aborting!" << CoinMessageEol;
		gm.setStatus(GamsModel::ErrorSystemFailure, GamsModel::ErrorNoSolution);
		gm.setSolution();
		exit(EXIT_FAILURE);
	}
#ifdef HAVE_TOLOWER
	for (int i=strlen(buffer); i--; )
		buffer[i]=tolower(buffer[i]);
#endif
	
	bool solver_can_mips=false;
try {
#if COIN_HAS_CLP
	if (strcmp(buffer, "clp")==0) {
		solver=new OsiClpSolverInterface();
	}
#endif
#if COIN_HAS_CBC
	if (!solver && strcmp(buffer, "cbc")==0) {
		solver=new OsiCbcSolverInterface();
		solver_can_mips=true;
	}
#endif
#if COIN_HAS_GLPK
	if (!solver && strcmp(buffer, "glpk")==0) {
		solver=new OsiGlpkSolverInterface();
		glp_term_hook(printme, &slvout);
		solver_can_mips=true;
	}
#endif
#if COIN_HAS_VOL
	if (!solver && strcmp(buffer, "volume")==0) {
		solver=new OsiVolSolverInterface();
	}
#endif
#if COIN_HAS_DYLP
	if (!solver && strcmp(buffer, "dylp")==0) {
		solver=new OsiDylpSolverInterface();
	}
#endif
#if COIN_HAS_SYMPHONY
	if (!solver && strcmp(buffer, "symphony")==0) {
		solver=new OsiSymSolverInterface();
		solver_can_mips=true;
	}
#endif
	if (!solver) {
		myout << "Solver " << buffer << " not recognized or not available." << CoinMessageEol;
		gm.setStatus(GamsModel::ErrorSystemFailure, GamsModel::ErrorNoSolution);
		gm.setSolution();
		exit(EXIT_FAILURE);
	}
	
	std::string solver_name;
	if (!solver->getStrParam(OsiSolverName, solver_name))
		solver_name=buffer;
	myout << "Using solver " << solver_name << '.' << CoinMessageEol;
	
	gm.setInfinity(-solver->getInfinity(), solver->getInfinity());
	
	// now load the LP into the GamsModel
	gm.readMatrix();

	if (gm.nDCols() && !solver_can_mips) {
		myout << "Solver " << solver_name << " cannot handle integer variables. Exiting..." << CoinMessageEol;
		gm.setStatus(GamsModel::CapabilityProblems, GamsModel::ErrorNoSolution);
		gm.setSolution();
		exit(EXIT_FAILURE);
	}
	
	solver->passInMessageHandler(&slvout);

	/* Overwrite GAMS Options */
//	if (!gm.optDefined("reslim")) gm.optSetDouble("reslim", gm.getResLim());
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
	if (gm.nDCols())
		for (j=0; j<gm.nCols(); j++)
			if (discVar[j]) solver->setInteger(j);

	// why some LP solver cannot minimize a linear function over a box?
	if (!gm.nRows()) {
		myout << "Problem has no rows. Adding fake row..." << CoinMessageEol;
		if (!gm.nCols()) {
			CoinPackedVector vec(0);
			solver->addCol(vec, -solver->getInfinity(), solver->getInfinity(), 0.);
		}
		int index=0; double coeff=1;
		CoinPackedVector vec(1, &index, &coeff);
		solver->addRow(vec, -solver->getInfinity(), solver->getInfinity());
	}

	if (gm.haveNames()) { // set variable and constraint names
		//TODO: set problem (=model) and objective name
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
			if (!gm.nCols()) {
				stbuffer="fakecol";
				solver->setColName(0, stbuffer);
			}
			stbuffer="fakerow";
			solver->setRowName(0, stbuffer);
		}
	}

	// Write MPS file
	if (gm.optDefined("writemps")) {
		gm.optGetString("writemps", buffer);
  	myout << "\nWriting MPS file " << buffer << "... " << CoinMessageEol;
  	solver->writeMps(buffer);
	}
	
	setupParameters(gm, myout, *solver);
	setupStartPoint(gm, myout, *solver);

	myout << CoinMessageNewline << CoinMessageEol;
	if (gm.nDCols()==0) { // LP
		myout << "Starting LP solver..." << CoinMessageEol;
		solver->initialSolve();

	} else { // MIP
		setupParametersMIP(gm, myout, *solver);
		solver->setHintParam(OsiDoReducePrint, true, OsiHintDo);

		myout << "Starting MIP solver... " << CoinMessageEol;
		solver->branchAndBound();

		if (solver->isProvenOptimal() || solver->isIterationLimitReached()) {
			// We might loose colLevel after a call to setColBounds. So we save the levels.
			double* colLevelsav = CoinCopyOfArray(solver->getColSolution(), gm.nCols());

			// No iteration limit for fixed run
			solver->setIntParam(OsiMaxNumIteration, 9999999);

			for (j=0; j<gm.nCols(); j++)
				if (discVar[j])
					solver->setColBounds(j,colLevelsav[j],colLevelsav[j]);
			delete[] colLevelsav;

			solver->setHintParam(OsiDoReducePrint, true, OsiHintTry);
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
	gm.setStatus(GamsModel::ErrorSolverFailure, GamsModel::ErrorNoSolution);
	gm.setSolution();
	exit(EXIT_FAILURE);
}

	return EXIT_SUCCESS;
}

void setupParameters(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver) {
	// Some tolerances and limits
	if (!solver.setIntParam(OsiMaxNumIteration, gm.optGetInteger("iterlim")))
		myout << "Failed to set iteration limit to " << gm.optGetInteger("iterlim") << CoinMessageEol;

	if (!solver.setDblParam(OsiDualTolerance, gm.optGetDouble("tol_dual")))
		myout << "Failed to set dual tolerance to " << gm.optGetDouble("tol_dual") << CoinMessageEol;

	if (!solver.setDblParam(OsiPrimalTolerance, gm.optGetDouble("tol_primal")))
		myout << "Failed to set primal tolerance to " << gm.optGetDouble("tol_primal") << CoinMessageEol;

	if (!solver.setHintParam(OsiDoScale, gm.optGetBool("scaling"), OsiHintDo))
		myout << "Failed to switch scaling " << (gm.optGetBool("scaling") ? "on" : "off") << '.' << CoinMessageEol;

	char buffer[255];
	gm.optGetString("startalg", buffer);
	if (!solver.setHintParam(OsiDoDualInInitial, (strcmp(buffer, "dual")==0), OsiHintDo))
		myout << "Failed to set starting algorithm to " << buffer << CoinMessageEol;

	if (!solver.setHintParam(OsiDoPresolveInInitial, gm.optGetBool("presolve"), OsiHintDo))
		myout << "Failed to switch presolve " << (gm.optGetBool("presolve") ? "on" : "off") << '.' << CoinMessageEol;
}

void setupParametersMIP(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver) {
}

void setupStartPoint(GamsModel& gm, CoinMessageHandler& myout, OsiSolverInterface& solver) {
	if (!gm.nCols() || !gm.nRows()) return;

	try {
		solver.setColSolution(gm.ColLevel());
		solver.setRowPrice(gm.RowMargin());

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
	} catch (CoinError error) {
		myout << "Setup of startpoint failed with error:" << error.message() << CoinMessageEol;
//		myout << "\t at:" << error.fileName() << ":" << error.className() << "::" << error.methodName() << CoinMessageEol;
		myout << "Trying to continue..." << CoinMessageEol; 
	}	
}
