// Copyright (C) GAMS Development and others 2008-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#ifndef OSRL2GAMS_HPP_
#define OSRL2GAMS_HPP_

#include "GAMSlinksConfig.h"

class OSResult;
struct gmoRec;

/** Reads an optimization result and stores result and solution in a Gams Modeling Object.
 */
class OSrL2Gams {
private:
	struct gmoRec* gmo;

public:
	/** Constructor.
	 * @param gmo_ GMO handler.
	 */
	OSrL2Gams(struct gmoRec* gmo_);

	/** Destructor.
	 */
	~OSrL2Gams() {}
	
	/** Writes a solution into a GMO with the result given as OSResult object.
	 * @param osresult Optimization result as object.
	 */ 
	void writeSolution(OSResult& osresult);

	/** Writes a solution into a GMO with the result given as osrl string.
	 * @param osrl Optimization result as string.
	 */ 
	void writeSolution(std::string& osrl);
};

#endif /*OSRL2GAMS_HPP_*/
