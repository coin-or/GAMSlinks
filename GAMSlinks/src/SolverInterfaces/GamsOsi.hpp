// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOSI_HPP_
#define GAMSOSI_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"
#include "GamsOptions.hpp"

class GamsMessageHandler;
class OsiSolverInterface;

class GamsOsi : public GamsSolver {
public:
	typedef enum { CBC, CPLEX, GUROBI, MOSEK, XPRESS } OSISOLVER;
private:
	struct gmoRec* gmo;
	struct gevRec* gev;

	GamsMessageHandler* msghandler;
	OsiSolverInterface* osi;
	OSISOLVER           solverid;

	char osi_message[100];

	bool setupProblem(OsiSolverInterface& solver);
	bool setupStartingPoint();
	bool setupParameters();
	bool writeSolution(double cputime, double walltime);

	bool isLP();

public:
	GamsOsi(OSISOLVER solverid_);
	~GamsOsi();

	int readyAPI(struct gmoRec* gmo, struct optRec* opt);

//	int haveModifyProblem();

//	int modifyProblem();

	int callSolver();

	const char* getWelcomeMessage() { return osi_message; }

}; // GamsOsi

extern "C" DllExport GamsOsi* STDCALL createNewGamsOsiCplex();

extern "C" DllExport GamsOsi* STDCALL createNewGamsOsiGurobi();

extern "C" DllExport GamsOsi* STDCALL createNewGamsOsiMosek();

extern "C" DllExport GamsOsi* STDCALL createNewGamsOsiXpress();

#endif /*GAMSOSI_HPP_*/
