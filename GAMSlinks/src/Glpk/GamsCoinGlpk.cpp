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

#include "OsiGlpkSolverInterface.hpp"
#include "CoinPackedVector.hpp"
#include "CoinHelperFunctions.hpp"

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsFinalize.hpp"
#include "GamsOptions.hpp"

int printme(void* info, const char* msg) {
	GamsMessageHandler* myout=(GamsMessageHandler*)info;
	assert(myout);

	*myout << msg;
	if (*msg && msg[strlen(msg)-1]=='\n')
		*myout << CoinMessageEol; // this will make the message handler actually print the message

	return 1;
}

#ifdef OGSI_HAVE_CALLBACK
void glpk_callback(glp_tree* tree, void* info) {
	double gap=glp_ios_mip_gap(tree);
	if (gap<=*(double*)info) {
		printf("interrupt because gap = %g <= %g = optcr\n", gap, *(double*)info);
		glp_ios_terminate(tree);
	}
//	switch (glp_ios_reason(tree)) {
//		case GLP_IBINGO:
//		case GLP_IHEUR:
//			
//		
//	}
}
#endif

void setupParameters(GamsOptions& opt, CoinMessageHandler& myout, OsiGlpkSolverInterface& solver, LPX* glpk_model);
void setupParametersMIP(GamsOptions& opt, CoinMessageHandler& myout, OsiGlpkSolverInterface& solver, LPX* glpk_model);
void setupStartPoint(GamsModel& gm, CoinMessageHandler& myout, OsiGlpkSolverInterface& solver);

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

	OsiGlpkSolverInterface solver;

	// Read in the model defined by the GAMS control file passed in as the first
	// argument to this program
	GamsModel gm(argv[1]);
	gm.setInfinity(-solver.getInfinity(),solver.getInfinity());
	gm.readMatrix();
	
	// Pass in the GAMS status/log file print routines
	GamsMessageHandler myout(&gm), slvout(&gm);
	slvout.setPrefix(0);

	glp_term_hook(printme, &slvout);
	solver.passInMessageHandler(&slvout);
	myout.setCurrentDetail(1);
	gm.PrintOut(GamsModel::StatusMask, "=1"); // turn on copying into .lst file

#ifdef GAMS_BUILD
	myout << "\nGAMS/CoinGlpk LP/MIP Solver (Glpk Library" << glp_version() << ")\nwritten by A. Makhorin\n " << CoinMessageEol;
#else
	myout << "\nGAMS/Glpk LP/MIP Solver (Glpk Library" << glp_version() << ")\nwritten by A. Makhorin\n " << CoinMessageEol;
#endif

	if (gm.nSOS1() || gm.nSOS2() || gm.nSemiContinuous()) {
		myout << "GLPK cannot handle special ordered sets (SOS) or semicontinuous variables" << CoinMessageEol;
		myout << "Exiting ..." << CoinMessageEol;
		gm.setStatus(GamsModel::CapabilityProblems, GamsModel::ErrorNoSolution);
		gm.setSolution();
		exit(EXIT_FAILURE);
	}

#ifdef GAMS_BUILD
	GamsOptions opt(gm.getSystemDir(), "coinglpk");
#else
	GamsOptions opt(gm.getSystemDir(), "glpk");
#endif
	opt.readOptionsFile(gm.getOptionfile());

	/* Overwrite GAMS Options */
	if (!opt.isDefined("reslim")) opt.setDouble("reslim", gm.getResLim());
	if (!opt.isDefined("iterlim")) opt.setInteger("iterlim", gm.getIterLim());
	if (!opt.isDefined("optcr")) opt.setDouble("optcr", gm.getOptCR());
