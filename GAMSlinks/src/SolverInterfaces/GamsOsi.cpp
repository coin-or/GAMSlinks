// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOsi.hpp"
#include "GamsOsiCplex.h"
#include "GamsOsiGlpk.h"
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
#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "gmspal.h"
#include "gevlice.h"
#endif
/* value for not available/applicable */
#if GMOAPIVERSION >= 7
#define GMS_SV_NA     gmoValNA(gmo)
#else
#define GMS_SV_NA     2.0E300
#endif

#if GMOAPIVERSION < 8
#define Hresused     HresUsed
#define Hobjval      HobjVal
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
#include "cplex.h"
#endif
#ifdef COIN_HAS_GLPK
#include "OsiGlpkSolverInterface.hpp"
extern "C" {
#include "glpk.h" // to get version numbers
}
#endif
#ifdef COIN_HAS_GRB
#include "OsiGrbSolverInterface.hpp"
extern "C" {
#include "gurobi_c.h" // to get version numbers
}
#endif
#ifdef COIN_HAS_XPR
#include "OsiXprSolverInterface.hpp"
#include "xprs.h"
extern "C" void STDCALL XPRScommand(XPRSprob, char*);
#endif
#ifdef COIN_HAS_MSK
#include "OsiMskSolverInterface.hpp"
#include "mosek.h"
#endif

static int glpkprint(void* info, const char* msg) {
  assert(info != NULL);
  gevLogStatPChar((gevHandle_t)info, msg);
  return 1;
}

