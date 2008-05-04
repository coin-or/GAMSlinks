// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsScip.cpp 420 2008-03-27 12:53:42Z stefan $
//
// Author: Stefan Vigerske

#ifndef SCIPBCH_HPP_
#define SCIPBCH_HPP_

#include "smag.h"
#include "GamsBCH.hpp"
#include "GamsHandler.hpp"

extern "C" {
#include "scip/scip.h"
}

/** Adds BCH relevant parameters to a SCIP object.
 */
SCIP_RETCODE BCHaddParam(SCIP* scip);

/** Setup BCH routines for SCIP.
 * Reads parameters, initialize GamsBCH object, and adds cut generator and heuristic callbacks to scip.
 */
SCIP_RETCODE BCHsetup(GamsBCH*& bch, smagHandle_t prob, GamsHandler& gamshandler, GamsDictionary& gamsdict, SCIP* scip);


#endif /*SCIPBCH_HPP_*/
