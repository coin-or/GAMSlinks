// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSIPOPT_HPP_
#define GAMSIPOPT_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"

#include "IpTNLP.hpp"
#include "IpIpoptApplication.hpp"
#include "IpSolveStatistics.hpp"

class GamsIpopt : public GamsSolver {
private:
	struct gmoRec* gmo;
	
	char           ipopt_message[100];
	
	Ipopt::SmartPtr<Ipopt::IpoptApplication>  ipopt;
	Ipopt::SmartPtr<Ipopt::TNLP>              nlp;

public:
	GamsIpopt();
	~GamsIpopt();
	
	int readyAPI(struct gmoRec* gmo, struct optRec* opt, struct dctRec* gcd);
	
//	int haveModifyProblem();
	
//	int modifyProblem();
	
	int callSolver();
	
	const char* getWelcomeMessage() { return ipopt_message; }

}; // GamsIpopt

extern "C" DllExport GamsIpopt* STDCALL createNewGamsIpopt();

#endif /*GAMSIPOPT_HPP_*/
