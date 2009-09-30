// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOsi.hpp"
#include "GamsOsiCplex.h"
#include "GamsOsiGurobi.h"
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
#ifdef COIN_HAS_GRB
#include "OsiGrbSolverInterface.hpp"
extern "C" {
#include "gurobi_c.h" // to get version numbers
}
#endif
#ifdef COIN_HAS_XPR
#include "OsiXprSolverInterface.hpp"
extern "C" void STDCALL XPRScommand(XPRSprob, char*);
#endif
#ifdef COIN_HAS_MSK
#include "OsiMskSolverInterface.hpp"
#include "mosek.h"
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
			sprintf(osi_message, "GAMS/OsiCplex (Osi library 0.100, CPLEX library %.2f)\nOsi link written by T. Achterberg\n", CPX_VERSION/100.);
#endif
			break;
#endif

#ifdef COIN_HAS_GRB
		case GUROBI:
#ifdef GAMS_BUILD
			sprintf(osi_message, "GAMS/CoinOsiGurobi (Osi library 0.100, GUROBI library %d.%d.%d)\nOsi link written by S. Vigerske\n", GRB_VERSION_MAJOR, GRB_VERSION_MINOR, GRB_VERSION_TECHNICAL);
#else
			sprintf(osi_message, "GAMS/OsiGurobi (Osi library 0.100, GUROBI library %d.%d.%d)\nwritten by S. Vigerske\n", GRB_VERSION_MAJOR, GRB_VERSION_MINOR, GRB_VERSION_TECHNICAL);
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
	if (gevLicenseCheck(gev, gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo))) {
		gevLogStat(gev, "The license check failed.\n");
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
				CPlicenseInit_t initType;

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

			case GUROBI: {
#ifdef COIN_HAS_GRB
				GRBenv* grbenv = NULL;
#ifdef GAMS_BUILD
				GUlicenseInit_t initType;

				/* Gurobi license setup */
				if (gevgurobilice(gev,(void**)&grbenv,NULL,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),
						gmoNDisc(gmo), 1, &initType)) {
					gevLogStat(gev, "*** Could not register GAMS/GUROBI license. Contact support@gams.com\n");
					gmoSolveStatSet(gmo, SolveStat_License); gmoModelStatSet(gmo, ModelStat_LicenseError);
					return 1;
				}
#endif
//				OsiGrbSolverInterface::setEnvironment(grbenv);
//				osi = new OsiGrbSolverInterface();
				osi = new OsiGrbSolverInterface(grbenv); // this lets OsiGrb take over ownership of grbenv, so it will be freed when osi is deleted
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/Gurobi interface.\n");
				return 1;
#endif
			} break;

			case MOSEK: {
#ifdef COIN_HAS_MSK
#ifdef GAMS_BUILD
				MSKenv_t mskenv;
				MKlicenseInit_t initType;
				
		    if (MSK_makeenv(&mskenv,NULL, NULL,NULL,NULL)) {
					gevLogStat(gev, "Failed to initialize Mosek environment.");
					return 1;
		    }
//		    err = MSK_linkfunctoenvstream(env_, MSK_STREAM_LOG, NULL, printlog);
//		    checkMSKerror( err, "MSK_linkfunctoenvstream", "incrementInstanceCounter" );
				if (gevmoseklice(gev,mskenv,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo), 1, &initType)) {
					gevLogStat(gev, "*** Could not register GAMS/MOSEK license. Contact support@gams.com\n");
					gmoSolveStatSet(gmo, SolveStat_License); gmoModelStatSet(gmo, ModelStat_LicenseError);
					return 1;
				}
		    if (MSK_initenv(mskenv)) {
					gevLogStat(gev, "Failed to initialize Mosek environment.");
					return 1;
		    }
#endif
				osi = new OsiMskSolverInterface(mskenv);
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/MOSEK interface.\n");
				return 1;
#endif
			} break;
				
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
//		return -1;
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
//	gevLogStat(gev, "\nCalling OSI main solution routine...");

	double start_cputime  = CoinCpuTime();
	double start_walltime = CoinWallclockTime();
	
	try {
		if (isLP() || solverid == XPRESS)
			osi->initialSolve();
		if (!isLP())
			osi->branchAndBound();
	} catch (CoinError error) {
		gevLogStatPChar(gev, "Exception caught when solving problem: ");
		gevLogStat(gev, error.message().c_str());
	}
	
	double end_cputime  = CoinCpuTime();
	double end_walltime = CoinWallclockTime();

