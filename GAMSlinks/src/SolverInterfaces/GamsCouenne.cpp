// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsBonmin.cpp 652 2009-04-11 17:14:51Z stefan $
//
// Author: Stefan Vigerske

#include "GamsCouenne.hpp"
#include "GamsCouenne.h"
#include "GamsMINLP.hpp"
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsCouenneSetup.hpp"

// to be sure to get (or not get) HAVE_M??? and HAVE_PARDISO defined
#include "IpoptConfig.h"

//#include "BonBonminSetup.hpp"
//#include "BonCouenneSetup.hpp"
#include "CouenneProblem.hpp"
#include "BonCbc.hpp"

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

extern "C" {
#ifndef HAVE_MA27
#define HAVE_HSL_LOADER
#else
# ifndef HAVE_MA28
# define HAVE_HSL_LOADER
# else
#  ifndef HAVE_MA57
#  define HAVE_HSL_LOADER
#  else
#   ifndef HAVE_MC19
#   define HAVE_HSL_LOADER
#   endif
#  endif
# endif
#endif
#ifdef HAVE_HSL_LOADER
#include "HSLLoader.h"
#endif
#ifndef HAVE_PARDISO
#include "PardisoLoader.h"
#endif
}

// GAMS
#ifdef GAMS_BUILD
#include "gmomcc.h"
#else
#include "gmocc.h"
#endif

using namespace Bonmin;
using namespace Ipopt;

GamsCouenne::GamsCouenne()
: gmo(NULL), msghandler(NULL), couenne_setup(NULL)
{
#ifdef GAMS_BUILD
	strcpy(couenne_message, "GAMS/CoinCouenne (Couenne Library 0.1)\nwritten by P. Belotti\n");
#else
	strcpy(couenne_message, "GAMS/Couenne (Couenne Library 0.1)\nwritten by P. Belotti\n");
#endif
}

GamsCouenne::~GamsCouenne() {
	delete couenne_setup;
	delete msghandler;
}

