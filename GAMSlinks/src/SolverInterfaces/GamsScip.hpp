// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSSCIP_HPP_
#define GAMSSCIP_HPP_

#include "GamsSolver.hpp"

#include <cstdlib>

#include "scip/type_retcode.h"
#include "scip/type_message.h"

class GamsMessageHandler;

typedef struct Scip             SCIP;
typedef struct SCIP_Messagehdlr SCIP_MESSAGEHDLR;
typedef struct SCIP_Var         SCIP_VAR;
typedef struct SCIP_LPi         SCIP_LPI;

/** GAMS interface to SCIP */
class GamsScip : public GamsSolver
{
   friend SCIP_DECL_MESSAGEERROR(GamsScipPrintWarningOrError);
   friend SCIP_DECL_MESSAGEINFO(GamsScipPrintInfo);

private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   bool                  isDemo;             /**< whether we run SCIP in demo mode */

   SCIP*                 scip;               /**< SCIP structure */
   SCIP_MESSAGEHDLR*     scipmsghandler;     /**< SCIP message handler to write through GEV */

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
   GamsScip()
   : gmo(NULL),
     gev(NULL),
     isDemo(false),
     scip(NULL),
     scipmsghandler(NULL)
   { }

   ~GamsScip();

   int readyAPI(
      struct gmoRec*     gmo_,               /**< GAMS modeling object */
      struct optRec*     opt_                /**< GAMS options object */
   );

   int callSolver();
};

#endif /*GAMSSCIP_HPP_*/
