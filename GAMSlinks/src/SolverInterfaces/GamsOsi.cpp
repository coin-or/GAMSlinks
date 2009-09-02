// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsOsi.cpp 701 2009-07-26 15:11:55Z stefan $
//
// Author: Stefan Vigerske

#include "GamsOsi.hpp"
#include "GamsOsiCplex.h"
#include "GamsOsiXpress.h"
#include "GamsOsiMosek.h"

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
#define GMS_SV_NA     2.0E300   /* not available/applicable */
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gevlice.h"
#endif
#include "GamsMessageHandler.hpp"
#include "GamsOsiHelper.hpp"

#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinPackedVector.hpp"
#ifdef COIN_HAS_CBC
#include "OsiCbcSolverInterface.hpp"
#endif
#ifdef COIN_HAS_CPX
#include "OsiCpxSolverInterface.hpp"
#endif
#ifdef COIN_HAS_XPR
#include "OsiXprSolverInterface.hpp"
#endif
#ifdef COIN_HAS_MSK
#include "OsiMskSolverInterface.hpp"
#endif

GamsOsi::GamsOsi(GamsOsi::OSISOLVER solverid_)
: gmo(NULL), gev(NULL), msghandler(NULL), osi(NULL), solverid(solverid_)
{
	switch (solverid) {

#ifdef COIN_HAS_CBC
		case CBC:
#ifdef GAMS_BUILD
			sprintf(osi_message, "GAMS/CoinOsiCbc (Osi library 0.100, CBC library 2.3)\nwritten by J. Forrest\n");
#else
			sprintf(osi_message, "GAMS/OsiCbc (Osi library 0.100, CBC library 2.3)\nwritten by J. Forrest\n");
#endif
			break;
#endif
			
#ifdef COIN_HAS_CPX
		case CPLEX:
#ifdef GAMS_BUILD
			sprintf(osi_message, "GAMS/CoinOsiCplex (Osi library 0.100, CPLEX library %.2f)\nwritten by T. Achterberg\n", CPX_VERSION/100.);
#else
			sprintf(osi_message, "GAMS/OsiCplex (Osi library 0.100, CPLEX library %.2f)\nwritten by T. Achterberg\n", CPX_VERSION/100.);
#endif
			break;
#endif
			
#ifdef COIN_HAS_MSK
		case MOSEK:
#ifdef GAMS_BUILD
			sprintf(osi_message, "GAMS/CoinOsiMosek (Osi library 0.100, MOSEK library %d.%d)\nwritten by Bo Jensen\n", MSK_VERSION_MAJOR, MSK_VERSION_MINOR);
#else
			sprintf(osi_message, "GAMS/OsiMosek (Osi library 0.100, MOSEK library %d.%d)\nwritten Bo Jensen\n", MSK_VERSION_MAJOR, MSK_VERSION_MINOR);
#endif
			break;
#endif
			
#ifdef COIN_HAS_XPR
		case XPRESS:
#ifdef GAMS_BUILD
			sprintf(osi_message, "GAMS/CoinOsiXpress (Osi library 0.100, XPRESS library %d)\n", XPVERSION);
#else
			sprintf(osi_message, "GAMS/OsiXpress (Osi library 0.100, XPRESS library %d)\n", XPVERSION);
#endif
			break;
#endif
			
     default:
    	 fprintf(stderr, "Unsupported solver id: %d\n", solverid);
    	 exit(EXIT_FAILURE);
	}
}

GamsOsi::~GamsOsi() {
	delete osi;
	delete msghandler;
}

int GamsOsi::readyAPI(struct gmoRec* gmo_, struct optRec* opt) {
	gmo = gmo_;
	assert(gmo);
	assert(!osi);
	assert(!opt);

	if (getGmoReady())
		return 1;

	if (getGevReady())
		return 1;

	gev = (gevRec*)gmoEnvironment(gmo);

#ifdef GAMS_BUILD
#define GEVPTR gev 
#include "cgevmagic2.h"
	if  (gevLicenseCheck(gev, gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo))) {
	  gmoSolveStatSet(gmo, SolveStat_License); gmoModelStatSet(gmo, ModelStat_LicenseError);
	  return 1;
	}
