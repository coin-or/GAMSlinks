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

struct gmoRec;
struct gcdRec;
struct optRec;

/** An abstract interface to a solver that takes Gams Modeling Objects (GMO) as input.
 */
class GamsSolver {
public:
	virtual ~GamsSolver() { }
	
	/** Initialization of solver interface and solver.
	 * Loads problem into solver.
	 */
	virtual int readyAPI(struct gmoRec* gmo, struct optRec* opt, struct gcdRec* gcd) = 0;
	
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

typedef GamsSolver* (*createNewGamsSolver_t)(void);

#endif /*GAMSSOLVER_HPP_*/