#if 0
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
#endif
	
	gevLogStat(gev, "");
	writeSolution(end_cputime - start_cputime, end_walltime - start_walltime);

#if 0
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
#endif
	
	return 0;
}

bool GamsOsi::setupProblem(OsiSolverInterface& solver) {
	if (gmoGetVarTypeCnt(gmo, var_SC) || gmoGetVarTypeCnt(gmo, var_SI) || gmoGetVarTypeCnt(gmo, var_S1) || gmoGetVarTypeCnt(gmo, var_S2)) {
		gevLogStat(gev, "SOS, semicontinuous, and semiinteger variables not supported by OSI. Aborting...\n");
		gmoSolveStatSet(gmo, SolveStat_Capability);
		gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
		return false;
	}
	
	if (gmoGetEquTypeCnt(gmo, equ_N))
		gmoSetNRowPerm(gmo);
	
	if (!gamsOsiLoadProblem(gmo, solver))
		return false;

//	if (!gmoN(gmo)) {
//		gevLog(gev, "Problem has no columns. Adding fake column...");
//		CoinPackedVector vec(0);
//		solver.addCol(vec, -solver.getInfinity(), solver.getInfinity(), 0.);
//	}

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
	if (!gmoHaveBasis(gmo))
		return true;
	
	double* varlevel = new double[gmoN(gmo)];
	double* rowprice = new double[gmoM(gmo)];
	CoinWarmStartBasis basis;
	basis.setSize(gmoN(gmo), gmoM(gmo));

	gmoGetVarL(gmo, varlevel);
	gmoGetEquM(gmo, rowprice);

	int nbas = 0;
	for (int j = 0; j < gmoN(gmo); ++j) {
		switch (gmoGetVarBasOne(gmo, j)) {
			case 0: // this seem to mean that variable should be basic
				if (nbas < gmoM(gmo)) {
					basis.setStructStatus(j, CoinWarmStartBasis::basic);
					++nbas;
				} else if (gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo))
					basis.setStructStatus(j, CoinWarmStartBasis::isFree);
				else if (fabs(gmoGetVarLowerOne(gmo, j) - varlevel[j]) < fabs(gmoGetVarUpperOne(gmo, j) - varlevel[j]))
					basis.setStructStatus(j, CoinWarmStartBasis::atLowerBound);
				else
					basis.setStructStatus(j, CoinWarmStartBasis::atUpperBound);
				break;
			case 1:
				if (fabs(gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j)) < fabs(gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j)))
					basis.setStructStatus(j, CoinWarmStartBasis::atLowerBound);
				else
					basis.setStructStatus(j, CoinWarmStartBasis::atUpperBound);
				break;
			default:
				gevLogStat(gev, "Error: invalid basis indicator for column.");
				delete[] rowprice;
				delete[] varlevel;
				return false;
		}
	}

	for (int j = 0; j< gmoM(gmo); ++j) {
		switch (gmoGetEquBasOne(gmo, j)) {
			case 0:
				if (nbas < gmoM(gmo)) {
					basis.setArtifStatus(j, CoinWarmStartBasis::basic);
					++nbas;
				} else
					basis.setArtifStatus(j, CoinWarmStartBasis::atUpperBound);
				break;
			case 1:
				basis.setArtifStatus(j, CoinWarmStartBasis::atUpperBound);
				break;
			default:
				gevLogStat(gev, "Error: invalid basis indicator for row.");
				delete[] rowprice;
				delete[] varlevel;
				return false;
		}
	}

	try {
		if (solverid != GUROBI) {
			osi->setColSolution(varlevel);
			osi->setRowPrice(rowprice);
		}
		if (!osi->setWarmStart(&basis)) {
			gevLogStat(gev, "Failed to set initial basis. Exiting ...");
			delete[] varlevel;
			delete[] rowprice;
			return false;
		} else if (solverid == GUROBI ) {
			gevLog(gev, "Registered advanced basis. This turns off presolve!");
			gevLog(gev, "In case of poor performance consider turning off advanced basis registration via GAMS option BRatio=1.");
		} else {
			gevLog(gev, "Registered advanced basis.");
		}
	} catch (CoinError error) {
		gevLogStatPChar(gev, "Exception caught when setting initial basis: ");
		gevLogStat(gev, error.message().c_str());
		delete[] varlevel;
		delete[] rowprice;
		return false;
	}

	delete[] varlevel;
	delete[] rowprice;

	return true;
}