#endif
	
	try {
		switch (solverid) {
			case CBC:
#ifdef COIN_HAS_CBC
				osi = new OsiCbcSolverInterface;
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/Cbc interface.\n");
				return 1;
#endif
				break;
				
			case CPLEX: {
#ifdef COIN_HAS_CPX
#ifdef GAMS_BUILD
				int cp_l=0, cp_m=0, cp_q=0, cp_p=0;
				licenseInit_t initType;

				/* Cplex license setup */
				if (gevcplexlice(gev,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),
						gmoNDisc(gmo), 1, &initType, &cp_l, &cp_m, &cp_q, &cp_p)) {
					gevLogStat(gev, "*** Could not register GAMS/CPLEX license. Contact support@gams.com\n");
					gmoSolveStatSet(gmo, SolveStat_License); gmoModelStatSet(gmo, ModelStat_LicenseError);
					return 1;
				}
#endif
				osi = new OsiCpxSolverInterface;
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/CPLEX interface.\n");
				return 1;
#endif
			} break;
			
			case MOSEK:
#ifdef COIN_HAS_MSK
				osi = new OsiMskSolverInterface;
#ifdef GAMS_BUILD
				gevLogStat(gev, "Still alive.\n");
				
#endif
				
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/MOSEK interface.\n");
				return 1;
#endif
				break;
				
			case XPRESS: {
#ifdef COIN_HAS_XPR
#ifdef GAMS_BUILD
				XPlicenseInit_t initType;
				char msg[256];
				
				/* Xpress license setup */
				if (gevxpresslice(gev,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),
						gmoNDisc(gmo), 1, &initType, msg, sizeof(msg))) {
					gevLogStat(gev, "*** Could not register GAMS/XPRESS license. Contact support@gams.com\n");
					gevLogStat(gev, msg);
					gmoSolveStatSet(gmo, SolveStat_License); gmoModelStatSet(gmo, ModelStat_LicenseError);
					return 1;
				}
#endif				
				OsiXprSolverInterface* osixpr = new OsiXprSolverInterface(gmoM(gmo), gmoNZ(gmo));
				if (!osixpr->getNumInstances()) {
					gevLogStat(gev, "Failed to setup XPRESS instance. Maybe you do not have a license?\n");
					return 1;
				}
				osi = osixpr;
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/XPRESS interface.\n");
				return 1;
#endif
			} break;
			
			default:
				gevLogStat(gev, "Unsupported solver id\n");
				return 1;
		}
	} catch (CoinError error) {
		gevLogStatPChar(gev, "Exception caught when creating Osi interface: ");
		gevLogStat(gev, error.message().c_str());
		return 1;
	} catch (...) {
		gevLogStat(gev, "Unknown exception caught when creating Osi interface\n");
		return 1;
	}

	gmoPinfSet(gmo,  osi->getInfinity());
	gmoMinfSet(gmo, -osi->getInfinity());
	gmoObjReformSet(gmo, 1);
	gmoObjStyleSet(gmo, ObjType_Fun);
	gmoIndexBaseSet(gmo, 0);

	msghandler = new GamsMessageHandler(gev);
	osi->passInMessageHandler(msghandler);
	osi->setHintParam(OsiDoReducePrint, true, OsiHintTry);

	if (!setupProblem(*osi)) {
		gevLogStat(gev, "Error setting up problem...");
		return -1;
	}

	if (gmoN(gmo) && !setupStartingPoint()) {
		gevLogStat(gev, "Error setting up starting point.");
		return -1;
	}
	
	if (!setupParameters()) {
		gevLogStat(gev, "Error setting up OSI parameters.");
		return -1;
	}

	return 0;
}

//int GamsOsi::haveModifyProblem() {
//	return 0;
//}
//
//int GamsOsi::modifyProblem() {
//	assert(false);
//	return -1;
//}

int GamsOsi::callSolver() {
	gevLogStat(gev, "\nCalling OSI main solution routine...");

	double start_cputime  = CoinCpuTime();
	double start_walltime = CoinWallclockTime();

	if (isLP())
		osi->initialSolve();
	else
		osi->branchAndBound();

	double end_cputime  = CoinCpuTime();
	double end_walltime = CoinWallclockTime();

	double* varlow = NULL;
	double* varup  = NULL;
	double* varlev = NULL;
	if (!isLP() && osi->isProvenOptimal()) { // solve again with fixed noncontinuous variables and original bounds on continuous variables
		gevLog(gev, "Resolve with fixed noncontinuous variables.");
		varlow = new double[gmoN(gmo)];
		varup  = new double[gmoN(gmo)];
		varlev = new double[gmoN(gmo)];
		gmoGetVarLower(gmo, varlow);
		gmoGetVarUpper(gmo, varup);
		memcpy(varlev, osi->getColSolution(), gmoN(gmo) * sizeof(double));
		for (int i = 0; i < gmoN(gmo); ++i)
			if ((enum gmoVarType)gmoGetVarTypeOne(gmo, i) != var_X)
				varlow[i] = varup[i] = varlev[i];
		osi->setColLower(varlow);
		osi->setColUpper(varup);

		osi->messageHandler()->setLogLevel(1);
		osi->resolve();
		if (!osi->isProvenOptimal()) {
			gevLog(gev, "Resolve failed, values for dual variables will be unavailable.");
			osi->setColSolution(varlev);
		}
	}

	writeSolution(end_cputime - start_cputime, end_walltime - start_walltime);

	if (!isLP() && varlev) { // reset original bounds
		gmoGetVarLower(gmo, varlow);
		gmoGetVarUpper(gmo, varup);
		osi->setColLower(varlow);
		osi->setColUpper(varup);
		osi->setColSolution(varlev);
	}
	delete[] varlow;
	delete[] varup;
	delete[] varlev;

	return 0;
}

