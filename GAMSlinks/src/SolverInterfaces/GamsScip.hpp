// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSSCIP_HPP_
#define GAMSSCIP_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.hpp"

#include "scip/type_retcode.h"
#include "scip/type_message.h"

class GamsMessageHandler;

typedef struct Scip             SCIP;
typedef struct SCIP_Messagehdlr SCIP_MESSAGEHDLR;
typedef struct SCIP_Var         SCIP_VAR;
typedef struct SCIP_LPi         SCIP_LPI;

class GamsScip : public GamsSolver {
	friend SCIP_DECL_MESSAGEERROR(GamsScipPrintWarningOrError);
	friend SCIP_DECL_MESSAGEINFO(GamsScipPrintInfoOrDialog);
	
private:
   struct gmoRec*      gmo;
   struct gevRec*      gev;
   bool                isDemo;

   GamsMessageHandler* gamsmsghandler;

   SCIP*               scip;
   SCIP_MESSAGEHDLR*   scipmsghandler;
   SCIP_VAR**          vars;

   SCIP_LPI*           lpi;

   char                scip_message[300];

   SCIP_RETCODE freeLPI();
   SCIP_RETCODE freeSCIP();
   SCIP_RETCODE setupLPI();
   SCIP_RETCODE setupSCIP();
   SCIP_RETCODE setupSCIPParameters();
   SCIP_RETCODE setupMIQCP();
   SCIP_RETCODE setupInitialBasis();
   SCIP_RETCODE setupStartPoint();
   SCIP_RETCODE processLPSolution(double time);
   SCIP_RETCODE processMIQCPSolution();

   bool isLP();

public:
   GamsScip();
   ~GamsScip();

   int readyAPI(struct gmoRec* gmo, struct optRec* opt);

// int haveModifyProblem();

// int modifyProblem();

   int callSolver();

   const char* getWelcomeMessage() { return scip_message; }

}; // GamsScip

extern "C" DllExport GamsScip* STDCALL createNewGamsScip();

#endif /*GAMSSCIP_HPP_*/