bool GamsOsi::setupParameters() {
	// iteration limit only for LPs
	if (isLP())
		osi->setIntParam(OsiMaxNumIteration, gevGetIntOpt(gev, gevIterLim));

	double reslim = gevGetDblOpt(gev, gevResLim);
	int nodelim = gevGetIntOpt(gev, gevNodeLim);
	double optcr = gevGetDblOpt(gev, gevOptCR);
	double optca = gevGetDblOpt(gev, gevOptCA);

	switch (solverid) {
		case CBC: 
#if STEFAN
	model->setDblParam(CbcModel::CbcMaximumSeconds,  options.getDouble("reslim"));
	model->setIntParam(CbcModel::CbcMaxNumNode,           options.getInteger("nodelim"));
	model->setDblParam(CbcModel::CbcAllowableGap,         options.getDouble ("optca"));
	model->setDblParam(CbcModel::CbcAllowableFractionGap, options.getDouble ("optcr"));
#endif
		
#ifdef COIN_HAS_CPX
		case CPLEX: {
			OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
			assert(osicpx != NULL);
			CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_TILIM, reslim);
			if (!isLP() && nodelim)
				CPXsetintparam(osicpx->getEnvironmentPtr(), CPX_PARAM_NODELIM, nodelim);
			CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_EPGAP, optcr);
			CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_EPAGAP, optca);
			
			if (gmoOptFile(gmo)) {
				char buffer[4096];
				gmoNameOptFile(gmo, buffer);
				gevLogStatPChar(gev, "Let CPLEX read option file "); gevLogStat(gev, buffer);
				int ret = CPXreadcopyparam(osicpx->getEnvironmentPtr(), buffer);
				if (ret) {
					const char* errstr = CPXgeterrorstring(osicpx->getEnvironmentPtr(), ret, buffer);
					gevLogStatPChar(gev, "Reading option file failed: ");
					if (errstr)
						gevLogStat(gev, errstr);
					else
						gevLogStat(gev, "unknown error code");
				}
			}
			
		} break;
#endif
			
#ifdef COIN_HAS_GRB
		case GUROBI:  {
			OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
			assert(osigrb != NULL);
			GRBenv* grbenv = GRBgetenv(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL));
			GRBsetdblparam(grbenv, GRB_DBL_PAR_TIMELIMIT, reslim);
			if (!isLP() && nodelim)
				GRBsetdblparam(grbenv, GRB_DBL_PAR_NODELIMIT, (double)nodelim);
			GRBsetdblparam(grbenv, GRB_DBL_PAR_MIPGAP, optcr);
			//optca not in gurobi
			
			if (gmoOptFile(gmo)) {
				char buffer[4096];
				gmoNameOptFile(gmo, buffer);
				gevLogStatPChar(gev, "Let GUROBI read option file "); gevLogStat(gev, buffer);
				int ret = GRBreadparams(grbenv, buffer);
				if (ret) {
					const char* errstr = GRBgeterrormsg(grbenv);
					gevLogStatPChar(gev, "Reading option file failed: ");
					if (errstr)
						gevLogStat(gev, errstr);
					else
						gevLogStat(gev, "unknown error");
				}
			}
			
		} break;
#endif
			
#ifdef COIN_HAS_MSK
		case MOSEK: {
			OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
			assert(osimsk != NULL);
			MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_OPTIMIZER_MAX_TIME, reslim);
			if (!isLP() && nodelim)
				MSK_putintparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_IPAR_MIO_MAX_NUM_RELAXS, nodelim);
			MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_MIO_NEAR_TOL_REL_GAP, optcr);
			MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_MIO_NEAR_TOL_ABS_GAP, optca);
			if (gmoOptFile(gmo)) {
				char buffer[4096];
				gmoNameOptFile(gmo, buffer);
				gevLogStatPChar(gev, "Let MOSEK read option file "); gevLogStat(gev, buffer);
				MSK_putstrparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_SPAR_PARAM_READ_FILE_NAME, buffer);
				MSK_readparamfile(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL));
			}
			
			break;
		}
