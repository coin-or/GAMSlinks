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
 * If the model was solved to optimality, this method tries to obtain the optimal basis from the solver.
 * If solver->optimalBasisIsAvailable() returns positive, the basis returned by solver->getBasisStatus is used.
 * Else, if solver->getWarmStart() returns a basis, then this basis is used.
 * Otherwise we try to guess a basis, which is likely to fail.    
 * @param gm The GamsModel.
 * @param myout The GAMS message handler for output.
 * @param solver The OSI solver interface to read the solution from.
 * @param PresolveInfeasible Indicate, whether the solver found the model infeasible in the presolve. 
 */
void GamsFinalizeOsi(GamsModel *gm, GamsMessageHandler *myout, OsiSolverInterface *solver, bool PresolveInfeasible);

#endif /*GAMSFINALIZE_HPP_*/
