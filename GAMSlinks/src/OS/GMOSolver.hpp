// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Stefan Vigerske

#ifndef GMOSOLVER_HPP_
#define GMOSOLVER_HPP_

#include "GAMSlinksConfig.h"

#include <string>

class OSInstance;
class OSResult;
class OSiLReader;
class OSrLWriter;
#include "OSDefaultSolver.h"
#include "OSErrorClass.h"

extern "C" {
#include "gmocc.h"
}

#include "GamsHandlerGmo.hpp"
#include "GamsOptions.hpp"

/* Interface between a GAMS model in form of a Gams Modeling Object (GMO) and Optimization Services (OS) routines.
 */
class GMOSolver : public DefaultSolver {
private:
	gmoHandle_t gmo;
	
	OSiLReader* osilreader;
	OSrLWriter* osrlwriter;

	void buildGmoInstance(OSInstance* osinstance);

public:
	GMOSolver();
	
	~GMOSolver();
	
	/** solve results in an instance being read into the Ipopt
	 * data structrues and optimized */ 
	void solve() throw (ErrorClass);
	
	/*! \fn void CoinSolver::buildSolverInstance() 
	 *  \brief The implementation of the virtual functions. 
	 *  \return void.
	 */	
	void buildSolverInstance() throw(ErrorClass);	
};

#endif /*GMOSOLVER_HPP_*/
