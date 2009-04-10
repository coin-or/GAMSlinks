// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Stefan Vigerske

#ifndef GAMSOS_HPP_
#define GAMSOS_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"
#include "GamsOptions.hpp"
#include <string>

class OSInstance;
class OSResult;

/* Interface between a GAMS model in form of a Gams Modeling Object (GMO) and Optimization Services (OS) routines.
 */
class GamsOS : public GamsSolver {
private:
	struct gmoRec* gmo;
	
	char           os_message[100];
	
	GamsOptions    gamsopt;
	OSInstance*    osinstance;

	std::string getSolverName(bool isnonlinear, bool isdiscrete);
	bool localSolve(OSInstance* osinstance, std::string& osol);
	bool remoteSolve(OSInstance* osinstance, std::string& osol);
	bool processResult(std::string* osrl, OSResult* osresult);

public:
	GamsOS();

	int readyAPI(struct gmoRec* gmo, struct optRec* opt, struct dctRec* gcd);
	
//	int haveModifyProblem();
	
//	int modifyProblem();
	
	int callSolver();
	
	const char* getWelcomeMessage() { return os_message; }	
};

extern "C" DllExport GamsOS* STDCALL createNewGamsOS();

#endif /*GAMSOS_HPP_*/
