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
class GamsCbc;

class GamsCouenne: public GamsSolver {
private:
	struct gmoRec* gmo;
		
	char           couenne_message[100];

	Ipopt::SmartPtr<GamsMINLP> minlp;
	GamsCouenneSetup*  couenne_setup;
	
	GamsCbc*       gamscbc;

	bool isMIP();

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