#endif
			
#ifdef COIN_HAS_XPR
		case XPRESS: {
			OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
			assert(osixpr != NULL);
			
			XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_MAXTIME, (int)reslim);
			if (!isLP() && nodelim)
				XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_MAXNODE, nodelim);
			XPRSsetdblcontrol(osixpr->getLpPtr(), XPRS_MIPRELSTOP, optcr);
			XPRSsetdblcontrol(osixpr->getLpPtr(), XPRS_MIPABSSTOP, optca);
			if (gmoOptFile(gmo)) {
				char buffer[4096];
				gmoNameOptFile(gmo, buffer);
				gevLogStatPChar(gev, "Let XPRESS process option file "); gevLogStat(gev, buffer);
				std::ifstream optfile(buffer);
				if (!optfile.good()) {
					gevLogStat(gev, "Could not open option file.");
				} else {
					do {
						optfile.getline(buffer, 4096);
						XPRScommand(osixpr->getLpPtr(), buffer);
					} while (optfile.good());
				}
			}
			
			break;
		}
#endif
		
		default:
			gevLogStat(gev, "Encountered unsupported solver id in setupParameters.");
			return false;
	}
	
	return true;
}

bool GamsOsi::writeSolution(double cputime, double walltime) {
	bool write_solution = false;
	char buffer[255];

	switch (solverid) {
#ifdef COIN_HAS_CPX
		case CPLEX: {
			OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
			assert(osicpx);
			int stat = CPXgetstat( osicpx->getEnvironmentPtr(), osicpx->getLpPtr(OsiCpxSolverInterface::KEEPCACHED_ALL) );
			switch (stat) {
				case CPX_STAT_OPTIMAL:
				case CPX_STAT_OPTIMAL_INFEAS:
				case CPXMIP_OPTIMAL:
				case CPXMIP_OPTIMAL_INFEAS:
					write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
					if (stat == CPX_STAT_OPTIMAL_INFEAS || stat == CPXMIP_OPTIMAL_INFEAS)
						gevLogStat(gev, "Solved to optimality, but solution has infeasibilities after unscaling.");
					else
						gevLogStat(gev, "Solved to optimality.");
					break;
				
				case CPX_STAT_UNBOUNDED:
				case CPXMIP_UNBOUNDED:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
					gevLogStat(gev, "Model unbounded.");
					break;
					
				case CPX_STAT_INFEASIBLE:
				case CPX_STAT_INForUNBD:
				case CPXMIP_INFEASIBLE:
				case CPXMIP_INForUNBD:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
					gevLogStat(gev, "Model infeasible.");
					break;
					
				case CPX_STAT_NUM_BEST:
				case CPX_STAT_FEASIBLE:
					gevLogStat(gev, "Feasible solution found.");
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
					break;
				
				case CPX_STAT_ABORT_IT_LIM:
					gmoSolveStatSet(gmo, SolveStat_Iteration);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Iteration limit reached.");
					break;
					
				case CPX_STAT_ABORT_TIME_LIM:
					gmoSolveStatSet(gmo, SolveStat_Resource);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Time limit reached.");
					break;

				case CPX_STAT_ABORT_OBJ_LIM:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Objective limit reached.");
					break;
					
				case CPX_STAT_ABORT_USER:
					gmoSolveStatSet(gmo, SolveStat_User);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Stopped on user interrupt.");
					break;
					
				case CPXMIP_OPTIMAL_TOL:
					write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Solved to optimality within gap tolerances.");
					break;

				case CPXMIP_SOL_LIM:
					write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Solver);
					gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Solution limit reached.");
					break;

				case CPXMIP_NODE_LIM_FEAS:
					write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Iteration);
					gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Node limit reached, have feasible solution.");
					break;
					
				case CPXMIP_NODE_LIM_INFEAS:
					gmoSolveStatSet(gmo, SolveStat_Iteration);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Node limit reached, do not have feasible solution.");
					break;
					
				case CPXMIP_TIME_LIM_FEAS:
					write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Resource);
					gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Time limit reached, have feasible solution.");
					break;

				case CPXMIP_TIME_LIM_INFEAS:
					gmoSolveStatSet(gmo, SolveStat_Resource);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Time limit reached, do not have feasible solution.");
					break;
					
				case CPXMIP_FAIL_FEAS:
				case CPXMIP_FAIL_FEAS_NO_TREE:
				case CPXMIP_MEM_LIM_FEAS:
				case CPXMIP_ABORT_FEAS:
				case CPXMIP_FEASIBLE:
					write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Solver);
					gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Solving failed, but have feasible solution.");
					break;
					
				case CPXMIP_FAIL_INFEAS:
				case CPXMIP_FAIL_INFEAS_NO_TREE:
				case CPXMIP_MEM_LIM_INFEAS:
				case CPXMIP_ABORT_INFEAS:
					gmoSolveStatSet(gmo, SolveStat_Solver);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Solving failed, do not have feasible solution.");
					break;
			}
						
		} break;
