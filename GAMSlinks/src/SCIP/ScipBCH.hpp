// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: ScipBCH.hpp 738 2009-09-09 16:18:27Z stefan $
//
// Author: Stefan Vigerske

#ifndef SCIPBCH_HPP_
#define SCIPBCH_HPP_

#include "smag.h"
#include "GamsBCH.hpp"
#include "GamsHandler.hpp"

#include "scip/type_scip.h"
#include "scip/type_retcode.h"
#include "scip/type_var.h"

/** Adds BCH relevant parameters to a SCIP object.
 */
SCIP_RETCODE BCHaddParam(SCIP* scip);

/** Setup BCH routines for SCIP.
 * Reads parameters, initialize GamsBCH object, and adds cut generator and heuristic callbacks to scip.
 */
SCIP_RETCODE BCHsetup(SCIP* scip, SCIP_VAR*** vars, smagHandle_t prob, GamsHandler& gamshandler, GamsDictionary& gamsdict, GamsBCH*& bch, void*& bchdata);

/** Cleans up memory allocated by BCH routines.
 */
SCIP_RETCODE BCHcleanup(smagHandle_t prob, GamsBCH*& bch, void*& bchdata);

#endif /*SCIPBCH_HPP_*/
