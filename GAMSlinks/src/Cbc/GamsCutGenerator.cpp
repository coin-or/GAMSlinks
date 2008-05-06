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

		//FIXME: currently we assume that the cuts received via BCH are valid globally; we hope, that in the future BCH will tell us more about it 
		cut->setGloballyValid(true);
//		cut->setGloballyValid(!info.inTree);
		
//		std::clog << "Cut consistent: " << cut->consistent() << cut->consistent(si)
//			<< " infeasible: " << cut->infeasible(si)
//			<< " violation: " << cut->violated(si.getColSolution())
//			<< "\t globally valid: " << cut->globallyValid()
//			<< std::endl;
//		cut->mutableRow().sortIncrIndex();
		cs.insert(cut);
	}
//	std::clog << "GamsCutGen: Added " << cs.sizeRowCuts() << " cuts." << std::endl;
//	cs.printCuts();
	
}