#endif

#ifdef COIN_HAS_GRB
		case GUROBI: {
			OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
			assert(osigrb);
			int stat, nrsol;
			GRBgetintattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_INT_ATTR_SOLCOUNT, &nrsol);
		  GRBgetintattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_INT_ATTR_STATUS, &stat);
			write_solution = nrsol;
			switch (stat) {
				case GRB_OPTIMAL:
					assert(nrsol);
					gmoSolveStatSet(gmo, SolveStat_Normal);
					if (isLP()) {
						gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
						gevLogStat(gev, "Solved to optimality.");
					} else {
						double bound;
						GRBgetdblattr(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL), GRB_DBL_ATTR_OBJBOUND, &bound);
						if(fabs(bound - osi->getObjValue()) < 1e-9) {
							gevLogStat(gev, "Solved to optimality.");
							gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
						} else {
							gevLogStat(gev, "Solved to optimality within tolerances.");
							gmoModelStatSet(gmo, ModelStat_Integer);
						}
					}
					break;
					
				case GRB_INFEASIBLE:
					assert(!nrsol);
				case GRB_INF_OR_UNBD:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
					gevLogStat(gev, "Model infeasible.");
					break;
					
				case GRB_UNBOUNDED:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
					gevLogStat(gev, "Model unbounded.");
					break;
					
				case GRB_CUTOFF:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Objective limit reached.");
					break;
					
				case GRB_ITERATION_LIMIT:
					gmoSolveStatSet(gmo, SolveStat_Iteration);
					if (nrsol) {
						gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
						gevLogStat(gev, "Iteration limit reached, but have feasible solution.");
					} else {
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Iteration limit reached.");
					}
					break;

				case GRB_NODE_LIMIT:
					gmoSolveStatSet(gmo, SolveStat_Iteration);
					assert(!isLP());
					if (nrsol) {
						gmoModelStatSet(gmo, ModelStat_Integer);
						gevLogStat(gev, "Node limit reached, but have feasible solution.");
					} else {
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Node limit reached.");
					}
					break;
					
				case GRB_TIME_LIMIT:
					gmoSolveStatSet(gmo, SolveStat_Resource);
					if (nrsol) {
						gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
						gevLogStat(gev, "Time limit reached, but have feasible solution.");
					} else {
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Time limit reached.");
					}
					break;
					
				case GRB_SOLUTION_LIMIT:
					gmoSolveStatSet(gmo, SolveStat_Solver);
					if (nrsol) {
						gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
						gevLogStat(gev, "Solution limit reached.");
					} else {
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Solution limit reached, but do not have solution.");
					}
					break;
					
				case GRB_INTERRUPTED:
					gmoSolveStatSet(gmo, SolveStat_User);
					if (nrsol) {
						gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
						gevLogStat(gev, "User interrupt, have feasible solution.");
					} else {
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "User interrupt.");
					}
					break;
					
				case GRB_NUMERIC:
					gmoSolveStatSet(gmo, SolveStat_Solver);
					if (nrsol) {
						gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
						gevLogStat(gev, "Stopped on numerical difficulties, but have feasible solution.");
					} else {
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Stopped on numerical difficulties.");
					}
					break;
					
				case GRB_LOADED:
				default:
					gmoSolveStatSet(gmo, SolveStat_Solver);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Solving failed, unknown status.");
					break;
			}
			break;
		}
#endif
		
