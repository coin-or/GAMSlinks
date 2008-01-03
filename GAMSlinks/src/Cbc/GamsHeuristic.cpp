// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsHeuristic.hpp"

GamsHeuristic::GamsHeuristic(GamsBCH& bch_)
: bch(bch_)
{ setWhen(3);
  setHeuristicName("GamsBCH");
}

CbcHeuristic* GamsHeuristic::clone() const {
	return new GamsHeuristic(bch);
}

void GamsHeuristic::resetModel(CbcModel *model) {
	model_=model;
}

int GamsHeuristic::solution(double &objectiveValue, double *newSolution) {
	if (model_->bestSolution())
		bch.setIncumbentSolution(model_->bestSolution(), model_->getObjValue());
	
	OsiSolverInterface* solver=model_->solver();
	assert(solver);
	if (!bch.doHeuristic(model_->getBestPossibleObjValue(), solver->getObjValue())) return 0; // skip heuristic

	bch.setNodeSolution(solver->getColSolution(), solver->getObjValue(), solver->getColLower(), solver->getColUpper());

	return bch.runHeuristic(newSolution, objectiveValue);
}