GamsOsi::GamsOsi(GamsOsi::OSISOLVER solverid_)
: gmo(NULL), gev(NULL), msghandler(NULL), osi(NULL), solverid(solverid_)
{
	switch (solverid) {

#ifdef COIN_HAS_CBC
		case CBC:
			sprintf(osi_message, "OsiCbc (Osi library 0.102, CBC library 2.4)\nOsi link written by J. Forrest. Osi is part of COIN-OR.\n");
			break;
#endif

#ifdef COIN_HAS_CPX
		case CPLEX:
			sprintf(osi_message, "OsiCplex (Osi library 0.102, CPLEX library %.2f)\nOsi link written by T. Achterberg. Osi is part of COIN-OR.\n", CPX_VERSION/100.);
			break;
#endif

#ifdef COIN_HAS_GLPK
		case GLPK:
		  sprintf(osi_message, "OsiGlpk (Osi library 0.102, GLPK library %d.%d)\nOsi link written by Vivian De Smedt and Braden Hunsaker. Osi is part of COIN-OR.\n", GLP_MAJOR_VERSION, GLP_MINOR_VERSION);
		  break;
#endif

#ifdef COIN_HAS_GRB
		case GUROBI:
			sprintf(osi_message, "OsiGurobi (Osi library 0.102, GUROBI library %d.%d.%d)\nOsi link written by S. Vigerske. Osi is part of COIN-OR.\n", GRB_VERSION_MAJOR, GRB_VERSION_MINOR, GRB_VERSION_TECHNICAL);
			break;
#endif

#ifdef COIN_HAS_MSK
		case MOSEK:
			sprintf(osi_message, "OsiMosek (Osi library 0.102, MOSEK library %d.%d)\nwritten by Bo Jensen. Osi is part of COIN-OR.\n", MSK_VERSION_MAJOR, MSK_VERSION_MINOR);
			break;
#endif

#ifdef COIN_HAS_XPR
		case XPRESS:
			sprintf(osi_message, "OsiXpress (Osi library 0.102, FICO Xpress-Optimizer library %d)\nOsi is part of COIN-OR.\n", XPVERSION);
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
  char buffer[1024];

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
	switch( solverid )
	{
		case CPLEX:
#include "coinlibdCL7svn.h"
			break;
		case GLPK:
#include "coinlibdCL6svn.h"
			break;
		case GUROBI:
#include "coinlibdCL8svn.h"
			break;
		case MOSEK:
#include "coinlibdCL9svn.h"
			break;
		case XPRESS:
#include "coinlibdCLAsvn.h"
			break;
	}
	auditGetLine(buffer, sizeof(buffer));
	gevLogStat(gev, "");
	gevLogStat(gev, buffer);
	gevStatAudit(gev, buffer);
#endif

	gevLogStat(gev, "");
	gevLogStatPChar(gev, getWelcomeMessage());

	if (solverid == GLPK) {
	  options.setGMO(gmo);
	  if (opt) {
	    options.setOpt(opt);
	  } else {
	    gmoNameOptFile(gmo, buffer);
#ifdef GAMS_BUILD
	    options.readOptionsFile("glpk", gmoOptFile(gmo) ? buffer : NULL);
#else
	    options.readOptionsFile("glpk", gmoOptFile(gmo) ? buffer : NULL);
#endif
	  }
	  if (options.isDefined("reslim"))  gevSetDblOpt(gev, gevResLim,  options.getDouble ("reslim"));
	  if (options.isDefined("iterlim")) gevSetIntOpt(gev, gevIterLim, options.getInteger("iterlim"));
	  if (options.isDefined("optcr"))   gevSetDblOpt(gev, gevOptCR,   options.getDouble ("optcr"));
	}

#ifdef GAMS_BUILD
	if(!checkLicense(gmo)) {
    gmoSolveStatSet(gmo, SolveStat_License);
    gmoModelStatSet(gmo, ModelStat_LicenseError);
    return 1;
	}
#endif

//#ifdef GAMS_BUILD
//#define GEVPTR gev
//#include "cmagic2.h"
//	if (licenseCheck(gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo))) {
//		char msg[256];
//		gevLogStat(gev, "The license check failed:\n");
//		while (licenseGetMessage(msg, sizeof(msg)))
//			gevLogStat(gev,msg);
//	  gmoSolveStatSet(gmo, SolveStat_License);
//	  gmoModelStatSet(gmo, ModelStat_LicenseError);
//	  return 1;
//	}
//#endif

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
                          if (!registerGamsCplexLicense(gmo)) {
                            gmoSolveStatSet(gmo, SolveStat_License);
                            gmoModelStatSet(gmo, ModelStat_LicenseError);
                            return 1;
                          }
//#ifdef GAMS_BUILD
//                                int rc, cp_l=0, cp_m=0, cp_q=0, cp_p=0;
//				CPlicenseInit_t initType;
//
//				/* Cplex license setup */
//                                rc = gevcplexlice(gev,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),
//                                                  gmoNDisc(gmo), 0, &initType, &cp_l, &cp_m, &cp_q, &cp_p);
//				if (rc || (0==rc && ((0==cp_m && gmoNDisc(gmo)) || (0==cp_q && gmoNLNZ(gmo)))))
//                                        gevLogStat(gev, "Trying to use Cplex standalone license.\n");
//
//#endif
				osi = new OsiCpxSolverInterface;
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/CPLEX interface.\n");
				return 1;
#endif
			} break;

			case GLPK: {
#ifdef COIN_HAS_GLPK
			  osi = new OsiGlpkSolverInterface();
#else
			  gevLogStat(gev, "GamsOsi compiled without Osi/Glpk interface.\n");
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
						gmoNDisc(gmo), 0, &initType))
					gevLogStat(gev, "Trying to use Gurobi standalone license.\n");
#endif
				osi = new OsiGrbSolverInterface(grbenv); // this lets OsiGrb take over ownership of grbenv (if not NULL), so it will be freed when osi is deleted
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
                                int rc;

				if (MSK_makeenv(&mskenv,NULL, NULL,NULL,NULL)) {
					gevLogStat(gev, "Failed to create Mosek environment.");
					gmoSolveStatSet(gmo, SolveStat_SetupErr);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					return 1;
				}
				rc = gevmoseklice(gev,mskenv,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo), 0, &initType);
				if (rc || (0==rc && (MBGAMS==initType && gmoNDisc(gmo))))
					gevLogStat(gev, "Trying to use Mosek standalone license.\n");

				if (MSK_initenv(mskenv)) {
					gevLogStat(gev, "Failed to initialize Mosek environment.");
					gmoSolveStatSet(gmo, SolveStat_SetupErr);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					return 1;
				}
				osi = new OsiMskSolverInterface(mskenv);
#else
				osi = new OsiMskSolverInterface();
#endif
#else
				gevLogStat(gev, "GamsOsi compiled without Osi/MOSEK interface.\n");
				return 1;