#ifdef COIN_HAS_MSK
		case MOSEK: {
			OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
			assert(osimsk);

			MSKsolstae status;
		  MSKsoltypee solution;

			int res;
			if (isLP()) {
				solution = MSK_SOL_BAS;
				MSK_solutiondef(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &res);
				if (res == MSK_RES_OK ) {
					solution = MSK_SOL_ITR;
					MSK_solutiondef(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &res);
					if (res == MSK_RES_OK) {
						gmoSolveStatSet(gmo, SolveStat_SolverErr);
						gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
						gevLogStat(gev, "Failure retrieving solution status.");
						break;
					}
				}
			} else {
				solution = MSK_SOL_ITG;
				MSK_solutiondef(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &res);
				if (res == MSK_RES_OK) {
					gmoSolveStatSet(gmo, SolveStat_SolverErr);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					gevLogStat(gev, "Failure retrieving solution status.");
					break;
				}
			}
			
		  MSK_getsolution(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, NULL, &status,
		  	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		  
		  switch (status) {
        case MSK_SOL_STA_NEAR_INTEGER_OPTIMAL:
        case MSK_SOL_STA_NEAR_OPTIMAL:
        	write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Normal);
					if( isLP() )
						gmoModelStatSet(gmo, ModelStat_OptimalLocal);
					else
						gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Solved nearly to optimality.");
        	break;
        	
        case MSK_SOL_STA_PRIM_FEAS:
        case MSK_SOL_STA_PRIM_AND_DUAL_FEAS:
        	write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Normal);
					if( isLP() )
						gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
					else
						gmoModelStatSet(gmo, ModelStat_Integer);
					gevLogStat(gev, "Solved to feasibility.");
					break;
        	        	
        case MSK_SOL_STA_OPTIMAL:
        case MSK_SOL_STA_INTEGER_OPTIMAL:
        	write_solution = true;
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
					gevLogStat(gev, "Solved to optimality.");
					break;
        	
        case MSK_SOL_STA_PRIM_INFEAS_CER:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
					gevLogStat(gev, "Model is infeasible.");
					break;
        	
        case MSK_SOL_STA_DUAL_INFEAS_CER:
					gmoSolveStatSet(gmo, SolveStat_Normal);
					gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
					gevLogStat(gev, "Model is unbounded.");
					break;

        case MSK_SOL_STA_DUAL_FEAS:
        case MSK_SOL_STA_NEAR_PRIM_FEAS:
        case MSK_SOL_STA_NEAR_DUAL_FEAS:
        case MSK_SOL_STA_NEAR_PRIM_AND_DUAL_FEAS:
        case MSK_SOL_STA_NEAR_PRIM_INFEAS_CER:
        case MSK_SOL_STA_NEAR_DUAL_INFEAS_CER:
					gmoSolveStatSet(gmo, SolveStat_Solver);
					gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
					gevLogStat(gev, "Stopped before feasibility or infeasibility proven.");
					break;

        case MSK_SOL_STA_UNKNOWN:
        default:
					gmoSolveStatSet(gmo, SolveStat_Solver);
					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
					gevLogStat(gev, "Solve status unknown.");
		  }
		  
			break;
		}
#endif
		
