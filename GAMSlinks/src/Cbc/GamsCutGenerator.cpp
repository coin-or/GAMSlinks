// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsCutGenerator.hpp"

GamsCutGenerator::GamsCutGenerator(GamsBCH& bch_, CbcModel*& modelptr_)
: bch(bch_), modelptr(modelptr_)
{ }

CglCutGenerator* GamsCutGenerator::clone() const {
	return new GamsCutGenerator(bch, modelptr);
}

void GamsCutGenerator::generateCuts(const OsiSolverInterface &si, OsiCuts &cs, const CglTreeInfo info) const {
	if (modelptr && modelptr->bestSolution()) 
		bch.setIncumbentSolution(modelptr->bestSolution(), modelptr->getObjValue());
	
	if (!bch.doCuts()) return; // skip cut generation

	bch.setNodeSolution(si.getColSolution(), si.getObjValue(), si.getColLower(), si.getColUpper());

	std::vector<GamsBCH::Cut> cuts;
	bch.generateCuts(cuts);
	
	for (std::vector<GamsBCH::Cut>::iterator it(cuts.begin()); it!=cuts.end(); ++it) {
		// this constructors takes over the ownership of indices and coeff
		OsiRowCut* cut=new OsiRowCut(it->lb, it->ub, it->nnz, it->nnz, it->indices, it->coeff);
		cs.insert(cut);
	}
	
}
