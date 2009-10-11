// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSCOUENNE_HPP_
#define GAMSCOUENNE_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"
#include "GamsMINLP.hpp"

#include "IpSmartPtr.hpp"

class GamsMessageHandler;
namespace Bonmin {
	class OsiTMINLPInterface;
	class RegisteredOptions;
}
namespace Ipopt {
	class Journalist;
	class OptionsList;
}
class CouenneProblem;
class expression;
class GamsCbc;

class GamsCouenne: public GamsSolver {
private:
	struct gmoRec* gmo;
	struct gevRec* gev;
		
	char           couenne_message[100];

	Ipopt::SmartPtr<Ipopt::Journalist> jnlst;
	Ipopt::SmartPtr<Bonmin::RegisteredOptions> roptions;
	Ipopt::SmartPtr<Ipopt::OptionsList> options;
	
	Ipopt::SmartPtr<GamsMINLP> minlp;
//	CouenneProblem*            problem;
	
	GamsCbc*       gamscbc;

	bool isMIP();
  CouenneProblem* setupProblem();
  CouenneProblem* setupProblemMIQQP();
  expression* parseGamsInstructions(CouenneProblem* prob, int codelen, int* opcodes, int* fields, int constantlen, double* constants);
  void printOptions();

public:
	GamsCouenne();
	~GamsCouenne();
	
	int readyAPI(struct gmoRec* gmo, struct optRec* opt);
	
//	int haveModifyProblem();
	
//	int modifyProblem();
	
	int callSolver();
	
	const char* getWelcomeMessage() { return couenne_message; }

}; // GamsCouenne

extern "C" DllExport GamsCouenne* STDCALL createNewGamsCouenne();

#endif /*GAMSCOUENNE_HPP_*/