#ifdef COIN_HAS_XPR
		case XPRESS: {
			OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
			assert(osixpr);
			int status;
			if (isLP()) {
				XPRSgetintattrib(osixpr->getLpPtr(), XPRS_LPSTATUS, &status);

				switch (status) {
					case XPRS_LP_OPTIMAL:
						write_solution = true;
						gmoSolveStatSet(gmo, SolveStat_Normal);
						gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
						gevLogStat(gev, "Solved to optimality.");
						break;
						
					case XPRS_LP_INFEAS:
						gmoSolveStatSet(gmo, SolveStat_Normal);
						gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
						gevLogStat(gev, "Model is infeasible.");
						break;
						
					case XPRS_LP_CUTOFF:
						gmoSolveStatSet(gmo, SolveStat_Solver);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Objective limit reached.");
						break;
						
					case XPRS_LP_UNBOUNDED:
						gmoSolveStatSet(gmo, SolveStat_Normal);
						gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
						gevLogStat(gev, "Model is unbounded.");
						break;
						
					case XPRS_LP_CUTOFF_IN_DUAL:
						gmoSolveStatSet(gmo, SolveStat_Solver);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Dual objective limit reached.");
						break;

					case XPRS_LP_UNFINISHED:
					case XPRS_LP_UNSOLVED:
#if XPVERSION > 20
					case XPRS_LP_NONCONVEX:
#endif
						gmoSolveStatSet(gmo, SolveStat_Solver);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Model not solved.");
						break;
						
					default:
						gmoSolveStatSet(gmo, SolveStat_Solver);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Solution status unknown.");
						break;
				}
			} else {
				XPRSgetintattrib(osixpr->getLpPtr(), XPRS_MIPSTATUS, &status);
				switch (status) {
					case XPRS_MIP_LP_NOT_OPTIMAL:
						gmoSolveStatSet(gmo, SolveStat_Solver);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "LP relaxation not solved to optimality.");
						break;
						
					case XPRS_MIP_LP_OPTIMAL:
						gmoSolveStatSet(gmo, SolveStat_Solver);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "Only LP relaxation solved.");
						break;
						
					case XPRS_MIP_NO_SOL_FOUND:
						gmoSolveStatSet(gmo, SolveStat_Normal);
						gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
						gevLogStat(gev, "No solution found.");
						break;

					case XPRS_MIP_SOLUTION:
						write_solution = true;
						gmoSolveStatSet(gmo, SolveStat_Normal);
						gmoModelStatSet(gmo, ModelStat_Integer);
						gevLogStat(gev, "Found integer feasible solution.");
						break;
						
					case XPRS_MIP_INFEAS:
						gmoSolveStatSet(gmo, SolveStat_Normal);
						gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
						gevLogStat(gev, "Model is infeasible.");
						break;
						
					case XPRS_MIP_OPTIMAL:
						write_solution = true;
						gmoSolveStatSet(gmo, SolveStat_Normal);
						double bestBound;
				    XPRSgetdblattrib(osixpr->getLpPtr(), XPRS_BESTBOUND, &bestBound);
				    if(fabs(bestBound - osixpr->getObjValue()) < 1e-9) {
				    	gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
				    	gevLogStat(gev, "Solved to optimality.");
				    } else {
				    	gmoModelStatSet(gmo, ModelStat_Integer);
				    	gevLogStat(gev, "Solved to optimality within tolerances.");
				    }
						break;
				}
			}			
			break;
		}
#endif		

		default:
	
			try {
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
					gevLogStat(gev, "Model status unknown.");
				}
			} catch (CoinError error) {
				gevLogStatPChar(gev, "Exception caught when requesting solution status: ");
				gevLogStat(gev, error.message().c_str());
				gmoSolveStatSet(gmo, SolveStat_Solver);
				gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
			}
	}

	try {
		if (isLP())
			gmoSetHeadnTail(gmo, Hiterused, osi->getIterationCount());
		gmoSetHeadnTail(gmo, HresUsed, cputime);
#if STEFAN
		gmoSetHeadnTail(gmo, Tmipbest, model->getBestPossibleObjValue());
		gmoSetHeadnTail(gmo, Tmipnod, model->getNodeCount());
#endif
	} catch (CoinError error) {
		gevLogStatPChar(gev, "Exception caught when requesting solution statistics: ");
		gevLogStat(gev, error.message().c_str());
	}

	if (write_solution) {
		if (isLP()) {
			if (!gamsOsiStoreSolution(gmo, *osi, false))
				return false;
		} else { // is MIP -> store only primal values for now
			double* rowprice = CoinCopyOfArrayOrZero((double*)NULL, gmoM(gmo));
			try {
				gmoSetHeadnTail(gmo, HobjVal, osi->getObjValue());
				gmoSetSolution2(gmo, osi->getColSolution(), rowprice);
			} catch (CoinError error) {
				gevLogStatPChar(gev, "Exception caught when requesting primal solution values: ");
				gevLogStat(gev, error.message().c_str());
			}
			delete[] rowprice;
		}
	}

	if (!isLP()) {
		if (write_solution) {
			snprintf(buffer, 255, "MIP solution: %21.10g   (%g seconds)", gmoGetHeadnTail(gmo, HobjVal), cputime);
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

DllExport GamsOsi* STDCALL createNewGamsOsiGurobi() {
   return new GamsOsi(GamsOsi::GUROBI);
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
osi_C_interface(ogu, GamsOsi::GUROBI)
osi_C_interface(oxp, GamsOsi::XPRESS)
osi_C_interface(omk, GamsOsi::MOSEK)
