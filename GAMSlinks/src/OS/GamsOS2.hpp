// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Stefan Vigerske

#ifndef GAMSOS2_HPP_
#define GAMSOS2_HPP_

#include "GAMSlinksConfig.h"

#include <string>

class OSInstance;
class OSResult;

extern "C" {
#include "gmocc.h"
}

#include "GamsHandlerGmo.hpp"
#include "GamsOptions.hpp"

/* Interface between a GAMS model in form of a Gams Modeling Object (GMO) and Optimization Services (OS) routines.
 */
class GamsOS {
private:
	/** Handle for GMO.
	 */
	gmoHandle_t gmo;
	
	GamsHandlerGmo gamshandler;
	GamsOptions gamsopt;
	

	std::string getSolverName(bool isnonlinear, bool isdiscrete);
	bool localSolve(OSInstance* osinstance, std::string& osol);
	bool remoteSolve(OSInstance* osinstance, std::string& osol);
	bool processResult(std::string* osrl, OSResult* osresult);

public:
	GamsOS(gmoHandle_t gmo_);

	/** "Executes" the GMO/OS link.
	 * @return True on success, False on failure.
	 */
  bool execute();
	
};

#endif /*GAMSOS2_HPP_*/
