// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSGDX_HPP_
#define GAMSGDX_HPP_

#include "GAMSlinksConfig.h"

class GamsDictionary;
struct gmoRec;
struct gdxRec;

/** Wrapper class for GAMS data exchange (GDX) file handler.
 * Provides a method to simplify writing a column vector as GDX file.
 */
class GamsGDX {
private:
	struct gmoRec* gmo;
	GamsDictionary& dict;
	struct gdxRec* gdx;
	
	void reportError(int n) const;
	
public:
	/** Constructor.
	 * @param gams_ A GAMS handler.
	 * @param dict_ A GAMS dictionary.
	 */
	GamsGDX(struct gmoRec* gmo_, GamsDictionary& dictionary);
	
	/** Destructor.
	 */
	~GamsGDX();

	/** Loads and initializes the GDX dynamic library.
	 * @return True on success, False on failure.
	 */
	bool init();
	
	/** Writes a column vector to a GDX file.
	 * Assumes that the GDX library has been initialized successfully before.
	 * @param x Primal column values.
	 * @param rc Dual column values.
	 * @param objval Objective value for this point (needed if model is reformulated).
	 * @param filename The name of the GDX file to create.
	 */
	bool writePoint(const double* x, const double* rc, double objval, const char* filename) const;
	
};


#endif /*GAMSGDX_HPP_*/
