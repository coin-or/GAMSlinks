// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSCBC_HPP_
#define GAMSCBC_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"

#include "GamsDictionary.hpp"
#include "GamsOptions.hpp"

class CbcModel;
class GamsMessageHandler;
class OsiSolverInterface;

class GamsCbc : public GamsSolver {
private:
	struct gmoRec* gmo;
	
	GamsDictionary dict;
	GamsOptions    options;
	
	GamsMessageHandler* msghandler;
	CbcModel*      model;
	int            cbc_argc;
	char**         cbc_args;
	
	char           cbc_message[100];

	bool setupProblem(OsiSolverInterface& solver);
	bool setupPrioritiesSOSSemiCon();
	bool setupStartingPoint();
	bool setupParameters();
	bool writeSolution(double cputime, double walltime);
	
	bool isLP();
	
public:
	GamsCbc();
	~GamsCbc();
	
	int readyAPI(struct gmoRec* gmo, struct optRec* opt, struct dctRec* gcd);
	
//	int haveModifyProblem();
	
//	int modifyProblem();
	
	int callSolver();
	
	const char* getWelcomeMessage() { return cbc_message; }

}; // GamsCbc

extern "C" DllExport GamsCbc* STDCALL createNewGamsCbc();

#endif /*GAMSCBC_HPP_*/