bool GamsOsi::setupProblem(OsiSolverInterface& solver) {
	if (gmoGetVarTypeCnt(gmo, var_SC) || gmoGetVarTypeCnt(gmo, var_SI) || gmoGetVarTypeCnt(gmo, var_S1) || gmoGetVarTypeCnt(gmo, var_S2)) {
		gevLogStat(gev, "SOS, semicontinuous, and semiinteger variables not supported by OSI. Aborting...\n");
		return false;
	}
	
	if (!gamsOsiLoadProblem(gmo, solver))
		return false;

	if (!gmoN(gmo)) {
		gevLog(gev, "Problem has no columns. Adding fake column...");
		CoinPackedVector vec(0);
		solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.);
	}

	if (gmoDict(gmo)!=NULL) {
		solver.setIntParam(OsiNameDiscipline, 2);
		char buffer[255];
		std::string stbuffer;
		for (int j = 0; j < gmoN(gmo); ++j) {
			gmoGetVarNameOne(gmo, j, buffer);
			stbuffer = buffer;
			solver.setColName(j, stbuffer);
		}
		for (int j = 0; j < gmoM(gmo); ++j) {
			gmoGetEquNameOne(gmo, j, buffer);
			stbuffer = buffer;
			solver.setRowName(j, stbuffer);
		}
	}
        
	return true;
}


bool GamsOsi::setupStartingPoint() {
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
					gevLogStat(gev, "Error: invalid basis indicator for column.");
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
					gevLogStat(gev, "Error: invalid basis indicator for row.");
					delete[] cstat;
					delete[] rstat;
					delete[] rowprice;
					delete[] varlevel;
					return false;
			}
		}

		osi->setColSolution(varlevel);
		osi->setRowPrice(rowprice);
		if (osi->setBasisStatus(cstat, rstat)) {
			gevLogStat(gev, "Failed to set initial basis. Exiting ...");
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

	delete[] varlevel;

	return true;
}

bool GamsOsi::setupParameters() {
	//note: does not seem to work via Osi: OsiDoPresolveInInitial, OsiDoDualInInitial

	// Some tolerances and limits
#if STEFAN
	model->setDblParam(CbcModel::CbcMaximumSeconds,  options.getDouble("reslim"));
#endif
	
	// iteration limit only for LPs
	if (isLP())
		osi->setIntParam(OsiMaxNumIteration, gevGetIntOpt(gev, gevIterLim));

#if STEFAN
	// MIP parameters
	model->setIntParam(CbcModel::CbcMaxNumNode,           options.getInteger("nodelim"));
	model->setDblParam(CbcModel::CbcAllowableGap,         options.getDouble ("optca"));
	model->setDblParam(CbcModel::CbcAllowableFractionGap, options.getDouble ("optcr"));
#endif
	return true;
}

