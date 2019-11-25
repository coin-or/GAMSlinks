// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSSCIP_HPP_
#define GAMSSCIP_HPP_

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct palRec* palHandle_t;

#include <cstdlib>

#include "scip/type_retcode.h"

typedef struct Scip             SCIP;
typedef struct SCIP_Messagehdlr SCIP_MESSAGEHDLR;

/** GAMS interface to SCIP */
class GamsScip
{
   friend void printSCIPOptions();
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct palRec*        pal;                /**< GAMS audit and license object */

   SCIP*                 scip;               /**< SCIP structure */
   bool                  ipoptlicensed;      /**< whether a commercial Ipopt license is available */
   bool                  calledxprslicense;  /**< whether we have registered the GAMS/Xpress license (gevxpressliceInitTS) */

   SCIP_RETCODE setupSCIP();
   SCIP_RETCODE freeSCIP();

public:
   GamsScip()
   : gmo(NULL),
     gev(NULL),
     pal(NULL),
     scip(NULL),
     ipoptlicensed(false),
     calledxprslicense(false)
   { }

   ~GamsScip();

   int readyAPI(
      struct gmoRec*     gmo_                /**< GAMS modeling object */
   );

   int callSolver();
};

#endif /*GAMSSCIP_HPP_*/
