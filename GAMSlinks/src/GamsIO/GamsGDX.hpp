// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSGDX_HPP_
#define GAMSGDX_HPP_

#include "GAMSlinksConfig.h"

#include "GamsHandler.hpp"
#include "GamsDictionary.hpp"

extern "C" {
	struct dictRec;
	struct gdxRec;
}

/** Writing of GDX (Gams Data Exchange) files.
 */
class GamsGDX {
private:
	GamsHandler& gams;
	GamsDictionary& dict;
	
	struct gdxRec* gdx;
	
	void reportError(int n) const;
	
public:
	GamsGDX(GamsHandler& gams_, GamsDictionary& dict_);
	
	~GamsGDX();
	
	bool init();
	
	bool writePoint(const double* x, const double* rc, double objval, const char* filename) const;
	
};


#endif /*GAMSGDX_HPP_*/
