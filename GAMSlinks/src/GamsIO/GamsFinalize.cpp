// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#include "GamsFinalize.hpp"
#include "CoinWarmStart.hpp"
#include "CoinWarmStartBasis.hpp"

void GamsFinalizeOsi(GamsModel *gm, GamsMessageHandler *myout, OsiSolverInterface *solver, int PresolveInfeasible) {
	if (PresolveInfeasible) {
		gm->setIterUsed(0);
		gm->setResUsed(gm->SecondsSinceStart());
		gm->setObjVal(0.0);

		gm->setStatus(GamsModel::NormalCompletion,GamsModel::InfeasibleNoSolution);	
		(*myout) << "\n" << CoinMessageEol;
		(*myout) << "Presolve determined model infeasible.";
	}	else {
		// Get some statistics 
		gm->setIterUsed(solver->getIterationCount());
		gm->setResUsed(gm->SecondsSinceStart());
		gm->setObjVal(solver->getObjValue());

		gm->setStatus(GamsModel::ErrorSystemFailure,GamsModel::ErrorNoSolution);	
		(*myout) << "\n" << CoinMessageEol;

		if (solver->isAbandoned()) 
			(*myout) << "Model abandoned.";
		else if (solver->isProvenOptimal()) {
			if (gm->nDCols()) { 
// If it was a MIP we really don't know that we have the optimum solution, lets
// be conservative and just say we have an integer solution.
				(*myout) << "Integer Solution.";
				gm->setStatus(GamsModel::NormalCompletion,GamsModel::IntegerSolution);	
			} else {
				(*myout) << "Solved optimal.";
				gm->setStatus(GamsModel::NormalCompletion,GamsModel::Optimal);	
			}
		}	else if (solver->isProvenPrimalInfeasible()) {
			(*myout) << "Model infeasible.";
			gm->setStatus(GamsModel::NormalCompletion,GamsModel::InfeasibleNoSolution);	
		} 
		// GAMS doesn't have dual infeasible, so we hope for the best and call it
		// unbounded
		else if (solver->isProvenDualInfeasible()) {
			(*myout) << "Model unbounded.";
			gm->setStatus(GamsModel::NormalCompletion,GamsModel::UnboundedNoSolution);	
		} 
		else if (solver->isIterationLimitReached()) {
			(*myout) << "Iteration limit exceeded.";
			gm->setStatus(GamsModel::IterationInterrupt,GamsModel::NoSolutionReturned);	
		} 
		else if (solver->isPrimalObjectiveLimitReached()) {
			(*myout) << "Primal objective limit reached.";
		} 
		else if (solver->isDualObjectiveLimitReached()) {
			(*myout) << "Dual objective limit reached.";
		} else
			(*myout) << "Unknown solve outcome.";
	}
	(*myout) << CoinMessageEol;
		
	// We write a solution if model was declared optimal.
	if (GamsModel::Optimal==gm->getModelStatus() || 
			GamsModel::IntegerSolution==gm->getModelStatus()) {
		const double 
			*colLevel=solver->getColSolution(),
			*colMargin=solver->getReducedCost(); 
		const double 
			*rowLevel=solver->getRowActivity(),
			*rowMargin=solver->getRowPrice();		
		int 
			*colBasis=gm->ColBasis(),
			*rowBasis=gm->RowBasis(),
			*discVar=gm->ColDisc();
		double 
			*colLb=gm->ColLb(), 
			*colUb=gm->ColUb();
		const double 
			*rowLb=solver->getRowLower(), 
			*rowUb=solver->getRowUpper();

		(*myout) << "Writing solution. Objective:" << gm->getObjVal()
						 << "Time:" << gm->SecondsSinceStart() << "s\n " << CoinMessageEol;
						 
		if (solver->optimalBasisIsAvailable()) {
			solver->getBasisStatus(colBasis, rowBasis);
			// translate from OSI codes to GAMS codes
			for (int j=0; j<gm->nCols(); j++)
				if (discVar[j] || gm->SOSIndicator()[j])
					colBasis[j]=GamsModel::SuperBasic;
				else switch (colBasis[j]) {
					case 3: colBasis[j]=GamsModel::NonBasicLower; break;
					case 2: colBasis[j]=GamsModel::NonBasicUpper; break;
					case 1: colBasis[j]=GamsModel::Basic; break;
					case 0: colBasis[j]=GamsModel::SuperBasic; break;
					default: (*myout) << "Column basis status " << colBasis[j] << " unknown!" << CoinMessageEol; exit(0);
				}
			for (int i=0; i<gm->nRows(); ++i) 
				switch (rowBasis[i]) { // Clp interchange Lower and Upper activity of rows since it thinks in term of artifical variables
					case 2: rowBasis[i]=GamsModel::NonBasicLower; break;
					case 3: rowBasis[i]=GamsModel::NonBasicUpper; break;
					case 1: rowBasis[i]=GamsModel::Basic; break;
					case 0: rowBasis[i]=GamsModel::SuperBasic; break;
					default: (*myout) << "Row basis status " << rowBasis[i] << " unknown!" << CoinMessageEol; exit(0);
				}
		} else {
			CoinWarmStart* ws=solver->getWarmStart();
			CoinWarmStartBasis* wsb=dynamic_cast<CoinWarmStartBasis*>(ws);
			if (wsb) {
				for (int j=0; j<gm->nCols(); j++)
					if (discVar[j] || gm->SOSIndicator()[j])
						colBasis[j]=GamsModel::SuperBasic;
					else switch (wsb->getStructStatus(j)) {
						case CoinWarmStartBasis::basic: colBasis[j]=GamsModel::Basic; break;
						case CoinWarmStartBasis::atLowerBound: colBasis[j]=GamsModel::NonBasicLower; break;
						case CoinWarmStartBasis::atUpperBound: colBasis[j]=GamsModel::NonBasicUpper; break;
						case CoinWarmStartBasis::isFree: colBasis[j]=GamsModel::SuperBasic; break;
						default: (*myout) << "Column basis status " << wsb->getStructStatus(j) << " unknown!" << CoinMessageEol; exit(0);
					}
				for (int j=0; j<gm->nRows(); j++) {
					switch (wsb->getArtifStatus(j)) {
						case CoinWarmStartBasis::basic: rowBasis[j]=GamsModel::Basic; break;
						case CoinWarmStartBasis::atLowerBound: rowBasis[j]=GamsModel::NonBasicLower; break;
						case CoinWarmStartBasis::atUpperBound: rowBasis[j]=GamsModel::NonBasicUpper; break;
						case CoinWarmStartBasis::isFree: rowBasis[j]=GamsModel::SuperBasic; break;
						default: (*myout) << "Row basis status " << wsb->getStructStatus(j) << " unknown!" << CoinMessageEol; exit(0);
					}
				}
			} else { // last alternative: trying to guess the basis... this will likely be wrong			
				for (int j=0; j<gm->nCols(); j++) {
					if (discVar[j] || gm->SOSIndicator()[j])
						colBasis[j] = GamsModel::SuperBasic; // (for discrete only)
					else if (colMargin[j]) {
						if (colLevel[j] - colLb[j] < 1e-4)
							colBasis[j] = GamsModel::NonBasicLower;
						else if (colUb[j] - colLevel[j] < 1e-4)
							colBasis[j] = GamsModel::NonBasicUpper;
						else 
							colBasis[j] = GamsModel::SuperBasic;
					} else
						colBasis[j] = GamsModel::Basic;
				}
		
				for (int i=0; i<gm->nRows(); i++) {
					if (rowMargin[i]) {
						if (rowLevel[i] - rowLb[i] < 1e-4)
							rowBasis[i] = GamsModel::NonBasicLower;
						else if (rowUb[i] - rowLevel[i] < 1e-4)
							rowBasis[i] = GamsModel::NonBasicUpper;
						else 
							rowBasis[i] = GamsModel::SuperBasic; 
					} else
						rowBasis[i] = GamsModel::Basic;
				}
			}
			if (ws) delete ws;
		}

		gm->setSolution(colLevel, (const double *) colMargin, (const int *) colBasis, 0,
										rowLevel, (const double *) rowMargin, (const int *) rowBasis, 0);
	}	else {
		gm->setSolution(); // Need this to trigger the write of GAMS solution file
	}
}
