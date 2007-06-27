// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#ifndef GAMSFINALIZE_HPP_
#define GAMSFINALIZE_HPP_

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"

#include "OsiSolverInterface.hpp"

/** Analyze solution stored in an OsiSolverInterface and writes GAMS solution file.
 * @see GamsWriteSolutionOsi on how the solution is written. 
 * @param gm The GamsModel.
 * @param myout A GAMS message handler for output.
 * @param solver The OSI solver interface to read the solution from.
 * @param PresolveInfeasible Indicate, whether the solver found the model infeasible in the presolve.
 * @param TimeLimitExceeded Indicate, whether a time limit was exceeded. (Time Limits are not directly supported by OsiSolverInterface.)  
 */
void GamsFinalizeOsi(GamsModel *gm, GamsMessageHandler *myout, OsiSolverInterface *solver, bool PresolveInfeasible, bool TimeLimitExceeded=false);

/** Writes GAMS solution file for a solution stored in an OsiSolverInterface.
 * If solver->optimalBasisIsAvailable() returns positive, the basis returned by solver->getBasisStatus is used.
 * Else, if solver->getWarmStart() returns a basis, then this basis is used.
 * Otherwise we try to guess a basis, which is likely to fail.    
 * @param gm The GamsModel.
 * @param myout A GAMS message handler for output.
 * @param solver The OSI solver interface to read the solution from.
 */
void GamsWriteSolutionOsi(GamsModel *gm, GamsMessageHandler *myout, OsiSolverInterface *solver);

#endif /*GAMSFINALIZE_HPP_*/
