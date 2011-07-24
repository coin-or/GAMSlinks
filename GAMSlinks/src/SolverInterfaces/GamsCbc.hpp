// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSCBC_HPP_
#define GAMSCBC_HPP_

#include "GamsSolver.hpp"

#include <cstdlib>

class CbcModel;
class GamsMessageHandler;
class OsiSolverInterface;

class GamsCbc : public GamsSolver
{
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */
   struct optRec*        opt;                /**< GAMS options object */

	GamsMessageHandler*   msghandler;         /**< message handler */
	CbcModel*             model;              /**< CBC model object */
	int                   cbc_argc;           /**< number of parameters to pass to CBC */
	char**                cbc_args;           /**< parameters to pass to CBC */

	bool setupProblem();
	bool setupStartingPoint(
	   bool               mipstart            /**< should an initial primal solution been setup? */
	);
	bool setupParameters(
	   bool&              mipstart,           /**< variable where to store whether the mipstart option has been set */
      bool&              multithread         /**< variable where to store whether multiple threads should be used */
	);
   bool writeSolution(
      double             cputime,            /**< CPU time spend by solver */
      double             walltime,           /**< wallclock time spend by solver */
      bool               multithread         /**< did we solve the MIP in multithread mode */
   );

	bool isLP();

public:
	GamsCbc()
	: gmo(NULL),
	  gev(NULL),
	  opt(NULL),
	  msghandler(NULL),
	  model(NULL),
	  cbc_argc(0),
	  cbc_args(NULL)
	{ }

	~GamsCbc();

   int readyAPI(
      struct gmoRec*     gmo_,               /**< GAMS modeling object */
      struct optRec*     opt_                /**< GAMS options object */
   );

	int callSolver();
};

extern "C" DllExport GamsCbc* STDCALL createNewGamsCbc();

#endif /*GAMSCBC_HPP_*/
