// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#ifndef OSRL2GMO_HPP_
#define OSRL2GMO_HPP_

#include "GAMSlinksConfig.h"

class OSResult;
extern "C" struct gmoRec;

/** Reads an optimization result and stores result and solution in a Gams Modeling Object.
 */
class OSrL2GMO {
public:
	/** Constructor.
	 * @param gmo_ GMO handler.
	 */
	OSrL2GMO(struct gmoRec* gmo_);

	/** Destructor.
	 */
	~OSrL2GMO() {}
	
	/** Writes a solution into a GMO with the result given as OSResult object.
	 * @param osresult Optimization result as object.
	 */ 
	void writeSolution(OSResult& osresult);

	/** Writes a solution into a GMO with the result given as osrl string.
	 * @param osrl Optimization result as string.
	 */ 
	void writeSolution(std::string& osrl);

private:
	struct gmoRec* gmo;
};


#endif /*OSRL2GMO_HPP_*/