#endif
			} break;

			case XPRESS: {
#ifdef COIN_HAS_XPR
#ifdef GAMS_BUILD
				XPlicenseInit_t initType;
				char msg[1024];

				/* Xpress license setup */
				if (gevxpresslice(gev,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),
						gmoNDisc(gmo), 0, &initType, msg, sizeof(msg)))
					gevLogStat(gev, "Trying to use Xpress standalone license.\n");
#endif
				OsiXprSolverInterface* osixpr = new OsiXprSolverInterface(gmoM(gmo), gmoNZ(gmo));
				if (!osixpr->getNumInstances()) {
					gevLogStat(gev, "Failed to setup XPRESS instance. Maybe you do not have a license?\n");
					gmoSolveStatSet(gmo, SolveStat_SetupErr);
					gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
					return 1;
				}
				osi = osixpr;

				XPRSgetbanner(msg);
				gevLog(gev, msg);
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
		gmoSolveStatSet(gmo, SolveStat_SetupErr);
		gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
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
	msghandler->setPrefix(false);
	osi->passInMessageHandler(msghandler);
	osi->setHintParam(OsiDoReducePrint, true,  OsiHintTry);

	/* gurobi does not support message callbacks (yet), so we turn off message printing if lo=2 */
	if (gevGetIntOpt(gev, gevLogOption) == 0 || (solverid == GUROBI && gevGetIntOpt(gev, gevLogOption) == 2))
		msghandler->setLogLevel(0);

	if (!setupProblem(*osi)) {
		gevLogStat(gev, "Error setting up problem...");
		return -1;
	}

	if (gmoN(gmo) && !setupStartingPoint()) {
		gevLogStat(gev, "Error setting up starting point.");
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
#ifdef COIN_HAS_GLPK
  if (solverid == GLPK)
    glp_term_hook(glpkprint, gev);
#endif

  if (gevGetIntOpt(gev, gevInteger3))
  {
  	char buffer[1024];
  	gmoNameInput(gmo, buffer);
  	//gevLogPChar(gev, "Writing MPS file ");
  	//gevLog(gev, buffer);
  	osi->writeMps(buffer);
  }

	double start_cputime  = CoinCpuTime();
	double start_walltime = CoinWallclockTime();

	try {
		if (isLP())
			osi->initialSolve();
		if (!isLP())
			osi->branchAndBound();
	} catch (CoinError error) {
		gevLogStatPChar(gev, "Exception caught when solving problem: ");
		gevLogStat(gev, error.message().c_str());
	}

	double end_cputime  = CoinCpuTime();
	double end_walltime = CoinWallclockTime();

	gevLogStat(gev, "");
	bool solwritten;
	writeSolution(end_cputime - start_cputime, end_walltime - start_walltime, solwritten);

	if (!isLP() && gevGetIntOpt(gev, gevInteger1) && solwritten)
		solveFixed();

#ifdef COIN_HAS_GLPK
  if (solverid == GLPK)
    glp_term_hook(NULL, NULL);
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

	if (gmoDict(gmo)!=NULL && gevGetIntOpt(gev, gevInteger2)) {
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
#if GMOAPIVERSION >= 8
		switch (gmoGetVarStatOne(gmo, j)) {
			case Bstat_Basic: // this seem to mean that variable should be basic
#else
     switch (gmoGetVarBasOne(gmo, j)) {
         case 0: // this seem to mean that variable should be basic
#endif
				if (nbas < gmoM(gmo)) {
					basis.setStructStatus(j, CoinWarmStartBasis::basic);
					++nbas;
				} else if (gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo))
					basis.setStructStatus(j, CoinWarmStartBasis::isFree);
				else if (gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) || fabs(gmoGetVarLowerOne(gmo, j) - varlevel[j]) < fabs(gmoGetVarUpperOne(gmo, j) - varlevel[j]))
					basis.setStructStatus(j, CoinWarmStartBasis::atLowerBound);
				else
					basis.setStructStatus(j, CoinWarmStartBasis::atUpperBound);
				break;
#if GMOAPIVERSION >= 8
			case Bstat_Lower:
			case Bstat_Upper:
			case Bstat_Super:
#else
			case 1:
#endif
				if (gmoGetVarLowerOne(gmo, j) <= gmoMinf(gmo) && gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo))
					basis.setStructStatus(j, CoinWarmStartBasis::isFree);
				else if (gmoGetVarUpperOne(gmo, j) >= gmoPinf(gmo) || fabs(gmoGetVarLOne(gmo, j) - gmoGetVarLowerOne(gmo, j)) < fabs(gmoGetVarUpperOne(gmo, j) - gmoGetVarLOne(gmo, j)))
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
#if GMOAPIVERSION >= 8
		switch (gmoGetEquStatOne(gmo, j)) {
			case Bstat_Basic:
#else
      switch (gmoGetEquBasOne(gmo, j)) {
         case 0:
#endif
				if (nbas < gmoM(gmo)) {
					basis.setArtifStatus(j, CoinWarmStartBasis::basic);
					++nbas;
				} else
					basis.setArtifStatus(j, gmoGetEquTypeOne(gmo, j) == equ_G ? CoinWarmStartBasis::atLowerBound : CoinWarmStartBasis::atUpperBound);
				break;
#if GMOAPIVERSION >= 8
			case Bstat_Lower:
			case Bstat_Upper:
			case Bstat_Super:
#else
			case 1:
#endif
				basis.setArtifStatus(j, gmoGetEquTypeOne(gmo, j) == equ_G ? CoinWarmStartBasis::atLowerBound : CoinWarmStartBasis::atUpperBound);
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
		if ((solverid != GUROBI && solverid != MOSEK) || nbas == gmoM(gmo)) {
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
		} else {
			gevLog(gev, "Did not attempt to register incomplete basis.\n");
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
#ifdef COIN_HAS_CBC
		case CBC: {
			OsiCbcSolverInterface* osicbc = dynamic_cast<OsiCbcSolverInterface*>(osi);
			osicbc->getModelPtr()->setDblParam(CbcModel::CbcMaximumSeconds,       reslim);
			osicbc->getModelPtr()->setIntParam(CbcModel::CbcMaxNumNode,           nodelim);
			osicbc->getModelPtr()->setDblParam(CbcModel::CbcAllowableGap,         optca);
			osicbc->getModelPtr()->setDblParam(CbcModel::CbcAllowableFractionGap, optcr);
		} break;
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

#ifdef COIN_HAS_GLPK
		case GLPK: {
		  OsiGlpkSolverInterface* osiglpk = dynamic_cast<OsiGlpkSolverInterface*>(osi);
		  assert(osiglpk != NULL);

		  LPX* glpk_model = osiglpk->getModelPtr();

		  if (reslim > 1e+6) { // GLPK cannot handle very large timelimits, so we run it without limit then
		    gevLogStat(gev, "Time limit too large. GLPK will run without timelimit.");
		    reslim = -1;
		  }
		  lpx_set_real_parm(glpk_model, LPX_K_TMLIM, reslim);

		  if (!isLP() && nodelim)
		    gevLogStat(gev, "Cannot set node limit for GLPK. Node limit ignored.");

		  lpx_set_real_parm(glpk_model, LPX_K_MIPGAP, optcr);
		  // optca not in glpk

		  osiglpk->setHintParam(OsiDoReducePrint, false, OsiForceDo); // GLPK loglevel 3

      // GLPK LP parameters

		  if (!osiglpk->setIntParam(OsiMaxNumIteration, options.getInteger("iterlim")))
		    gevLogStat(gev, "Failed to set iteration limit for GLPK.");

		  if (!osiglpk->setDblParam(OsiDualTolerance, options.getDouble("tol_dual")))
		    gevLogStat(gev, "Failed to set dual tolerance for GLPK.");

		  if (!osiglpk->setDblParam(OsiPrimalTolerance, options.getDouble("tol_primal")))
		    gevLogStat(gev, "Failed to set primal tolerance for GLPK.");

		  // more parameters
		  char buffer[255];
		  options.getString("scaling", buffer);
		  if (strcmp(buffer, "off")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_SCALE, 0);
		  else if (strcmp(buffer, "equilibrium")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_SCALE, 1);
		  else if (strcmp(buffer, "mean")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_SCALE, 2);
		  else if (strcmp(buffer, "meanequilibrium")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_SCALE, 3); // default (set in OsiGlpk)

		  options.getString("startalg", buffer);
		  osiglpk->setHintParam(OsiDoDualInInitial, (strcmp(buffer, "dual")==0), OsiForceDo);

		  osiglpk->setHintParam(OsiDoPresolveInInitial, options.getBool("presolve"), OsiForceDo);

		  options.getString("pricing", buffer);
		  if (strcmp(buffer, "textbook")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_PRICE, 0);
		  else if (strcmp(buffer, "steepestedge")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_PRICE, 1); // default

		  options.getString("factorization", buffer);
		  if (strcmp(buffer, "forresttomlin")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_BFTYPE, 1);
		  else if (strcmp(buffer, "bartelsgolub")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_BFTYPE, 2);
		  else if (strcmp(buffer, "givens")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_BFTYPE, 3);

		  // GLPK MIP parameters

		  options.getString("backtracking", buffer);
		  if (strcmp(buffer, "depthfirst")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_BTRACK, 0);
		  else if (strcmp(buffer, "breadthfirst")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_BTRACK, 1);
		  else if (strcmp(buffer, "bestprojection")==0)
		    lpx_set_int_parm(glpk_model, LPX_K_BTRACK, 2);

		  int cutindicator=0;
		  switch (options.getInteger("cuts")) {
		    case -1 : break; // no cuts
		    case  1 : cutindicator=LPX_C_ALL; break; // all cuts
		    case  0 : // user defined cut selection
		      if (options.getBool("covercuts")) cutindicator|=LPX_C_COVER;
		      if (options.getBool("cliquecuts")) cutindicator|=LPX_C_CLIQUE;
		      if (options.getBool("gomorycuts")) cutindicator|=LPX_C_GOMORY;
		      if (options.getBool("mircuts")) cutindicator|=LPX_C_MIR;
		      break;
		    default: ;
		  }
		  lpx_set_int_parm(glpk_model, LPX_K_USECUTS, cutindicator);

		  double tol_integer=options.getDouble("tol_integer");
		  if (tol_integer>0.001) {
		    gevLog(gev, "Cannot use tol_integer of larger then 0.001. Setting integer tolerance to 0.001.");
		    tol_integer=0.001;
		  }
		  lpx_set_real_parm(glpk_model, LPX_K_TOLINT, tol_integer);

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
#if GRB_VERSION_MAJOR >= 3
         GRBsetdblparam(grbenv, GRB_DBL_PAR_MIPGAPABS, optca);
#endif

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

bool GamsOsi::writeSolution(double cputime, double walltime, bool& write_solution) {
	char buffer[255];

	write_solution = false;

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

#ifdef COIN_HAS_GLPK
      case GLPK: {
         OsiGlpkSolverInterface* osiglpk = dynamic_cast<OsiGlpkSolverInterface*>(osi);
         assert(osiglpk);

         if (osiglpk->isTimeLimitReached()) {
            gmoSolveStatSet(gmo, SolveStat_Resource);
            if (osiglpk->isFeasible()) {
               write_solution = true;
               gevLogStat(gev, "Time limit reached, have feasible solution.");
               gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
            } else {
               gevLogStat(gev, "Time limit reached, do not have feasible solution.");
               gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
            }
         } else if (osiglpk->isIterationLimitReached()) {
            gmoSolveStatSet(gmo, SolveStat_Iteration);
            if (osiglpk->isFeasible()) {
               write_solution = true;
               gevLogStat(gev, "Iteration limit reached, have feasible solution.");
               gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
            } else {
               gevLogStat(gev, "Iteration limit reached, do not have feasible solution.");
               gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
            }
         }  else if (osiglpk->isProvenPrimalInfeasible()) {
            gevLogStat(gev, "Model infeasible.");
            gmoSolveStatSet(gmo, SolveStat_Normal);
            gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
         } else if (osiglpk->isProvenDualInfeasible()) { // GAMS doesn't have dual infeasible, so we hope for the best and call it unbounded
            gevLogStat(gev, "Model unbounded.");
            gmoSolveStatSet(gmo, SolveStat_Normal);
            gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
         } else if (osiglpk->isProvenOptimal()) {
            write_solution = true;
            gevLogStat(gev, "Optimal solution found.");
            gmoSolveStatSet(gmo, SolveStat_Normal);
            gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
         } else if (osiglpk->isFeasible()) {
            write_solution = true;
            gevLogStat(gev, "Feasible solution found.");
            gmoSolveStatSet(gmo, SolveStat_Normal);
            gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
         } else if (osiglpk->isAbandoned()) {
            gevLogStat(gev, "Model abandoned.");
            gmoSolveStatSet(gmo, SolveStat_SolverErr);
            gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
         } else {
            gevLogStat(gev, "Unknown solve outcome.");
            gmoSolveStatSet(gmo, SolveStat_SystemErr);
            gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
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

#if GRB_VERSION_MAJOR >= 3
            case GRB_SUBOPTIMAL:
               gmoSolveStatSet(gmo, SolveStat_Solver);
               if (nrsol) {
                  gmoModelStatSet(gmo, isLP() ? ModelStat_NonOptimalIntermed : ModelStat_Integer);
                  gevLogStat(gev, "Stopped on suboptimal but feasible solution.");
               } else {
                  gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
                  gevLogStat(gev, "Stopped at suboptimal point.");
               }
               break;
#endif

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

			MSKprostae probstatus;
			MSKsolstae solstatus;
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

		  MSK_getsolution(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), solution, &probstatus, &solstatus,
		  	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		  switch (solstatus) {
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
        	switch (probstatus) {
        		case MSK_PRO_STA_DUAL_FEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    					gevLogStat(gev, "Model is dual feasible.");
    					break;

        		case MSK_PRO_STA_DUAL_INFEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_UnboundedNoSolution);
    					gevLogStat(gev, "Model is dual infeasible (probably unbounded).");
    					break;

        		case MSK_PRO_STA_ILL_POSED:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    					gevLogStat(gev, "Model is ill posed.");
    					break;

        		case MSK_PRO_STA_NEAR_DUAL_FEAS:
        		case MSK_PRO_STA_NEAR_PRIM_AND_DUAL_FEAS:
        		case MSK_PRO_STA_NEAR_PRIM_FEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
    					gevLogStat(gev, "Stopped before feasibility or infeasibility proven.");
    					break;

        		case MSK_PRO_STA_PRIM_AND_DUAL_FEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
    					gevLogStat(gev, "Model is primal and dual feasible.");
    					break;

        		case MSK_PRO_STA_PRIM_AND_DUAL_INFEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
    					gevLogStat(gev, "Model is primal and dual infeasible.");
    					break;

        		case MSK_PRO_STA_PRIM_FEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed);
    					gevLogStat(gev, "Model is primal feasible.");
    					break;

        		case MSK_PRO_STA_PRIM_INFEAS:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
    					gevLogStat(gev, "Model is primal infeasible.");
    					break;

        		case MSK_PRO_STA_PRIM_INFEAS_OR_UNBOUNDED:
    					gmoSolveStatSet(gmo, SolveStat_Normal);
    					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    					gevLogStat(gev, "Model is infeasible or unbounded.");
    					break;

        		case MSK_PRO_STA_UNKNOWN:
        		default:
    					gmoSolveStatSet(gmo, SolveStat_Solver);
    					gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
    					gevLogStat(gev, "Solve status unknown.");
    					break;
        	}
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
		gmoSetHeadnTail(gmo, Hresused, cputime);
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
			if (!gamsOsiStoreSolution(gmo, *osi, false)) {
				write_solution = false;
				return false;
			}
		} else { // is MIP -> store only primal values for now
			double* rowprice = CoinCopyOfArrayOrZero((double*)NULL, gmoM(gmo));
			try {
				gmoSetHeadnTail(gmo, Hobjval, osi->getObjValue());
				gmoSetSolution2(gmo, osi->getColSolution(), rowprice);
			} catch (CoinError error) {
				gevLogStatPChar(gev, "Exception caught when requesting primal solution values: ");
				gevLogStat(gev, error.message().c_str());
				write_solution = false;
			}
			delete[] rowprice;
		}
	}

	if (!isLP()) {
		if (write_solution) {
			snprintf(buffer, 255, "MIP solution: %21.10g   (%g seconds)", gmoGetHeadnTail(gmo, Hobjval), cputime);
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

bool GamsOsi::solveFixed() {
	gevLogStat(gev, "\nSolving LP obtained from MIP by fixing discrete variables to values in solution.\n");

	for (int i = 0; i < gmoN(gmo); ++i)
		if (gmoGetVarTypeOne(gmo, i) != var_X) {
			double solval, dummy;
			int dummy2;
			gmoGetSolutionVarRec(gmo, i, &solval, &dummy, &dummy2, &dummy2);
			osi->setColBounds(i, solval, solval);
		}

	if (gevGetDblOpt(gev, gevReal1)) {
		switch (solverid) {
#ifdef COIN_HAS_CBC
			case CBC: {
				OsiCbcSolverInterface* osicbc = dynamic_cast<OsiCbcSolverInterface*>(osi);
				osicbc->getModelPtr()->setDblParam(CbcModel::CbcMaximumSeconds, gevGetDblOpt(gev, gevReal1));
			} break;
#endif

#ifdef COIN_HAS_CPX
			case CPLEX: {
				OsiCpxSolverInterface* osicpx = dynamic_cast<OsiCpxSolverInterface*>(osi);
				assert(osicpx != NULL);
				CPXsetdblparam(osicpx->getEnvironmentPtr(), CPX_PARAM_TILIM, gevGetDblOpt(gev, gevReal1));
			} break;
#endif

#ifdef COIN_HAS_GLPK
			case GLPK: {
			  OsiGlpkSolverInterface* osiglpk = dynamic_cast<OsiGlpkSolverInterface*>(osi);
			  assert(osiglpk != NULL);
			  if (gevGetDblOpt(gev, gevReal1) > 1e+6) { // GLPK cannot handle very large timelimits, so we run it without limit then
			    gevLogStat(gev, "Time limit for final LP solve too large. GLPK will run without timelimit.");
				  lpx_set_real_parm(osiglpk->getModelPtr(), LPX_K_TMLIM, -1);
			  }
			  lpx_set_real_parm(osiglpk->getModelPtr(), LPX_K_TMLIM, gevGetDblOpt(gev, gevReal1));
			} break;
	#endif

	#ifdef COIN_HAS_GRB
			case GUROBI:  {
				OsiGrbSolverInterface* osigrb = dynamic_cast<OsiGrbSolverInterface*>(osi);
				assert(osigrb != NULL);
				GRBsetdblparam(GRBgetenv(osigrb->getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ALL)), GRB_DBL_PAR_TIMELIMIT, gevGetDblOpt(gev, gevReal1));
			} break;
	#endif

	#ifdef COIN_HAS_MSK
			case MOSEK: {
				OsiMskSolverInterface* osimsk = dynamic_cast<OsiMskSolverInterface*>(osi);
				assert(osimsk != NULL);
				MSK_putdouparam(osimsk->getLpPtr(OsiMskSolverInterface::KEEPCACHED_ALL), MSK_DPAR_OPTIMIZER_MAX_TIME, gevGetDblOpt(gev, gevReal1));
				break;
			}
	#endif

	#ifdef COIN_HAS_XPR
			case XPRESS: {
				OsiXprSolverInterface* osixpr = dynamic_cast<OsiXprSolverInterface*>(osi);
				assert(osixpr != NULL);
				XPRSsetintcontrol(osixpr->getLpPtr(), XPRS_MAXTIME, (int)gevGetDblOpt(gev, gevReal1));
				break;
			}
	#endif

			default:
				gevLogStat(gev, "Encountered unsupported solver id in setupParameters.");
				return false;
		}
	}

	osi->resolve();

	if (osi->isProvenOptimal()) {
		if (!gamsOsiStoreSolution(gmo, *osi, false)) {
			gevLogStat(gev, "Failed to store LP solution. Only primal solution values will be available in GAMS solution file.\n");
			return false;
		}
	} else {
		gevLogStat(gev, "Solve of final LP failed. Only primal solution values will be available in GAMS solution file.\n");
		gevSetIntOpt(gev, gevInteger1, 0);
	}

	for (int i = 0; i < gmoN(gmo); ++i)
		if (gmoGetVarTypeOne(gmo, i) != var_X)
			osi->setColBounds(i, gmoGetVarLowerOne(gmo, i), gmoGetVarUpperOne(gmo, i));

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

DllExport GamsOsi* STDCALL createNewGamsOsiGlpk() {
   return new GamsOsi(GamsOsi::GLPK);
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
		assert(Cptr != NULL); \
		assert(Gptr != NULL); \
		char msg[256]; \
		if (!gmoGetReady(msg, sizeof(msg))) \
			return 1; \
		if (!gevGetReady(msg, sizeof(msg))) \
			return 1; \
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
osi_C_interface(ogl, GamsOsi::GLPK)
osi_C_interface(ogu, GamsOsi::GUROBI)
osi_C_interface(oxp, GamsOsi::XPRESS)
osi_C_interface(omk, GamsOsi::MOSEK)