//	if (!opt.isDefined("cutoff") && gm.getCutOff()!=gm.ObjSense()*solver.getInfinity()) opt.setDouble("cutoff", gm.getCutOff());

	gm.TimerStart();

	// OsiSolver needs rowrng for the loadProblem call
	double *rowrng = CoinCopyOfArrayOrZero((double*)NULL, gm.nRows());

	// until recently, Glpk did not like zeros in the problem matrix
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
	bool* discVar=gm.ColDisc();
	if (gm.nDCols())
		for (j=0; j<gm.nCols(); j++)
			if (discVar[j]) solver.setInteger(j);

	// why this LP solver cannot minimize a linear function over a box?
	if (!gm.nRows()) {
		myout << "Problem has no rows. Adding fake row..." << CoinMessageEol;
		if (!gm.nCols()) {
			myout << "Problem has no columns. Adding fake column..." << CoinMessageEol;
			CoinPackedVector vec(0);
			solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.);
		}
		int index=0; double coeff=1;
		CoinPackedVector vec(1, &index, &coeff);
		solver.addRow(vec, -solver.getInfinity(), solver.getInfinity());
	}

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
		if (!gm.nRows()) {
			if (!gm.nCols()) {
				stbuffer="fakecol";
				solver.setColName(0, stbuffer);
			}
			stbuffer="fakerow";
			solver.setRowName(0, stbuffer);
		}
	}

	LPX* glpk_model=solver.getModelPtr();

	// Write MPS file
	if (opt.isDefined("writemps")) {
		opt.getString("writemps", buffer);
  	myout << "\nWriting MPS file " << buffer << "... " << CoinMessageEol;
		lpx_write_mps(glpk_model, buffer);
	}
	
	setupParameters(opt, myout, solver, glpk_model);
//	setupStartPoint(gm, myout, solver);

	// from glpsol: if scaling is turned on and presolve is off (or interior point is used), then do scaling 
  if (lpx_get_int_parm(glpk_model, LPX_K_SCALE) && !lpx_get_int_parm(glpk_model, LPX_K_PRESOL))
  	lpx_scale_prob(glpk_model);
  // from glpsol: if no presolve (and simplex is used), use special basis method
  if (!lpx_get_int_parm(glpk_model, LPX_K_PRESOL)) {
		opt.getString("initbasis", buffer);
		if (strcmp(buffer, "standard")==0)
			lpx_std_basis(glpk_model);
		else if (strcmp(buffer, "advanced")==0)
			lpx_adv_basis(glpk_model);
		else if (strcmp(buffer, "bixby")==0)
			lpx_cpx_basis(glpk_model);
  }

  bool mipoptimal=false;
	myout.setCurrentDetail(2);
	gm.PrintOut(GamsModel::StatusMask, "=2"); // turn off copying into .lst file
	myout << CoinMessageNewline << CoinMessageEol;
	if (gm.nDCols()==0) { // LP

		myout << "Starting Glpk LP solver..." << CoinMessageEol;
		solver.initialSolve();

	} else { // MIP
		setupParametersMIP(opt, myout, solver, glpk_model);
#ifdef OGSI_HAVE_CALLBACK
		double optcr=gm.getOptCR();
		solver.registerCallback(glpk_callback, (void*)&optcr);
#endif
		myout << "Starting GLPK Branch and Bound... " << CoinMessageEol;
		solver.branchAndBound();
		mipoptimal=solver.isProvenOptimal();

		int mipstat = lpx_mip_status(glpk_model);
		if (!solver.isIterationLimitReached()
#ifdef OGSI_HAVE_TIMELIMIT
				&& !solver.isTimeLimitReached()
#endif
				&& (mipstat == LPX_I_FEAS || mipstat == LPX_I_OPT)) {
			const double *colLevel=solver.getColSolution();
			// We are loosing colLevel after a call to lpx_set_* when using the Visual compiler. So we save the levels.
			double *colLevelsav = new double[gm.nCols()];
			for (j=0; j<gm.nCols(); j++) colLevelsav[j] = colLevel[j];

			// No iteration limit for fixed run and special time limit
			lpx_set_real_parm(glpk_model, LPX_K_TMLIM, opt.getDouble("reslim_fixedrun"));
			lpx_set_int_parm(glpk_model, LPX_K_ITLIM, -1);

			for (j=0; j<gm.nCols(); j++)
				if (discVar[j])
					solver.setColBounds(j,colLevelsav[j],colLevelsav[j]);
			delete[] colLevelsav;

			solver.setHintParam(OsiDoReducePrint, true, OsiHintTry); // loglevel 1
			myout << "\nSolving fixed problem... " << CoinMessageEol;
			solver.resolve();
			if (!solver.isProvenOptimal())
				myout << "Problems solving fixed problem. No solution returned." << CoinMessageEol;
		}
	}

	// Determine status and write solution
	myout.setCurrentDetail(1);
	gm.PrintOut(GamsModel::StatusMask, "=1"); // turn on copying into .lst file
	
	gm.setIterUsed(solver.getIterationCount());
	gm.setResUsed(gm.SecondsSinceStart());
	gm.setObjVal(solver.getObjValue());

	gm.setStatus(GamsModel::ErrorSystemFailure,GamsModel::ErrorNoSolution);	
	myout << "\n" << CoinMessageEol;

