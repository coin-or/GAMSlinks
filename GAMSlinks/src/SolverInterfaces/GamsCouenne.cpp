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
  
 	return 1;
}

int GamsCouenne::callSolver() {
	return 1;
}

CouenneProblem* setupProblem() {
	//TODO
	return NULL;
}