int GamsCouenne::readyAPI(struct gmoRec* gmo_, struct optRec* opt, struct dctRec* gcd) {
	gmo = gmo_;
	assert(gmo);
	assert(IsNull(minlp));
	assert(couenne_setup == NULL);

	char msg[256];
#ifndef GAMS_BUILD
  if (!gmoGetReadyD(GAMSIO_PATH, msg, sizeof(msg)))
#endif
  	if (!gmoGetReady(msg, sizeof(msg))) {
  		fprintf(stderr, "Error loading GMO library: %s\n",msg);
  		return 1;
  	}

	gmoObjStyleSet(gmo, ObjType_Fun);
	gmoObjReformSet(gmo, 1);
 	gmoIndexBaseSet(gmo, 0);
 	
 	if (!gmoN(gmo)) {
 		gmoLogStat(gmo, "Error: Bonmin requires variables.");
 		return 1;
 	}

 	for (int i = 0; i < gmoN(gmo); ++i)
		if (gmoGetVarTypeOne(gmo, i) == var_SC || gmoGetVarTypeOne(gmo, i) == var_SI) {
			gmoLogStat(gmo, "Error: Semicontinuous and semiinteger variables not supported by Bonmin.");
			return 1;
		}
 
  minlp = new GamsMINLP(gmo);
  CouenneProblem* prob = setupProblem();
  if (!prob)
  	return 1;

	couenne_setup = new GamsCouenneSetup(gmo);
	couenne_setup->InitializeCouenne(minlp, prob);

//	// instead of initializeOptionsAndJournalist we do it our own way, so we can use the SmagJournal
//  SmartPtr<OptionsList> options = new OptionsList();
//	SmartPtr<Journalist> journalist= new Journalist();
//  SmartPtr<Bonmin::RegisteredOptions> roptions = new Bonmin::RegisteredOptions();
// 	SmartPtr<Journal> jrnl = new GamsJournal(gmo, "console", J_ITERSUMMARY, J_STRONGWARNING);
//	jrnl->SetPrintLevel(J_DBG, J_NONE);
//	if (!journalist->AddJournal(jrnl))
//		gmoLogStat(gmo, "Failed to register GamsJournal for IPOPT output.");
//	options->SetJournalist(journalist);
//	options->SetRegisteredOptions(GetRawPtr(roptions));
//
//	couenne_setup->setOptionsAndJournalist(roptions, options, journalist);
//  couenne_setup->registerOptions();
//
//	roptions->SetRegisteringCategory("Linear Solver", Bonmin::RegisteredOptions::IpoptCategory);
//#ifdef HAVE_HSL_LOADER
//	// add option to specify path to hsl library
//  couenne_setup->roptions()->AddStringOption1("hsl_library", // name
//			"path and filename of HSL library for dynamic load",  // short description
//			"", // default value 
//			"*", // setting1
//			"path (incl. filename) of HSL library", // description1
//			"Specify the path to a library that contains HSL routines and can be load via dynamic linking. "
//			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options, e.g., ``linear_solver''."
//	);
//#endif
//#ifndef HAVE_PARDISO
//	// add option to specify path to pardiso library
//  couenne_setup->roptions()->AddStringOption1("pardiso_library", // name
//			"path and filename of Pardiso library for dynamic load",  // short description
//			"", // default value 
//			"*", // setting1
//			"path (incl. filename) of Pardiso library", // description1
//			"Specify the path to a Pardiso library that and can be load via dynamic linking. "
//			"Note, that you still need to specify to pardiso as linear_solver."
//	);
//#endif
//  
//  // add options specific to BCH heuristic callback
////TODO  BCHsetupOptions(*bonmin_setup.roptions());
////  printOptions(journalist, bonmin_setup.roptions());
//  
//	// Change some options
//	couenne_setup->options()->SetNumericValue("bound_relax_factor", 0, true, true);
//	couenne_setup->options()->SetNumericValue("nlp_lower_bound_inf", gmoMinf(gmo), false, true);
//	couenne_setup->options()->SetNumericValue("nlp_upper_bound_inf", gmoPinf(gmo), false, true);
//	if (gmoUseCutOff(gmo))
//		couenne_setup->options()->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == Obj_Min ? gmoCutOff(gmo) : -gmoCutOff(gmo), true, true);
//	couenne_setup->options()->SetNumericValue("bonmin.allowable_gap", gmoOptCA(gmo), true, true);
//	couenne_setup->options()->SetNumericValue("bonmin.allowable_fraction_gap", gmoOptCR(gmo), true, true);
//	if (gmoNodeLim(gmo))
//		couenne_setup->options()->SetIntegerValue("bonmin.node_limit", gmoNodeLim(gmo), true, true);
//	couenne_setup->options()->SetNumericValue("bonmin.time_limit", gmoResLim(gmo), true, true);
//
//	if (gmoNLM(gmo) == 0  && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp))
//		couenne_setup->options()->SetStringValue("hessian_constant", "yes", true, true); 
//	
//	try {
//		if (gmoOptFile(gmo)) {
//			couenne_setup->options()->SetStringValue("print_user_options", "yes", true, true);
//			char buffer[1024];
//			gmoNameOptFile(gmo, buffer);
//			couenne_setup->readOptionsFile(buffer);
//		}	else { // need to let Bonmin read something, otherwise it will try to read its default option file bonmin.opt
//			couenne_setup->readOptionsString(std::string());
//		}
//	} catch (IpoptException error) {
//		gmoLogStat(gmo, error.Message().c_str());
//	  return 1;
//	} catch (std::bad_alloc) {
//		gmoLogStat(gmo, "Error: Not enough memory\n");
//		return -1;
//	} catch (...) {
//		gmoLogStat(gmo, "Error: Unknown exception thrown.\n");
//		return -1;
//	}
//
//	std::string libpath;
//#ifdef HAVE_HSL_LOADER
//	if (couenne_setup->options()->GetStringValue("hsl_library", libpath, "")) {
//		char buffer[512];
//		if (LSL_loadHSL(libpath.c_str(), buffer, 512) != 0) {
//			gmoLogStatPChar(gmo, "Failed to load HSL library at user specified path: ");
//			gmoLogStat(gmo, buffer);
//		  return 1;
//		}
//	}
//#endif
//#ifndef HAVE_PARDISO
//	if (couenne_setup->options()->GetStringValue("pardiso_library", libpath, "")) {
//		char buffer[512];
//		if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512) != 0) {
//			gmoLogStatPChar(gmo, "Failed to load Pardiso library at user specified path: ");
//			gmoLogStat(gmo, buffer);
//		  return 1;
//		}
//	}
//#endif
//
//	couenne_setup->options()->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");
////	// or should we also check the tolerance for acceptable points?
////	bonmin_setup.options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, "");
////	bonmin_setup.options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, "");
//
//	bool hessian_is_approx = false;
//	std::string parvalue;
//	couenne_setup->options()->GetStringValue("hessian_approximation", parvalue, "");
//	if (parvalue == "exact") {
//		int do2dir, dohess;
//		if (gmoHessLoad(gmo, 10, 10, &do2dir, &dohess) || !dohess) { // TODO make 10 (=conopt default for rvhess) a parameter
//			gmoLogStat(gmo, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
//			couenne_setup->options()->SetStringValue("hessian_approximation", "limited-memory");
//			hessian_is_approx = true;
//	  }
//	} else
//		hessian_is_approx = true;
//	
//	if (hessian_is_approx) { // check whether QP strong branching is enabled
//		couenne_setup->options()->GetStringValue("varselect_stra", parvalue, "bonmin.");
//		if (parvalue == "qp-strong-branching") {
//			gmoLogStat(gmo, "Error: QP strong branching does not work when the Hessian is approximated. Aborting...");
//			return 1;
//		}
//	}
//	
//	msghandler = new GamsMessageHandler(gmo);
//	
//	// the easiest would be to call bonmin_setup.initializeBonmin(smagminlp), but then we cannot set the message handler
//	// so we do the following
//	try {
//		CouenneInterface ci;
//		ci.initialize(roptions, options, journalist, /* "bonmin.",*/ GetRawPtr(minlp));
//		ci.passInMessageHandler(msghandler);

//		initializeCouenne(ci);

//		std::cout << "INITIALIZE IPOPT SOLVER " << std::endl;
// 		app_ = new Bonmin::IpoptSolver(couenneSetup.roptions(),//GetRawPtr(roptions),  
//					       couenneSetup.options(),//GetRawPtr( options), 					       
//					       couenneSetup.journalist(),//GetRawPtr(jnlst),  
//					       prefix); 		
	      	
//		std::cout << "INITIALIZE COUENNE MODEL" << std::endl;
//		ci.setModel( GetRawPtr( minlp) );
//		std::cout << "INITIALIZE COUENNE SOLVER" << std::endl;
//		ci->setSolver( GetRawPtr( app_) );
			
//		std::cout << "INITIALIZE COUENNE " << std::endl;
//		couenneSetup.InitializeCouenne(argv, couenne, ci);
//
//		
////		OsiTMINLPInterface first_osi_tminlp;
//		//first_osi_tminlp.initialize(roptions, options, journalist, "bonmin.", smagminlp);
////		first_osi_tminlp.initialize(roptions, options, journalist, GetRawPtr(minlp));
////		first_osi_tminlp.passInMessageHandler(msghandler);
////		couenne_setup->initialize(ci);
//  } catch(CoinError &error) {
//  	char buf[1024];
//  	snprintf(buf, 1024, "%s::%s\n%s\n", error.className().c_str(), error.methodName().c_str(), error.message().c_str());
//		gmoLogStatPChar(gmo, buf);
//		return 1;
//	} catch (std::bad_alloc) {
//		gmoLogStat(gmo, "Error: Not enough memory!");
//		return -1;
//	} catch (...) {
//		gmoLogStat(gmo, "Error: Unknown exception thrown!");
//		return -1;
//	}
  
  
 	return 1;
}

int GamsCouenne::callSolver() {
	return 1;
}

CouenneProblem* setupProblem() {
	//TODO
	return NULL;
}