#ifdef OGSI_HAVE_TIMELIMIT
	bool timelimitreached=solver.isTimeLimitReached();
#else
	bool timelimitreached=false;
#endif
#ifdef OGSI_HAVE_FEASIBILITY
	bool feasible=solver.isFeasible();
#else
	bool feasible=false;
#endif

	if (timelimitreached) {
		if (feasible) {
			myout << "Time limit exceeded. Have feasible solution.";
			if (gm.isLP())
				gm.setStatus(GamsModel::ResourceInterrupt,GamsModel::IntermediateNonoptimal);
			else
				gm.setStatus(GamsModel::ResourceInterrupt,GamsModel::IntegerSolution);
		} else {
			myout << "Time limit exceeded.";
			gm.setStatus(GamsModel::ResourceInterrupt,GamsModel::NoSolutionReturned);	
		}
	} else if (solver.isProvenOptimal()) { // LP or fixed LP was solved to optimality
		if (!gm.isLP() && !mipoptimal) { 
			myout << "Integer Solution.";
			gm.setStatus(GamsModel::NormalCompletion,GamsModel::IntegerSolution);	
		} else {
			myout << "Solved optimal.";
			gm.setStatus(GamsModel::NormalCompletion,GamsModel::Optimal);	
		}
	}	else if (solver.isProvenPrimalInfeasible()) {
		myout << "Model infeasible.";
		gm.setStatus(GamsModel::NormalCompletion,GamsModel::InfeasibleNoSolution);	
	} else if (solver.isProvenDualInfeasible()) { // GAMS doesn't have dual infeasible, so we hope for the best and call it unbounded
		myout << "Model unbounded.";
		gm.setStatus(GamsModel::NormalCompletion,GamsModel::UnboundedNoSolution);	
	} else if (solver.isIterationLimitReached()) {
		if (feasible) {
			myout << "Iteration limit exceeded. Have feasible solution.";
			if (gm.isLP())
				gm.setStatus(GamsModel::IterationInterrupt,GamsModel::IntermediateNonoptimal);
			else
				gm.setStatus(GamsModel::IterationInterrupt,GamsModel::IntegerSolution);
		} else {
			myout << "Iteration limit exceeded.";
			gm.setStatus(GamsModel::IterationInterrupt,GamsModel::NoSolutionReturned);	
		}
	} else if (solver.isPrimalObjectiveLimitReached()) {
		myout << "Primal objective limit reached.";
	} else if (solver.isDualObjectiveLimitReached()) {
		myout << "Dual objective limit reached.";
	} else if (solver.isAbandoned()) { 
		myout << "Model abandoned.";
	} else {
		myout << "Unknown solve outcome.";
	}

	myout << CoinMessageEol;

	// We write a solution if model was declared optimal or feasible.
	if (GamsModel::Optimal==gm.getModelStatus() || 
			GamsModel::IntegerSolution==gm.getModelStatus()) {
		GamsWriteSolutionOsi(&gm, &myout, &solver);
	}	else {
		gm.setSolution(); // Need this to trigger the write of GAMS solution file
	}

	return 0;
}

