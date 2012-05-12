// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSBONMIN_HPP_
#define GAMSBONMIN_HPP_

#include <cstdlib>

#include "GamsSolver.hpp"

class GamsMessageHandler;
namespace Bonmin
{
class BonminSetup;
}

/** GAMS interface to Bonmin */
class GamsBonmin : public GamsSolver
{
private:
	struct gmoRec*        gmo;                /**< GAMS modeling object */
	struct gevRec*        gev;                /**< GAMS environment */

   Bonmin::BonminSetup*  bonmin_setup;       /**< Bonmin solver application */
	GamsMessageHandler*   msghandler;         /**< COIN-OR message handler for GAMS */
   bool                  ipoptLicensed;      /**< whether a commercial Ipopt license is available */

	bool isNLP();
	bool isMIP();

public:
	GamsBonmin()
	: gmo(NULL),
	  gev(NULL),
	  bonmin_setup(NULL),
	  msghandler(NULL),
	  ipoptLicensed(false)
	{ }

	~GamsBonmin();

   int readyAPI(
      struct gmoRec*     gmo,                /**< GAMS modeling object */
      struct optRec*     opt                 /**< GAMS options object */
   );

	int callSolver();

   /** indicates whether the solver interface and solver supports the modifyProblem call */
   static int haveModifyProblem()
   {
      return -1;
   }
};

#endif /* GAMSBONMIN_HPP_ */
