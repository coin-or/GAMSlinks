// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSCUTGENERATOR_HPP_
#define GAMSCUTGENERATOR_HPP_

#include "GAMSlinksConfig.h"
// from CoinUtils
#include "CoinPragma.hpp"

#include "GamsBCH.hpp"
#include "CglCutGenerator.hpp"
#include "CbcModel.hpp"

class GamsCutGenerator : public CglCutGenerator {
private:
	GamsBCH& bch;
	
	const CbcModel& model;

public:
	GamsCutGenerator(GamsBCH& bch_, const CbcModel& model_);
	
	void generateCuts(const OsiSolverInterface &si, OsiCuts &cs, const CglTreeInfo info) const;
	
	CglCutGenerator* clone() const; 
};

#endif /*GAMSCUTGENERATOR_HPP_*/