void setupParameters(GamsOptions& opt, CoinMessageHandler& myout, OsiGlpkSolverInterface& solver, LPX* glpk_model) {
	// Some tolerances and limits
	solver.setIntParam(OsiMaxNumIteration, opt.getInteger("iterlim"));
	double timelimit=opt.getDouble("reslim");
	if (timelimit>1e+6) { // GLPK cannot handle very large timelimits, so we run it without limit then 
		myout << "Time limit" << timelimit << "too large. GLPK will run without timelimit." << CoinMessageEol;
		timelimit=-1;
	}
	lpx_set_real_parm(glpk_model, LPX_K_TMLIM, timelimit);

	if (!solver.setDblParam(OsiDualTolerance, opt.getDouble("tol_dual")))
		myout << "Failed to set dual tolerance to " << opt.getDouble("tol_dual") << CoinMessageEol;

	if (!solver.setDblParam(OsiPrimalTolerance, opt.getDouble("tol_primal")))
		myout << "Failed to set primal tolerance to " << opt.getDouble("tol_primal") << CoinMessageEol;

	// more parameters
	char buffer[255];
	opt.getString("scaling", buffer);
	if (strcmp(buffer, "off")==0)
		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 0);
	else if (strcmp(buffer, "equilibrium")==0)
		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 1);
	else if (strcmp(buffer, "mean")==0)
		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 2);
	else if (strcmp(buffer, "meanequilibrium")==0)
		lpx_set_int_parm(glpk_model, LPX_K_SCALE, 3); // default (set in OsiGlpk)

	opt.getString("startalg", buffer);
	solver.setHintParam(OsiDoDualInInitial, (strcmp(buffer, "dual")==0), OsiForceDo);

	solver.setHintParam(OsiDoPresolveInInitial, opt.getBool("presolve"), OsiForceDo);

	opt.getString("pricing", buffer);
	if (strcmp(buffer, "textbook")==0)
		lpx_set_int_parm(glpk_model, LPX_K_PRICE, 0);
	else if	(strcmp(buffer, "steepestedge")==0)
		lpx_set_int_parm(glpk_model, LPX_K_PRICE, 1); // default

	solver.setHintParam(OsiDoReducePrint, false, OsiForceDo); // GLPK loglevel 3
	
	opt.getString("factorization", buffer);
	if (strcmp(buffer, "forresttomlin")==0)
		lpx_set_int_parm(glpk_model, LPX_K_BFTYPE, 1);
	else if (strcmp(buffer, "bartelsgolub")==0)
		lpx_set_int_parm(glpk_model, LPX_K_BFTYPE, 2);
	else if (strcmp(buffer, "givens")==0)
		lpx_set_int_parm(glpk_model, LPX_K_BFTYPE, 3);
}

void setupParametersMIP(GamsOptions& opt, CoinMessageHandler& myout, OsiGlpkSolverInterface& solver, LPX* glpk_model) {
	char buffer[255];
	opt.getString("backtracking", buffer);
	if (strcmp(buffer, "depthfirst")==0)
		lpx_set_int_parm(glpk_model, LPX_K_BTRACK, 0);
	else if	(strcmp(buffer, "breadthfirst")==0)
		lpx_set_int_parm(glpk_model, LPX_K_BTRACK, 1);
	else if	(strcmp(buffer, "bestprojection")==0)
		lpx_set_int_parm(glpk_model, LPX_K_BTRACK, 2);

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

	double optcr=opt.getDouble("optcr");
	lpx_set_real_parm(glpk_model, LPX_K_MIPGAP, optcr);

	double tol_integer=opt.getDouble("tol_integer");
	if (tol_integer>0.001) {
		myout << "Cannot use tol_integer of larger then 0.001. Setting integer tolerance to 0.001." << CoinMessageEol;
		tol_integer=0.001;
	}
	lpx_set_real_parm(glpk_model, LPX_K_TOLINT, tol_integer);
}

void setupStartPoint(GamsModel& gm, CoinMessageHandler& myout, OsiGlpkSolverInterface& solver) {
	if (!gm.nCols() || !gm.nRows()) return;
//   solver.setColSolution(gm.ColLevel()); // no useful implementation in OsiGLPK yet
//   solver.setRowPrice(gm.RowMargin()); // no useful implementation in OsiGLPK yet
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
