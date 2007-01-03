// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#ifndef GAMSFINALIZE_HPP_
#define GAMSFINALIZE_HPP_

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "OsiSolverInterface.hpp"

// GAMS
#include "GamsModel.hpp"
#include "GamsMessageHandler.hpp"
/** Analyze solution stored in an OsiSolverInterface and writes GAMS solution file.
 */
void GamsFinalizeOsi(GamsModel *gm, GamsMessageHandler *myout, OsiSolverInterface *solver, int PresolveInfeasible);

#endif /*GAMSFINALIZE_HPP_*/
