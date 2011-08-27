// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsHeuristic.hpp 374 2008-03-02 12:02:50Z stefan $
//
// Author: Stefan Vigerske

#ifndef GAMSHEURISTIC_HPP_
#define GAMSHEURISTIC_HPP_

#include "GAMSlinksConfig.h"
// from CoinUtils
#include "CoinPragma.hpp"

#include "GamsBCH.hpp"
#include "CbcHeuristic.hpp"
#include "CbcModel.hpp"

class GamsHeuristic : public CbcHeuristic {
private:
	GamsBCH& bch;
	

public:
	GamsHeuristic(GamsBCH& bch_);
	
	CbcHeuristic* clone() const;
	
	void resetModel(CbcModel *model);
	
	/**
	 * Sets solution values if good, sets objective value
	 * @return 0 if no solution, 1 if valid solution with better objective value than one passed in 
	 */
	int solution(double &objectiveValue, double *newSolution);
	
	using CbcHeuristic::solution;
};

#endif /*GAMSHEURISTIC_HPP_*/
