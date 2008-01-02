// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsCutGenerator.hpp"

GamsCutGenerator::GamsCutGenerator(GamsBCH& bch_, const CbcModel& model_)
: bch(bch_), model(model_)
{ }

CglCutGenerator* GamsCutGenerator::clone() const {
	return new GamsCutGenerator(bch, model);
}

void GamsCutGenerator::generateCuts(const OsiSolverInterface &si, OsiCuts &cs, const CglTreeInfo info) const {
//	printf("current inc. sol.: %g\t%g\t%d\n", model.getObjValue(), model.getBestPossibleObjValue(), (int)model.bestSolution());
	if (model.getObjValue()>-si.getInfinity() && model.getObjValue()<si.getInfinity()) {
		assert(model.bestSolution());
//		if (!have_incumbent || true)
			bch.setIncumbentSolution(model.bestSolution(), model.getObjValue());
	}
//TODO	bch.setIncumbent();
//	printf("gm dim: x\t osi dim: %d\n", si.getNumCols());
	bch.setNodeSolution(si.getColSolution(), si.getObjValue(), si.getColLower(), si.getColUpper());

	std::vector<GamsBCH::Cut> cuts;
	bch.generateCuts(cuts);
	
	for (std::vector<GamsBCH::Cut>::iterator it(cuts.begin()); it!=cuts.end(); ++it) {
		// this constructors takes over the ownership of indices and coeff
		OsiRowCut* cut=new OsiRowCut(it->lb, it->ub, it->nnz, it->nnz, it->indices, it->coeff);
		cs.insert(cut);
	}
	
}