bool GamsOsi::writeSolution(double cputime, double walltime) {
	bool write_solution = false;
	char buffer[255];

	gevLogStat(gev, "");
	if (osi->isProvenDualInfeasible()) {
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
		gevLogStat(gev, "Model unbounded.");
		
	} else if (osi->isProvenPrimalInfeasible()) {
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
		gevLogStat(gev, "Model infeasible.");

	} else if (osi->isAbandoned()) {
		gmoSolveStatSet(gmo, SolveStat_SolverErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gevLogStat(gev, "Model abandoned.");

	} else if (osi->isProvenOptimal()) {
		write_solution = true;
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
		gevLogStat(gev, "Solved to optimality.");

#if STEFAN
	} else if (model->isSecondsLimitReached()) {
		gmoSolveStatSet(gmo, SolveStat_Resource);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		gevLogStat(gev, "Time limit reached.");

#endif
	} else if (osi->isIterationLimitReached()) {
		gmoSolveStatSet(gmo, SolveStat_Iteration);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		gevLogStat(gev, "Iteration limit reached.");

	} else if (osi->isPrimalObjectiveLimitReached()) {
		gmoSolveStatSet(gmo, SolveStat_Solver);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		gevLogStat(gev, "Primal objective limit reached.");

	} else if (osi->isDualObjectiveLimitReached()) {
		gmoSolveStatSet(gmo, SolveStat_Solver);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		gevLogStat(gev, "Dual objective limit reached.");

	} else {
		gmoSolveStatSet(gmo, SolveStat_Solver);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
		gevLogStat(gev, "Model status unknown, no feasible solution found.");
		gevLogStat(gev, buffer);
	}

	gmoSetHeadnTail(gmo, Hiterused, osi->getIterationCount());
	gmoSetHeadnTail(gmo, HresUsed, cputime);
#if STEFAN
	gmoSetHeadnTail(gmo, Tmipbest, model->getBestPossibleObjValue());
	gmoSetHeadnTail(gmo, Tmipnod, model->getNodeCount());
#endif
	if (write_solution && !gamsOsiStoreSolution(gmo, *osi, false))
		return false;

	if (!isLP()) {
		if (osi->getColSolution()) {
			snprintf(buffer, 255, "MIP solution: %21.10g   (%g seconds)", osi->getObjValue(), cputime);
			gevLogStat(gev, buffer);
		}
#if STEFAN
		snprintf(buffer, 255, "Best possible: %20.10g", model->getBestPossibleObjValue());
		gevLogStat(gev, buffer);
		if (model->bestSolution()) {
			snprintf(buffer, 255, "Absolute gap: %21.5g   (absolute tolerance optca: %g)", CoinAbs(model->getObjValue() - model->getBestPossibleObjValue()), options.getDouble("optca"));
			gevLogStat(gev, buffer);
			snprintf(buffer, 255, "Relative gap: %21.5g   (relative tolerance optcr: %g)", CoinAbs(model->getObjValue() - model->getBestPossibleObjValue()) / CoinMax(CoinAbs(model->getBestPossibleObjValue()), 1.), options.getDouble("optcr"));
			gevLogStat(gev, buffer);
		}
#endif
	}

	return true;
}

bool GamsOsi::isLP() {
	if (gmoModelType(gmo) == Proc_lp)
		return true;
	if (gmoModelType(gmo) == Proc_rmip)
		return true;
	if (gmoNDisc(gmo))
		return false;
	return true;
}

DllExport GamsOsi* STDCALL createNewGamsOsiCplex() {
	return new GamsOsi(GamsOsi::CPLEX);
}

DllExport GamsOsi* STDCALL createNewGamsOsiMosek() {
	return new GamsOsi(GamsOsi::MOSEK);
}

DllExport GamsOsi* STDCALL createNewGamsOsiXpress() {
	return new GamsOsi(GamsOsi::XPRESS);
}

#define osi_C_interface( xxx, yyy ) \
	DllExport int STDCALL xxx ## CallSolver(xxx ## Rec_t *Cptr) { \
		assert(Cptr != NULL); \
		return ((GamsOsi*)Cptr)->callSolver(); \
	} \
	\
	DllExport int STDCALL xxx ## ModifyProblem(xxx ## Rec_t *Cptr) { \
		assert(Cptr != NULL); \
		return ((GamsOsi*)Cptr)->modifyProblem(); \
	} \
	\
	DllExport int STDCALL xxx ## HaveModifyProblem(xxx ## Rec_t *Cptr) { \
		assert(Cptr != NULL); \
		return ((GamsOsi*)Cptr)->haveModifyProblem(); \
	} \
	\
	DllExport int STDCALL xxx ## ReadyAPI(xxx ## Rec_t *Cptr, gmoHandle_t Gptr, optHandle_t Optr) { \
		gevHandle_t Eptr; \
		assert(Cptr != NULL); \
		assert(Gptr != NULL); \
		char msg[256]; \
		if (!gmoGetReady(msg, sizeof(msg))) \
			return 1; \
		if (!gevGetReady(msg, sizeof(msg))) \
			return 1; \
		Eptr = (gevHandle_t) gmoEnvironment(Gptr); \
		gevLogStatPChar(Eptr, ((GamsOsi*)Cptr)->getWelcomeMessage()); \
		\
		return ((GamsOsi*)Cptr)->readyAPI(Gptr, Optr); \
	} \
	\
	DllExport void STDCALL xxx ## Free(xxx ## Rec_t **Cptr) { \
		assert(Cptr != NULL); \
		delete (GamsOsi*)*Cptr; \
		*Cptr = NULL; \
	} \
	\
	DllExport void STDCALL xxx ## Create(xxx ## Rec_t **Cptr, char *msgBuf, int msgBufLen) { \
		assert(Cptr != NULL); \
		*Cptr = (xxx ## Rec_t*) new GamsOsi(yyy); \
		if (msgBufLen && msgBuf) \
			msgBuf[0] = 0; \
	}
	
osi_C_interface(ocp, GamsOsi::CPLEX)
osi_C_interface(oxp, GamsOsi::XPRESS)
osi_C_interface(omk, GamsOsi::MOSEK)
