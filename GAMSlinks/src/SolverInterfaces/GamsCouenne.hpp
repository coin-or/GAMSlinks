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
}
namespace Ipopt {
	class Journalist;
}
class GamsCouenneSetup;
//class CouenneProblem;
//class expression;

class GamsCouenne: public GamsSolver {
private:
	struct gmoRec* gmo;
		
	char           couenne_message[100];

//	GamsMessageHandler*    msghandler;
	Ipopt::SmartPtr<GamsMINLP> minlp;
	GamsCouenneSetup*  couenne_setup;

//	CouenneProblem* setupProblem(Ipopt::SmartPtr<Ipopt::Journalist>& journalist);
//	expression* parseGamsInstructions(CouenneProblem* prob, int codelen, int* opcodes, int* fields, int constantlen, double* constants);
//	bool initializeCouenne(CouenneInterface& ci);
//	void writeSolution(Bonmin::OsiTMINLPInterface& osi_tminlp, int itercount);
//	void writeSolutionNoDual(Bonmin::OsiTMINLPInterface& osi_tminlp, int itercount);
	
//	bool isNLP();

public:
	GamsCouenne();
	~GamsCouenne();
	
	int readyAPI(struct gmoRec* gmo, struct optRec* opt, struct dctRec* gcd);
	
//	int haveModifyProblem();
	
//	int modifyProblem();
	
	int callSolver();
	
	const char* getWelcomeMessage() { return couenne_message; }

}; // GamsCouenne

extern "C" DllExport GamsCouenne* STDCALL createNewGamsCouenne();

#endif /*GAMSCOUENNE_HPP_*/
