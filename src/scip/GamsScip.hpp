// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSSCIP_HPP_
#define GAMSSCIP_HPP_

#include <cstdlib>

#include "GamsLinksConfig.h"
#include "scip/type_retcode.h"

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct palRec* palHandle_t;

typedef struct Scip             SCIP;
typedef struct SCIP_Messagehdlr SCIP_MESSAGEHDLR;

/** GAMS interface to SCIP */
class DllExport GamsScip
{
   friend int main(int, char**);
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct palRec*        pal;                /**< GAMS audit and license object */

   SCIP*                 scip;               /**< SCIP structure */
   bool                  ipoptlicensed;      /**< whether a commercial Ipopt license is available */
   bool                  calledxprslicense;  /**< whether we have registered the GAMS/Xpress license (gevxpressliceInitTS) */
   bool                  gcg;                /**< whether we want to run GCG */

   SCIP_RETCODE setupSCIP();
   SCIP_RETCODE freeSCIP();

public:
   GamsScip(bool gcg_ = false)
   : gmo(NULL),
     gev(NULL),
     pal(NULL),
     scip(NULL),
     ipoptlicensed(false),
     calledxprslicense(false),
     gcg(gcg_)
   { }

   ~GamsScip();

   int readyAPI(
      struct gmoRec*     gmo_                /**< GAMS modeling object */
   );

   int callSolver();
};

#endif /*GAMSSCIP_HPP_*/
