// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSBONMIN_HPP_
#define GAMSBONMIN_HPP_

#include <cstdlib>

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct palRec* palHandle_t;

class GamsMessageHandler;
namespace Bonmin
{
class BonminSetup;
}

/** GAMS interface to Bonmin */
class GamsBonmin
{
private:
	struct gmoRec*        gmo;                /**< GAMS modeling object */
	struct gevRec*        gev;                /**< GAMS environment */
	struct palRec*        pal;                /**< GAMS audit and license object */

   Bonmin::BonminSetup*  bonmin_setup;       /**< Bonmin solver application */
	GamsMessageHandler*   msghandler;         /**< COIN-OR message handler for GAMS */
   bool                  ipoptlicensed;      /**< whether a commercial Ipopt license is available */

	bool isNLP();
	bool isMIP();

public:
	GamsBonmin()
	: gmo(NULL),
	  gev(NULL),
	  pal(NULL),
	  bonmin_setup(NULL),
	  msghandler(NULL),
	  ipoptlicensed(false)
	{ }

	~GamsBonmin();

   int readyAPI(
      struct gmoRec*     gmo                 /**< GAMS modeling object */
   );

	int callSolver();
};

#endif /* GAMSBONMIN_HPP_ */
