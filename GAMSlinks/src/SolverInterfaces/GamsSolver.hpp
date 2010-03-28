// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSSOLVER_HPP_
#define GAMSSOLVER_HPP_

#include "GAMSlinksConfig.h"
#include "GamsSolver.h"

/** An abstract interface to a solver that takes Gams Modeling Objects (GMO) as input.
 */
class GamsSolver {
private:
	bool need_unload_gmo;
	bool need_unload_gev;
public:
	GamsSolver()
	: need_unload_gmo(false), need_unload_gev(false)
	{ }

	/** Unloads GMO library, if we loaded it.
	 */
	virtual ~GamsSolver();

	/** Loads the GMO library, if not already loaded.
	 * @return Nonzero on failure, 0 on success.
	 */
	int getGmoReady();

	/** Loads the GEV library, if not already loaded.
	 * @return Nonzero on failure, 0 on success.
	 */
	int getGevReady();

	/** Calls the GAMS license check, if build by GAMS.
	 * @return True if license check was skipped or successful.
	 */
	bool checkLicense(struct gmoRec* gmo);

	/** Registers a GAMS/CPLEX license, if build by GAMS.
	 * @return True if license was registered or no CPLEX available.
	 */
	bool registerGamsCplexLicense(struct gmoRec* gmo);

	/** Initialization of solver interface and solver.
	 * Loads problem into solver.
	 */
	virtual int readyAPI(struct gmoRec* gmo, struct optRec* opt) = 0;

	/** Indicates whether the solver interface and solver supports the modifyProblem call.
	 * Default: no
	 */
	virtual int haveModifyProblem() { return 0; }

	/** Notify solver that the GMO object has been modified and changes should be passed forward to the solver.
	 * Default: do nothing
	 */
	virtual int modifyProblem() { return -1; }

	/** Solves instance.
	 * Store solution information in GMO object.
	 */
	virtual int callSolver() = 0;

	/** Get a string about the solver name, version, and authors.
	 */
	virtual const char* getWelcomeMessage() = 0;

}; // class GamsSolver

typedef GamsSolver* STDCALL createNewGamsSolver_t(void);

#endif /*GAMSSOLVER_HPP_*/
