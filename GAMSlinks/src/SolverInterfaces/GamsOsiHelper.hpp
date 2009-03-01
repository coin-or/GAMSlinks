// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOSIHELPER_HPP_
#define GAMSOSIHELPER_HPP_

#include "GAMSlinksConfig.h"

class OsiSolverInterface;
struct gmoRec;

/** Loads the problem from GMO into OSI.
 * @return True on success, false on failure.
 */
bool gamsOsiLoadProblem(struct gmoRec* gmo, OsiSolverInterface& solver);

/** Stores a solution from OSI in GMO.
 * @param swapRowStatus whether the status of nonbasic variables should be swapped between lower and upper
 * @return True on success, false on failure.
 */
bool gamsOsiStoreSolution(struct gmoRec* gmo, const OsiSolverInterface& solver, bool swapRowStatus = false);

#endif /*GAMSOSIHELPER_HPP_*/
